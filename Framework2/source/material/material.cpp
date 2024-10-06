/*
Copyright 2018 Alejandro Cosin

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

// GLOBAL INCLUDES

// PROJECT INCLUDES
#include "../../include/material/material.h"
#include "../../include/material/materialmanager.h"
#include "../../include/texture/texturemanager.h"
#include "../../include/texture/texture.h"
#include "../../include/core/coremanager.h"
#include "../../include/parameter/attributedata.h"
#include "../../include/parameter/attributedefines.h"
#include "../../include/pipeline/pipeline.h"
#include "../../include/shader/shader.h"
#include "../../include/shader/shadermanager.h"
#include "../../include/shader/sampler.h"
#include "../../include/shader/shaderreflection.h"
#include "../../include/shader/uniformBase.h"
#include "../../include/uniformbuffer/uniformbuffer.h"
#include "../../include/shader/shaderstoragebuffer.h"
#include "../../include/buffer/buffer.h"
#include "../../include/shader/shadertoplevelaccelerationstructure.h"

// NAMESPACE
using namespace attributedefines;

// DEFINES

// STATIC MEMBER INITIALIZATION

/////////////////////////////////////////////////////////////////////////////////////////////

Material::Material(string &&name, string &&className) : GenericResource(move(name), move(className), GenericResourceType::GRT_MATERIAL)
	, m_shader(nullptr)
	, m_pipelineLayout(VK_NULL_HANDLE)
	, m_exposedStructFieldDirty(false)
	, m_pushConstantExposedStructFieldDirty(false)
	, m_exposedStructFieldSize(0)
	, m_pushConstantExposedStructFieldSize(0)
	, m_materialUniformBufferIndex(-1)
	, m_isCompute(false)
	, m_isRayTracing(false)
	, m_isEmitter(false)
	, m_resourcesUsed(MaterialBufferResource::MBR_MODEL | MaterialBufferResource::MBR_CAMERA | MaterialBufferResource::MBR_MATERIAL)
	, m_materialInstanceIndex(shaderM->getNextInstanceSuffix())
	, m_materialSurfaceType(MaterialSurfaceType::MST_OPAQUE)
{
	m_vectorClearValue.resize(2);
	m_vectorClearValue[0].color.float32[0] = 1.0f;
	m_vectorClearValue[0].color.float32[1] = 1.0f;
	m_vectorClearValue[0].color.float32[2] = 1.0f;
	m_vectorClearValue[0].color.float32[3] = 1.0f;

	// Specify the depth/stencil clear value
	m_vectorClearValue[1].depthStencil.depth   = 1.0f;
	m_vectorClearValue[1].depthStencil.stencil = 0;
}

/////////////////////////////////////////////////////////////////////////////////////////////

Material::~Material()
{
	destroyMaterialResources();
}

/////////////////////////////////////////////////////////////////////////////////////////////

void Material::init()
{
	loadResources();
	loadShader();

	if (m_shader != nullptr)
	{
		m_isCompute    = m_shader->getIsCompute();
		m_isRayTracing = m_shader->getIsRayTracing();
	}

	buildMaterialResources();
}

/////////////////////////////////////////////////////////////////////////////////////////////

bool Material::matchExposedResources()
{
	bool result = true;
	const vectorUniformBasePtr& vecUniformBase = m_shader->getVecUniformBase();

	forI(m_vectorExposedStructField.size())
	{
		ExposedStructField& exposedStructField = m_vectorExposedStructField[i];
		bool foundStructFieldResource = false;

		forJ(vecUniformBase.size())
		{
			UniformBase* uniform = vecUniformBase[j];

			if ((exposedStructField.getInternalType()    == uniform->getResourceInternalType()) &&
				(exposedStructField.getStructName()      == uniform->getStructName()) &&
				(exposedStructField.getStructFieldName() == uniform->getName()))
			{
				exposedStructField.setStructFieldResource(uniform);
				foundStructFieldResource = true;
			}
		}

		if (!foundStructFieldResource)
		{
			cout << "ERROR: No match found for struct field resource " << exposedStructField.getStructName() << "." << exposedStructField.getStructName() << endl;
		}

		if (exposedStructField.getStructFieldResource() == nullptr)
		{
			result = false;
		}
	}

	const vectorUniformBasePtr& vecPushConstantUniformBase = m_shader->refPushConstant().refVecUniformBase();
	forI(m_vectorPushConstantExposedStructField.size())
	{
		ExposedStructField& exposedStructField = m_vectorPushConstantExposedStructField[i];

		forJ(vecPushConstantUniformBase.size())
		{
			UniformBase* uniform = vecPushConstantUniformBase[j];

			if ((exposedStructField.getInternalType()    == uniform->getResourceInternalType()) &&
				(exposedStructField.getStructName()      == uniform->getStructName()) &&
				(exposedStructField.getStructFieldName() == uniform->getName()))
			{
				exposedStructField.setStructFieldResource(uniform);
			}
		}

		if (exposedStructField.getStructFieldResource() == nullptr)
		{
			result = false;
		}
	}

	return result;
}

/////////////////////////////////////////////////////////////////////////////////////////////

void Material::unmatchExposedResources()
{
	m_vectorDirtyExposedStructField.clear();

	forI(m_vectorExposedStructField.size())
	{
		m_vectorExposedStructField[i].setStructFieldResource(nullptr);
	}

	forI(m_vectorPushConstantExposedStructField.size())
	{
		m_vectorPushConstantExposedStructField[i].setStructFieldResource(nullptr);
	}

	m_exposedStructFieldSize             = 0;
	m_pushConstantExposedStructFieldSize = 0;
}

/////////////////////////////////////////////////////////////////////////////////////////////

void Material::buildPipeline()
{
	// This are the steps to use resources in shaders
	// Pipeline side:
	// 1. For each data type to use (textures, uniform buffer, etc), make a VkDescriptorSetLayoutBinding
	// 2. Take all generated VkDescriptorSetLayoutBindings and make a VkDescriptorSetLayout
	// 3. Take the generated VkDescriptorSetLayout and generate a VkPipelineLayout, used when building a pipeline for the corresponding shader
	// Shader side:
	// 1. Build a descriptor pool, with the types of descriptor sets that will be contain (and how many will be generated), using a struct of
	//    type VkDescriptorPoolSize. Specify how many allocations will be done.
	// 2. Generate the corresponding descriptor set
	// 3. For this generated descriptor set, fill the details of the descriptors it has within: for each binding, allocate a VkWriteDescriptorSet
	//    (Note: for uniform buffers, an extra structure of type VkDescriptorBufferInfo inside one of the fields of the VkWriteDescriptorSet is needed
	// 4. Update the descriptor set with this information given by the VkWriteDescriptorSet data
	// 5. Use this descriptor set together with the generated VkPipelineLayout in descriptor set binding commands like vkCmdBindDescriptorSets

	uint numResourcesUsed = getNumDynamicUniformBufferResourceUsed();

	m_arrayDescriptorSetLayout.resize(numResourcesUsed);

	m_arrayDescriptorSetLayoutIsShared.resize(numResourcesUsed);

	forI(numResourcesUsed)
	{
		m_arrayDescriptorSetLayoutIsShared[i] = true;
	}

	vector<VkDescriptorType>   vectorDescriptorType;
	vector<uint32_t>           vectorBindingIndex;
	vector<VkShaderStageFlags> vectorStageFlags;
	vector<void*>              vectorDescriptorInfo;
	vectorInt                  vectorDescriptorCount; // Used to count how many elements are really present in the field VkWriteDescriptorSet::descriptorCount
	vector<int>                vectorDescriptorInfoHint;

	uint numDescriptorInfo = numResourcesUsed;
	numDescriptorInfo     += uint(m_shader->getVecTextureSampler().size());
	numDescriptorInfo     += uint(m_shader->getVecImageSampler().size());
	numDescriptorInfo     += uint(m_shader->getVectorShaderStorageBuffer().size());
	numDescriptorInfo     += uint(m_shader->getVectorTLAS().size());

	vectorDescriptorInfo.resize(numDescriptorInfo);
	vectorDescriptorCount.resize(numDescriptorInfo);
	uint descriptorInfoCounter = 0;

	forI(numResourcesUsed)
	{
		vectorDescriptorType.push_back(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC);
	}
	
	// TODO: Refactor this code
	if (m_isCompute || m_isRayTracing)
	{
		VkShaderStageFlags stageFlag;

		ShaderStageFlag shaderStageFlag = m_shader->getShaderStage();
		if(shaderStageFlag & ShaderStageFlag::SSF_COMPUTE)
		{
			stageFlag = VkShaderStageFlagBits::VK_SHADER_STAGE_COMPUTE_BIT;
		}
		else if ((shaderStageFlag & ShaderStageFlag::SSF_RAYGENERATION) && !(shaderStageFlag & ShaderStageFlag::SSF_RAYHIT) && !(shaderStageFlag & ShaderStageFlag::SSF_RAYMISS))
		{
			stageFlag = VkShaderStageFlagBits::VK_SHADER_STAGE_RAYGEN_BIT_KHR;
		}
		else if (!(shaderStageFlag & ShaderStageFlag::SSF_RAYGENERATION) && (shaderStageFlag & ShaderStageFlag::SSF_RAYHIT) && !(shaderStageFlag & ShaderStageFlag::SSF_RAYMISS))
		{
			stageFlag = VkShaderStageFlagBits::VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR;
		}
		else if (!(shaderStageFlag & ShaderStageFlag::SSF_RAYGENERATION) && !(shaderStageFlag & ShaderStageFlag::SSF_RAYHIT) && (shaderStageFlag & ShaderStageFlag::SSF_RAYMISS))
		{
			stageFlag = VkShaderStageFlagBits::VK_SHADER_STAGE_MISS_BIT_KHR;
		}
		else if ((shaderStageFlag & ShaderStageFlag::SSF_RAYGENERATION) && (shaderStageFlag & ShaderStageFlag::SSF_RAYHIT) && !(shaderStageFlag & ShaderStageFlag::SSF_RAYMISS))
		{
			stageFlag = VK_SHADER_STAGE_RAYGEN_BIT_KHR | VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR;
		}
		else if ((shaderStageFlag & ShaderStageFlag::SSF_RAYGENERATION) && !(shaderStageFlag & ShaderStageFlag::SSF_RAYHIT) && (shaderStageFlag & ShaderStageFlag::SSF_RAYMISS))
		{
			stageFlag = VK_SHADER_STAGE_RAYGEN_BIT_KHR | VK_SHADER_STAGE_MISS_BIT_KHR;
		}
		else if ((shaderStageFlag & ShaderStageFlag::SSF_RAYGENERATION) && (shaderStageFlag & ShaderStageFlag::SSF_RAYHIT) && (shaderStageFlag & ShaderStageFlag::SSF_RAYMISS))
		{
			stageFlag = VK_SHADER_STAGE_RAYGEN_BIT_KHR | VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR | VK_SHADER_STAGE_MISS_BIT_KHR;
		}

		forI(numResourcesUsed)
		{
			vectorStageFlags.push_back(stageFlag);
		}
	}
	else
	{
		// TODO: automatize this part, "oring" each general buffer used for every shader stage truly used in each shader
		if (m_resourcesUsed & MaterialBufferResource::MBR_MODEL)
		{
			vectorStageFlags.push_back(VK_SHADER_STAGE_VERTEX_BIT);
		}
		if (m_resourcesUsed & MaterialBufferResource::MBR_CAMERA)
		{
			vectorStageFlags.push_back(VK_SHADER_STAGE_VERTEX_BIT);
		}
		if (m_resourcesUsed & MaterialBufferResource::MBR_MATERIAL)
		{
			vectorStageFlags.push_back(VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_GEOMETRY_BIT | VK_SHADER_STAGE_FRAGMENT_BIT);
		}
	}

	if (m_isRayTracing)
	{
		m_shader->buildShaderGroupInfo();
	}

	if (m_resourcesUsed & MaterialBufferResource::MBR_MODEL)
	{
		vectorDescriptorInfo[descriptorInfoCounter]  = (void*)(&(gpuPipelineM->refSceneUniformData()->refBufferInfo()));
		vectorDescriptorCount[descriptorInfoCounter] = 1;
		descriptorInfoCounter++;
		vectorDescriptorInfoHint.push_back(0);
	}

	if (m_resourcesUsed & MaterialBufferResource::MBR_CAMERA)
	{
		vectorDescriptorInfo[descriptorInfoCounter]  = (void*)(&(gpuPipelineM->refSceneCameraUniformData()->refBufferInfo()));
		vectorDescriptorCount[descriptorInfoCounter] = 1;
		descriptorInfoCounter++;
		vectorDescriptorInfoHint.push_back(0);
	}

	if (m_resourcesUsed & MaterialBufferResource::MBR_MATERIAL)
	{
		vectorDescriptorInfo[descriptorInfoCounter]  = (void*)(&(materialM->refMaterialUniformData()->refBufferInfo()));
		vectorDescriptorCount[descriptorInfoCounter] = 1;
		descriptorInfoCounter++;
		vectorDescriptorInfoHint.push_back(0);
	}
	
	forI(numResourcesUsed)
	{
		vectorBindingIndex.push_back(i);
	}

	uint numImageDescriptorInfo = 0;
	vector<VkDescriptorImageInfo> vectorDescriptorImageInfo;
	numImageDescriptorInfo += uint(m_shader->getVecTextureSampler().size());
	numImageDescriptorInfo += uint(m_shader->getVecImageSampler().size());
	vectorDescriptorImageInfo.resize(numImageDescriptorInfo);
	uint imageDescriptorInfoCounter = 0;

	VkDescriptorImageInfo descriptorImageInfo;
	const vectorSamplerPtr& arraySampler = m_shader->getVecTextureSampler();

	// Vector of pointer to those textures which need a layout change for the descriptor set to be built, this is due to textures used as images
	// to store information and later sampled as textures. At material buid time, the texture can have a specific layout not compatible with the descriptor set
	// to build for a spceific material
	vectorTexturePtr vectorTextureLayoutChange;
	// Vector to store the original layout of the texture so it can be restored
	vectorImageLayout vectorTextureLayoutChangeOriginalLayout;
	// Vector to store the final layout for each texture, in this case all VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
	vectorImageLayout vectorTextureLayoutChangeFinalLayout;

	forIT(arraySampler)
	{
		if ((*it)->getSamplerType() != ResourceInternalType::RIT_COMBINED_IMAGE_SAMPLER)
		{
			Texture* texture = (*it)->getVectorTexture()[0];
			if (texture->getImageLayout() != VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
			{
				vectorTextureLayoutChange.push_back(texture);
				vectorTextureLayoutChangeOriginalLayout.push_back(texture->getImageLayout());
				vectorTextureLayoutChangeFinalLayout.push_back(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
			}
		}
	}

	// Change the layout of those textures with VkImage in a layout different from the expected one, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
	if (vectorTextureLayoutChange.size() > 0)
	{
		textureM->changeTextureLayout(vectorTextureLayoutChange, vectorTextureLayoutChangeOriginalLayout, vectorTextureLayoutChangeFinalLayout, m_shader->getIsCompute());
	}

	forIT(arraySampler)
	{
		if ((*it)->getSamplerType() == ResourceInternalType::RIT_COMBINED_IMAGE_SAMPLER)
		{
			// Non uniform qualifier texture arrays using extension GL_EXT_nonuniform_qualifier just build an array of the textures they hold
			int a = 0;
			(*it)->buildDescriptorImageInfoVector();

			m_arrayDescriptorSetLayoutIsShared.push_back(false);
			vectorDescriptorImageInfo[imageDescriptorInfoCounter] = {};
			imageDescriptorInfoCounter++;

			vectorDescriptorType.push_back((*it)->getDescriptorType()); // Has to be VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER
			vectorBindingIndex.push_back((*it)->getBindingIndex());
			vectorStageFlags.push_back((*it)->getShaderStage());

			vectorDescriptorInfo[descriptorInfoCounter]  = (void*)(&(*it)->refVectorDescriptorImageInfo());
			vectorDescriptorCount[descriptorInfoCounter] = (*it)->refVectorDescriptorImageInfo().size();
			descriptorInfoCounter++;
			vectorDescriptorInfoHint.push_back(4);
		}
		else
		{
			Texture* texture = (*it)->getVectorTexture()[0];
			m_arrayDescriptorSetLayoutIsShared.push_back(false);
			descriptorImageInfo = { (*it)->getSamplerHandle(), texture->getView(), texture->getImageLayout() };
			vectorDescriptorImageInfo[imageDescriptorInfoCounter] = descriptorImageInfo;
			imageDescriptorInfoCounter++;

			vectorDescriptorType.push_back((*it)->getDescriptorType());
			vectorBindingIndex.push_back((*it)->getBindingIndex());
			vectorStageFlags.push_back((*it)->getShaderStage());

			vectorDescriptorInfo[descriptorInfoCounter] = (void*)(&vectorDescriptorImageInfo[imageDescriptorInfoCounter - 1]);
			vectorDescriptorCount[descriptorInfoCounter] = 1;
			descriptorInfoCounter++;
			vectorDescriptorInfoHint.push_back(1);
		}
	}

	const vectorSamplerPtr& arrayImage = m_shader->getVecImageSampler();

	// Vector of pointer to those textures which need a layout change for the descriptor set to be built, this can be due to textures used as images
	// to store information and later sampled as textures. At material buid time, the texture can have a specific layout not compatible with the descriptor set
	// to build for a spceific material
	vectorTexturePtr vectorImageLayoutChange;
	// Vector to store the original layout of the image so it can be restored
	vectorImageLayout vectorImageLayoutChangeOriginalLayout;
	// Vector to store the final layout for each image, in this case all VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
	vectorImageLayout vectorImageLayoutChangeFinalLayout;

	forIT(arrayImage)
	{
		Texture* texture = (*it)->getVectorTexture()[0];
		if (texture->getImageLayout() != VK_IMAGE_LAYOUT_GENERAL)
		{
			vectorImageLayoutChange.push_back(texture);
			vectorImageLayoutChangeOriginalLayout.push_back(texture->getImageLayout());
			vectorImageLayoutChangeFinalLayout.push_back(VK_IMAGE_LAYOUT_GENERAL);
		}
	}

	// Change the layout of those images with VkImage in a layout different from the expected one, VK_IMAGE_LAYOUT_GENERAL
	if (vectorImageLayoutChange.size() > 0)
	{
		textureM->changeTextureLayout(vectorImageLayoutChange, vectorImageLayoutChangeOriginalLayout, vectorImageLayoutChangeFinalLayout, m_shader->getIsCompute());
	}

	forIT(arrayImage)
	{
		Texture* texture = (*it)->getVectorTexture()[0];
		m_arrayDescriptorSetLayoutIsShared.push_back(false);
		descriptorImageInfo = { (*it)->getSamplerHandle(), texture->getView(), texture->getImageLayout() };
		vectorDescriptorImageInfo[imageDescriptorInfoCounter] = descriptorImageInfo;
		imageDescriptorInfoCounter++;

		vectorDescriptorType.push_back((*it)->getDescriptorType());
		vectorBindingIndex.push_back((*it)->getBindingIndex());
		vectorStageFlags.push_back((*it)->getShaderStage());

		vectorDescriptorInfo[descriptorInfoCounter]  = (void*)(&vectorDescriptorImageInfo[imageDescriptorInfoCounter - 1]);
		vectorDescriptorCount[descriptorInfoCounter] = 1;
		descriptorInfoCounter++;
		vectorDescriptorInfoHint.push_back(1);
	}

	const vectorShaderStorageBufferPtr& arrayShaderStorageBuffer = m_shader->getVectorShaderStorageBuffer();
	forIT(arrayShaderStorageBuffer)
	{
		m_arrayDescriptorSetLayoutIsShared.push_back(false);
		vectorDescriptorType.push_back((*it)->getDescriptorType());
		vectorBindingIndex.push_back((*it)->getBindingIndex());
		vectorStageFlags.push_back((*it)->getShaderStage());

		(*it)->buildDescriptorBufferInfoVector();
		vectorDescriptorInfo[descriptorInfoCounter]  = static_cast<void*>((*it)->refVectorDescriptorBufferInfo().data());
		vectorDescriptorCount[descriptorInfoCounter] = static_cast<int>((*it)->refVectorBufferPtr().size());
		descriptorInfoCounter++;
		vectorDescriptorInfoHint.push_back(0);
	}

	const vectorShaderTopLevelAccelerationStructurePtr& vectorShaderTopLevelAccelerationStructure = m_shader->getVectorTLAS();
	forIT(vectorShaderTopLevelAccelerationStructure)
	{
		m_arrayDescriptorSetLayoutIsShared.push_back(false);
		vectorDescriptorType.push_back((*it)->getDescriptorType());
		vectorBindingIndex.push_back((*it)->getBindingIndex());
		vectorStageFlags.push_back((*it)->getShaderStage());

		(*it)->buildWriteDescriptorSetAccelerationStructure();
		vectorDescriptorInfo[descriptorInfoCounter]  = static_cast<void*>((*it)->refWriteDescriptorSetAccelerationStructure());
		vectorDescriptorCount[descriptorInfoCounter] = 1;
		descriptorInfoCounter++;
		vectorDescriptorInfoHint.push_back(3);
	}

	m_descriptorSet = gpuPipelineM->buildDescriptorSet(
		vectorDescriptorType,
		vectorBindingIndex,
		vectorStageFlags,
		vectorDescriptorCount,
		m_descriptorSetLayout,
		m_descriptorPool);

	gpuPipelineM->updateDescriptorSet(
		m_descriptorSet,
		vectorDescriptorType,
		vectorDescriptorInfo,
		vectorDescriptorInfoHint,
		vectorDescriptorCount,
		vectorBindingIndex);

	// Create the pipeline layout with the help of descriptor layout.
	VkPipelineLayoutCreateInfo pPipelineLayoutCreateInfo = {};
	pPipelineLayoutCreateInfo.sType                  = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pPipelineLayoutCreateInfo.pNext                  = NULL;
	pPipelineLayoutCreateInfo.pushConstantRangeCount = 0;
	pPipelineLayoutCreateInfo.pPushConstantRanges    = NULL;
	pPipelineLayoutCreateInfo.setLayoutCount         = 1;
	pPipelineLayoutCreateInfo.pSetLayouts            = &m_descriptorSetLayout;

	// If a push constant was declared and has at least one element, add to the pipeline
	// NOTE: for now, only one push constant range will be used
	VkPushConstantRange pushConstantRange{};
	if (m_shader->refPushConstant().refVecUniformBase().size() > 0)
	{
		pushConstantRange.stageFlags                     = m_shader->refPushConstant().getShaderStages();
		pushConstantRange.offset                         = 0;
		pushConstantRange.size                           = m_pushConstantExposedStructFieldSize;
		pPipelineLayoutCreateInfo.pushConstantRangeCount = 1;
		pPipelineLayoutCreateInfo.pPushConstantRanges    = &pushConstantRange;
	}

	VkResult result;
	result = vkCreatePipelineLayout(coreM->getLogicalDevice(), &pPipelineLayoutCreateInfo, NULL, &m_pipelineLayout);
	assert(result == VK_SUCCESS);

	// TODOs:
	//	+ Remove the m_isCompute and use m_shaderStage and the corresponding stage flag
	//  + Add the case where a shader descriptor is for uniform buffer but has an array of buffers instead of a single element
	//  + Add the case for the descriptor set for acceleration structure
	//  + Add a ray tracing pipeline path

	if (m_isCompute)
	{
		VkComputePipelineCreateInfo computePipelineCreateInfo;
		computePipelineCreateInfo.sType              = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
		computePipelineCreateInfo.pNext              = nullptr;
		computePipelineCreateInfo.flags              = 0;
		computePipelineCreateInfo.stage              = m_shader->refArrayShaderStages()[0];
		computePipelineCreateInfo.layout             = m_pipelineLayout;
		computePipelineCreateInfo.basePipelineHandle = nullptr;
		computePipelineCreateInfo.basePipelineIndex  = 0;
		
		result = vkCreateComputePipelines(coreM->getLogicalDevice(), gpuPipelineM->getPipelineCache(), 1, &computePipelineCreateInfo, nullptr, &m_pipeline.refPipeline());
	}
	else if (m_isRayTracing)
	{
		// Pending: Unify the three shaders in a single material, buld the shader groups in the shader and verify the
		//          shader stages provided (refArrayShaderStages) are correct.
		VkRayTracingPipelineCreateInfoKHR rayTracingPipelineCreateInfo = { VK_STRUCTURE_TYPE_RAY_TRACING_PIPELINE_CREATE_INFO_KHR };
		rayTracingPipelineCreateInfo.stageCount                   = 3;
		rayTracingPipelineCreateInfo.maxPipelineRayRecursionDepth = 2;
		rayTracingPipelineCreateInfo.layout                       = m_pipelineLayout;
		rayTracingPipelineCreateInfo.stageCount                   = m_shader->refArrayShaderStages().size();
		rayTracingPipelineCreateInfo.pStages                      = m_shader->refArrayShaderStages().data();
		rayTracingPipelineCreateInfo.groupCount                   = m_shader->refVectorRayTracingShaderGroupCreateInfo().size();
		rayTracingPipelineCreateInfo.pGroups                      = m_shader->refVectorRayTracingShaderGroupCreateInfo().data();

		result = vkfpM->vkCreateRayTracingPipelinesKHR(coreM->getLogicalDevice(), 0, nullptr, 1, &rayTracingPipelineCreateInfo, nullptr, &m_pipeline.refPipeline());
	}
	else
	{
		m_pipeline.setPipelineShaderStage(m_shader->refArrayShaderStages());
		m_pipeline.setPipelineLayout(m_pipelineLayout);

		result = vkCreateGraphicsPipelines(coreM->getLogicalDevice(), gpuPipelineM->getPipelineCache(), 1, &m_pipeline.refPipelineData().getPipelineInfo(), nullptr, &m_pipeline.refPipeline());
	}
	assert(result == VK_SUCCESS);

	// Restore the layout of those textures with VkImage in a layout different from the expected one, now changed to VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
	if (vectorTextureLayoutChange.size() > 0)
	{
		textureM->changeTextureLayout(vectorTextureLayoutChange, vectorTextureLayoutChangeFinalLayout, vectorTextureLayoutChangeOriginalLayout, m_shader->getIsCompute());
	}

	// Restore the layout of those images with VkImage in a layout different from the expected one, now changed to VK_IMAGE_LAYOUT_GENERAL
	if (vectorImageLayoutChange.size() > 0)
	{
		textureM->changeTextureLayout(vectorImageLayoutChange, vectorImageLayoutChangeFinalLayout, vectorImageLayoutChangeOriginalLayout, m_shader->getIsCompute());
	}
}

/////////////////////////////////////////////////////////////////////////////////////////////

void Material::update(float dt)
{

}

/////////////////////////////////////////////////////////////////////////////////////////////

void Material::updateExposedResources()
{
	m_vectorDirtyExposedStructField.clear();

	forIT(m_vectorExposedStructField)
	{
		if (ShaderReflection::setResourceCPUValue((*it)))
		{
			addIfNoPresent(&(*it), m_vectorDirtyExposedStructField);
		}
	}

	m_exposedStructFieldDirty = (m_vectorDirtyExposedStructField.size() > 0);
}

/////////////////////////////////////////////////////////////////////////////////////////////

void Material::updatePushConstantExposedResources()
{
	m_vectorPushConstantDirtyExposedStructField.clear();

	forIT(m_vectorPushConstantExposedStructField)
	{
		if (ShaderReflection::setResourceCPUValue((*it)))
		{
			addIfNoPresent(&(*it), m_vectorPushConstantDirtyExposedStructField);
		}
	}

	m_pushConstantExposedStructFieldDirty = (m_vectorPushConstantDirtyExposedStructField.size() > 0);
}

/////////////////////////////////////////////////////////////////////////////////////////////

bool Material::loadShader()
{
	return false;
}

/////////////////////////////////////////////////////////////////////////////////////////////

void Material::buildMaterialResources()
{
	exposeResources();
	m_exposedStructFieldSize             = computeExposedStructFieldSize(m_vectorExposedStructField);
	m_pushConstantExposedStructFieldSize = computeExposedStructFieldSize(m_vectorPushConstantExposedStructField);
	m_shader->initializeSamplerHandlers();
	m_shader->initializeImageHandlers();
	if (!matchExposedResources())
	{
		cout << "ERROR: Material::matchExposedResources returned false" << endl;
	}
	setupPipelineData();
}

/////////////////////////////////////////////////////////////////////////////////////////////

void Material::loadResources()
{

}

/////////////////////////////////////////////////////////////////////////////////////////////

void Material::exposeResources()
{

}

/////////////////////////////////////////////////////////////////////////////////////////////

void Material::setupPipelineData()
{

}

/////////////////////////////////////////////////////////////////////////////////////////////

bool Material::exposeStructField(ResourceInternalType internalType, void* data, string&& structName, string&& structFieldName)
{
	if ((data == nullptr) || !ShaderReflection::isDataType(internalType))
	{
		return false;
	}

	m_vectorExposedStructField.push_back(ExposedStructField(internalType, move(structName), move(structFieldName), data));
	return true;
}

/////////////////////////////////////////////////////////////////////////////////////////////

bool Material::pushConstantExposeStructField(ResourceInternalType internalType, void* data, string&& structName, string&& structFieldName)
{
	if ((data == nullptr) || !ShaderReflection::isDataType(internalType))
	{
		return false;
	}

	m_vectorPushConstantExposedStructField.push_back(ExposedStructField(internalType, move(structName), move(structFieldName), data));
	return true;
}

/////////////////////////////////////////////////////////////////////////////////////////////

bool Material::assignTextureToSampler(string&& textureSamplerName, string&& textureResourceName, VkDescriptorType descriptorType)
{
	return assignTextureToSampler(move(textureSamplerName), move(textureResourceName), descriptorType, VkFilter::VK_FILTER_MAX_ENUM);
}

/////////////////////////////////////////////////////////////////////////////////////////////

bool Material::assignTextureToSampler(string&& textureSamplerName, string&& textureResourceName, VkDescriptorType descriptorType, VkFilter filter)
{
	return m_shader->setTextureToSample(move(textureSamplerName), move(textureResourceName), descriptorType, filter);
}

/////////////////////////////////////////////////////////////////////////////////////////////

bool Material::assignImageToSampler(string&& imageSamplerName, string&& textureResourceName, VkDescriptorType descriptorType)
{
	return m_shader->setImageToSample(move(imageSamplerName), move(textureResourceName), descriptorType, VkFilter::VK_FILTER_MAX_ENUM);
}

/////////////////////////////////////////////////////////////////////////////////////////////

bool Material::assignImageToSampler(string&& imageSamplerName, string&& textureResourceName, VkDescriptorType descriptorType, VkFilter filter)
{
	return m_shader->setImageToSample(move(imageSamplerName), move(textureResourceName), descriptorType, filter);
}

/////////////////////////////////////////////////////////////////////////////////////////////

bool Material::assignCombinedImageSampler(string&& combinedImageSamplerName, vector<string>&& vectorTextureResourceName)
{
	return m_shader->setCombinedImageSampler(move(combinedImageSamplerName), move(vectorTextureResourceName));
}

/////////////////////////////////////////////////////////////////////////////////////////////

bool Material::assignShaderStorageBuffer(string &&storageBufferName, string&& bufferResourceName, VkDescriptorType descriptorType)
{
	return m_shader->setShaderStorageBuffer(move(storageBufferName), move(bufferResourceName), descriptorType);
}

/////////////////////////////////////////////////////////////////////////////////////////////

bool Material::assignShaderStorageBuffer(string&& storageBufferName, vector<string>&& vectorBufferResourceName, VkDescriptorType descriptorType)
{
	return m_shader->setShaderStorageBuffer(move(storageBufferName), move(vectorBufferResourceName), descriptorType);
}

/////////////////////////////////////////////////////////////////////////////////////////////

bool Material::assignAccelerationStructure(string&& accelerationStructureName, string&& accelerationStructureResourceName, VkDescriptorType descriptorType)
{
	return m_shader->setAccelerationStructureToRayTrace(move(accelerationStructureName), move(accelerationStructureResourceName), descriptorType);
}

/////////////////////////////////////////////////////////////////////////////////////////////

void Material::writeExposedDataToMaterialUB() const
{
	if (m_materialUniformBufferIndex == -1)
	{
		cout << "ERROR: m_materialUniformBufferIndex is -1 in Material::writeExposedDataToMaterialUB" << endl;
		return;
	}

	UniformBuffer* ubo = materialM->refMaterialUniformData();
	ubo->refCPUBuffer().resetDataAtCell(m_materialUniformBufferIndex);

	forIT(m_vectorExposedStructField)
	{
		ShaderReflection::appendExposedStructFieldDataToUniformBufferCell(&ubo->refCPUBuffer(), m_materialUniformBufferIndex, &(*it));
	}
}

/////////////////////////////////////////////////////////////////////////////////////////////

void Material::pushConstantWriteExposedDataToCPUBuffer() const
{
	CPUBuffer* cpuBuffer = &m_shader->refPushConstant().refCPUBuffer();

	int index;
	forIT(m_vectorPushConstantExposedStructField)
	{
		index = getPushConstantExposedResourceIndex(it->getInternalType(), it->getStructName(), it->getStructFieldName());
		cpuBuffer->resetDataAtCell(index);
		ShaderReflection::appendExposedStructFieldDataToUniformBufferCell(cpuBuffer, index, &(*it));
	}
}

/////////////////////////////////////////////////////////////////////////////////////////////

int Material::computeExposedStructFieldSize(const vector<ExposedStructField>& vectorExposed)
{
	int result = 0;

	forIT(vectorExposed)
	{
		result += resourceenum::getResourceInternalTypeSizeInBytes(it->getInternalType());
	}

	return result;
}

/////////////////////////////////////////////////////////////////////////////////////////////

void Material::destroyDescriptorLayout()
{
	for (int i = 0; i < m_arrayDescriptorSetLayout.size(); i++)
	{
		if (!m_arrayDescriptorSetLayoutIsShared[i])
		{
			vkDestroyDescriptorSetLayout(coreM->getLogicalDevice(), m_arrayDescriptorSetLayout[i], NULL);
		}
	}

	m_arrayDescriptorSetLayoutIsShared.clear();
	m_arrayDescriptorSetLayout.clear();
}

/////////////////////////////////////////////////////////////////////////////////////////////

void Material::destroyPipelineLayouts()
{
	if (m_pipelineLayout != VK_NULL_HANDLE)
	{
		vkDestroyPipelineLayout(coreM->getLogicalDevice(), m_pipelineLayout, NULL);

		m_pipelineLayout = VK_NULL_HANDLE;
	}
}

/////////////////////////////////////////////////////////////////////////////////////////////

void Material::destroyDescriptorPool()
{
	forIT(m_vectorTextureDescriptorPool)
	{
		vkDestroyDescriptorPool(coreM->getLogicalDevice(), (*it), NULL);
	}

	forIT(m_vectorImageDescriptorPool)
	{
		vkDestroyDescriptorPool(coreM->getLogicalDevice(), (*it), NULL);
	}

	forIT(m_vectorShaderStorageDescriptorPool)
	{
		vkDestroyDescriptorPool(coreM->getLogicalDevice(), (*it), NULL);
	}

	m_vectorTextureDescriptorPool.clear();
	m_vectorImageDescriptorPool.clear();
	m_vectorShaderStorageDescriptorPool.clear();
}

/////////////////////////////////////////////////////////////////////////////////////////////

void Material::destroyDescriptorSet()
{
	VkResult result;

	forI(m_vectorTextureDescriptorSet.size())
	{
		result = vkFreeDescriptorSets(coreM->getLogicalDevice(), m_vectorTextureDescriptorPool[i], (uint32_t)1, &m_vectorTextureDescriptorSet[i]);
		assert(result);
	}

	forI(m_vectorImageDescriptorSet.size())
	{
		result = vkFreeDescriptorSets(coreM->getLogicalDevice(), m_vectorImageDescriptorPool[i], (uint32_t)1, &m_vectorImageDescriptorSet[i]);
		assert(result);
	}

	forI(m_vectorShaderStorageDescriptorSet.size())
	{
		result = vkFreeDescriptorSets(coreM->getLogicalDevice(), m_vectorShaderStorageDescriptorPool[i], (uint32_t)1, &m_vectorShaderStorageDescriptorSet[i]);
		assert(result);
	}

	m_vectorTextureDescriptorSet.clear();
	m_vectorImageDescriptorSet.clear();
	m_vectorShaderStorageDescriptorSet.clear();
}

/////////////////////////////////////////////////////////////////////////////////////////////

void Material::bindTextureSamplers(VkCommandBuffer* commandBuffer) const
{
	const vectorSamplerPtr& arraySampler = m_shader->getVecTextureSampler();
	forI(arraySampler.size())
	{
		vkCmdBindDescriptorSets(*commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipelineLayout, arraySampler[i]->getSetIndex(), 1, &(m_vectorTextureDescriptorSet[i]), 0, nullptr);
	}
}

/////////////////////////////////////////////////////////////////////////////////////////////

void Material::bindImageSamplers(VkCommandBuffer* commandBuffer) const
{
	const vectorSamplerPtr& arrayImage = m_shader->getVecImageSampler();
	forI(arrayImage.size())
	{
		vkCmdBindDescriptorSets(*commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipelineLayout, arrayImage[i]->getSetIndex(), 1, &(m_vectorImageDescriptorSet[i]), 0, nullptr);
	}
}

/////////////////////////////////////////////////////////////////////////////////////////////

void Material::bindShaderStorageBuffers(VkCommandBuffer* commandBuffer) const
{
	const vectorShaderStorageBufferPtr& arrayShaderStorageBuffer = m_shader->getVectorShaderStorageBuffer();
	forI(arrayShaderStorageBuffer.size())
	{
		vkCmdBindDescriptorSets(*commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipelineLayout, arrayShaderStorageBuffer[i]->getSetIndex(), 1, &(m_vectorShaderStorageDescriptorSet[i]), 0, nullptr);
	}
}

/////////////////////////////////////////////////////////////////////////////////////////////

void Material::updatePushConstantCPUBuffer()
{
	updatePushConstantExposedResources();

	if(m_pushConstantExposedStructFieldDirty)
	{
		pushConstantWriteExposedDataToCPUBuffer();
		m_pushConstantExposedStructFieldDirty = false;
	}
}

/////////////////////////////////////////////////////////////////////////////////////////////

int Material::getPushConstantExposedResourceIndex(ResourceInternalType internalType, string structName, string structFieldName) const
{
	const ExposedStructField* structField;
	forI(m_vectorPushConstantExposedStructField.size())
	{
		structField = &m_vectorPushConstantExposedStructField[i];
		if ((structField->getInternalType() == internalType) && (structField->getStructName() == structName) && (structField->getStructFieldName() == structFieldName))
		{
			return i;
		}
	}

	return -1;
}

/////////////////////////////////////////////////////////////////////////////////////////////

void Material::destroyDescriptorSetResource()
{
	VkResult result = vkFreeDescriptorSets(coreM->getLogicalDevice(), m_descriptorPool, (uint32_t)1, &m_descriptorSet);
	assert(result == VK_SUCCESS);

	vkDestroyDescriptorPool(coreM->getLogicalDevice(), m_descriptorPool, nullptr);
	vkDestroyDescriptorSetLayout(coreM->getLogicalDevice(), m_descriptorSetLayout, nullptr);
}

/////////////////////////////////////////////////////////////////////////////////////////////

void Material::destroyPipelineResource()
{
	m_pipeline.destroyResources();
}

/////////////////////////////////////////////////////////////////////////////////////////////

void Material::destroyMaterialResources()
{
	destroyPipelineResource();
	destroyDescriptorSetResource();
	unmatchExposedResources();
	destroyDescriptorLayout();
	destroyPipelineLayouts();
	destroyDescriptorSet();
	destroyDescriptorPool();
	m_pipeline.destroyResources();
	//deleteVectorInstances(m_vectorDirtyExposedStructField);
	//deleteVectorInstances(m_vectorPushConstantDirtyExposedStructField);
	clearExposedFieldData();
}

/////////////////////////////////////////////////////////////////////////////////////////////

void Material::clearExposedFieldData()
{
	m_vectorExposedStructField.clear();
	m_vectorDirtyExposedStructField.clear();
	m_vectorPushConstantExposedStructField.clear();
	m_vectorPushConstantDirtyExposedStructField.clear();
}

/////////////////////////////////////////////////////////////////////////////////////////////

bool Material::shaderResourceNotification(string&& shaderResourceName, ManagerNotificationType notificationType)
{
	if (shaderResourceName == m_shaderResourceName)
	{
		switch (notificationType)
		{
			case ManagerNotificationType::MNT_REMOVED:
			{
				m_shader = nullptr;
				setReady(false);
				return true;
			}
			case ManagerNotificationType::MNT_ADDED:
			{
				m_shader = shaderM->getElement(move(shaderResourceName));
				return true;
			}
			case ManagerNotificationType::MNT_CHANGED:
			{
				if (getReady())
				{
					setReady(false);
				}
				return true;
			}
		}
	}

	return false;
}

/////////////////////////////////////////////////////////////////////////////////////////////

uint Material::getNumDynamicUniformBufferResourceUsed()
{
	int result = 0;

	if (m_resourcesUsed & MaterialBufferResource::MBR_NONE)
	{
		return 0;
	}

	if (m_resourcesUsed & MaterialBufferResource::MBR_MODEL)
	{
		result++;
	}

	if (m_resourcesUsed & MaterialBufferResource::MBR_CAMERA)
	{
		result++;
	}

	if (m_resourcesUsed & MaterialBufferResource::MBR_MATERIAL)
	{
		result++;
	}

	return result;
}

/////////////////////////////////////////////////////////////////////////////////////////////
