/*
Copyright 2017 Alejandro Cosin

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
#include "../../include/core/gpupipeline.h"
#include "../../include/core/coremanager.h"
#include "../../include/texture/texturemanager.h"
#include "../../include/buffer/buffer.h"
#include "../../include/buffer/buffermanager.h"
#include "../../include/shader/shadermanager.h"
#include "../../include/shader/shader.h"
#include "../../include/scene/scene.h"
#include "../../include/pipeline/pipeline.h"
#include "../../include/uniformbuffer/uniformbuffermanager.h"
#include "../../include/uniformbuffer/uniformbuffer.h"
#include "../../include/material/materialmanager.h"
#include "../../include/material/material.h"
#include "../../include/rastertechnique/rastertechnique.h"
#include "../../include/rastertechnique/rastertechniquemanager.h"
#include "../../include/camera/cameramanager.h"
#include "../../include/parameter/attributedefines.h"
#include "../../include/parameter/attributedata.h"
#include "../../include/renderpass/renderpassmanager.h"
#include "../../include/framebuffer/framebuffermanager.h"
#include "../../include/component/basecomponent.h"
#include "../../include/component/rendercomponent.h"
#include "../../include/component/skinnedmeshrendercomponent.h"

// NAMESPACE
using namespace attributedefines;

// DEFINES
const uint perVertexNumElement = 24;

// STATIC MEMBER INITIALIZATION

/////////////////////////////////////////////////////////////////////////////////////////////

GPUPipeline::GPUPipeline():
	  m_pipelineInitialized(false)
	, m_vertexStrideBytes(24)
{

}

/////////////////////////////////////////////////////////////////////////////////////////////

GPUPipeline::~GPUPipeline()
{

}

/////////////////////////////////////////////////////////////////////////////////////////////

void GPUPipeline::init()
{
	shaderM->obtainMaxPushConstantsSize();

	vectorUint arrayIndices;
	vectorUint8 arrayVertexData;

	vectorNodePtr vector3DModel = sceneM->getByNodeType(eNodeType::E_NT_STATIC_ELEMENT | eNodeType::E_NT_DYNAMIC_ELEMENT);

	buildSceneBufferData(vector3DModel, arrayIndices, arrayVertexData);

	createVertexBuffer(arrayIndices, "indexBuffer", arrayVertexData, "vertexBuffer");

	arrayIndices.clear();
	arrayVertexData.clear();
	vector3DModel = sceneM->getByNodeType(eNodeType::E_NT_SKINNEDMESH_ELEMENT); // Skinned meshes

	if (vector3DModel.size() > 0)
	{
		buildSceneBufferData(vector3DModel, arrayIndices, arrayVertexData);
		createVertexBuffer(arrayIndices, "indexBufferSkinnedMesh", arrayVertexData, "vertexBufferSkinnedMesh");

		// Build a secondary buffer to use as initial data for compute skeletal mesh update
		Buffer* bufferBufferSkinnedMeshOriginal = bufferM->buildBuffer(
			move(string("vertexBufferSkinnedMeshOriginal")),
			(void*)arrayVertexData.data(),
			sizeof(uint8_t) * arrayVertexData.size(),
			VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT| VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

		vectorFloat vectorSkinnedMeshData;
		buildSkeletalMeshBufferData(vector3DModel, vectorSkinnedMeshData);

		// Buffer with the bone ID and the bone weight data for each vertex in vertexBufferSkinnedMeshOriginal and vertexBufferSkinnedMesh,
		// currently up to 4 bones and weights are implemented per vertex, the information in this buffer is 8 float values per vertex data in vertexBufferSkinnedMesh and
		// vertexBufferSkinnedMeshOriginal with the first four being bone ID information and the remaining four being weight information
		Buffer* bufferSkinnedMeshData = bufferM->buildBuffer(
			move(string("skinnedMeshDataBuffer")),
			(void*)vectorSkinnedMeshData.data(),
			sizeof(float) * vectorSkinnedMeshData.size(),
			VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT| VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
	}

	setVertexInput(m_vertexStrideBytes); // Size in bytes of each vertex information. Currently: position, uv + ID, normal and tangent (vec3, vec3, vec3, vec3), 48 bytes
	
	// Create the vertex and fragment shader
	createPipelineCache();
	createSceneDataUniformBuffer();
	createSceneCameraUniformBuffer();

	m_vectorRasterTechnique.push_back(rasterTechniqueM->buildNewRasterTechnique(string("StaticSceneVoxelizationTechnique"), string("StaticSceneVoxelizationTechnique"), nullptr));
	if (vector3DModel.size() > 0)
	{
		m_vectorRasterTechnique.push_back(rasterTechniqueM->buildNewRasterTechnique(string("SkeletalAnimationUpdateTechnique"), string("SkeletalAnimationUpdateTechnique"), nullptr));
	}
	m_vectorRasterTechnique.push_back(rasterTechniqueM->buildNewRasterTechnique(string("ComputeFrustumCullingTechnique"),           string("ComputeFrustumCullingTechnique"),           nullptr));
	m_vectorRasterTechnique.push_back(rasterTechniqueM->buildNewRasterTechnique(string("SceneIndirectDrawTechnique"),               string("SceneIndirectDrawTechnique"),               nullptr));
	m_vectorRasterTechnique.push_back(rasterTechniqueM->buildNewRasterTechnique(string("BufferPrefixSumTechnique"),                 string("BufferPrefixSumTechnique"),                 nullptr));
	m_vectorRasterTechnique.push_back(rasterTechniqueM->buildNewRasterTechnique(string("VoxelVisibilityRayTracingTechnique"),       string("VoxelVisibilityRayTracingTechnique"),       nullptr));
	m_vectorRasterTechnique.push_back(rasterTechniqueM->buildNewRasterTechnique(string("StaticNeighbourInformationTechnique"),      string("StaticNeighbourInformationTechnique"),      nullptr));
	m_vectorRasterTechnique.push_back(rasterTechniqueM->buildNewRasterTechnique(string("ResetLitvoxelData"),                         string("ResetLitvoxelData"),                        nullptr));
	m_vectorRasterTechnique.push_back(rasterTechniqueM->buildNewRasterTechnique(string("DynamicSceneVoxelizationTechnique"),        string("DynamicSceneVoxelizationTechnique"),        nullptr));
	m_vectorRasterTechnique.push_back(rasterTechniqueM->buildNewRasterTechnique(string("DynamicVoxelCopyToBufferTechnique"),        string("DynamicVoxelCopyToBufferTechnique"),        nullptr));
	m_vectorRasterTechnique.push_back(rasterTechniqueM->buildNewRasterTechnique(string("LitVoxelTechnique"),                         string("LitVoxelTechnique"),                        nullptr));
	m_vectorRasterTechnique.push_back(rasterTechniqueM->buildNewRasterTechnique(string("GBufferTechnique"),                          string("GBufferTechnique"),                         nullptr));
	m_vectorRasterTechnique.push_back(rasterTechniqueM->buildNewRasterTechnique(string("RayTracingDeferredShadowsTechnique"),        string("RayTracingDeferredShadowsTechnique"),       nullptr));
	m_vectorRasterTechnique.push_back(rasterTechniqueM->buildNewRasterTechnique(string("ProcessCameraVisibleResultsTechnique"),      string("ProcessCameraVisibleResultsTechnique"),     nullptr));
	m_vectorRasterTechnique.push_back(rasterTechniqueM->buildNewRasterTechnique(string("DynamicVoxelVisibilityRayTracingTechnique"), string("DynamicVoxelVisibilityRayTracingTechnique"), nullptr));
	m_vectorRasterTechnique.push_back(rasterTechniqueM->buildNewRasterTechnique(string("LightBounceVoxelIrradianceTechnique"),       string("LightBounceVoxelIrradianceTechnique"),       nullptr));
	m_vectorRasterTechnique.push_back(rasterTechniqueM->buildNewRasterTechnique(string("DynamicLightBounceVoxelIrradianceTechnique"),string("DynamicLightBounceVoxelIrradianceTechnique"),nullptr));
	m_vectorRasterTechnique.push_back(rasterTechniqueM->buildNewRasterTechnique(string("LightBounceDynamicVoxelIrradianceTechnique"), string("LightBounceDynamicVoxelIrradianceTechnique"), nullptr));
	m_vectorRasterTechnique.push_back(rasterTechniqueM->buildNewRasterTechnique(string("IrradianceGaussianBlurTechnique"),           string("IrradianceGaussianBlurTechnique"),           nullptr));
	m_vectorRasterTechnique.push_back(rasterTechniqueM->buildNewRasterTechnique(string("SceneLightingDeferredTechnique"),            string("SceneLightingDeferredTechnique"),            nullptr));
	m_vectorRasterTechnique.push_back(rasterTechniqueM->buildNewRasterTechnique(string("VoxelRasterInScenarioTechnique"),            string("VoxelRasterInScenarioTechnique"),            nullptr));
	m_vectorRasterTechnique.push_back(rasterTechniqueM->buildNewRasterTechnique(string("AntialiasingTechnique"),                     string("AntialiasingTechnique"),                     nullptr));

	m_vectorReRecordFlags.resize(m_vectorRasterTechnique.size());
	fill(m_vectorReRecordFlags.begin(), m_vectorReRecordFlags.end(), false);

	// Inter-depencence in resources requires to init the materials after all other resources have been built (textures, buffers, etc)
	materialM->initAllMaterials();
	materialM->buildMaterialUniformBuffer();

	const vector<Material*>& vectorMaterial = materialM->getVectorElement();
	forI(vectorMaterial.size())
	{
		vectorMaterial[i]->buildPipeline();
	}

	forIT(m_vectorRasterTechnique)
	{
		(*it)->generateSempahore(); // TODO: Set as protected and call from other part of the framework
	}

	m_pipelineInitialized = true;
}

/////////////////////////////////////////////////////////////////////////////////////////////

void GPUPipeline::createVertexBuffer(vectorUint& arrayIndices, const string& indexBufferName, vectorUint8& arrayVertexData, const string& vertexBufferName)
{
	Buffer* buffer = bufferM->buildBuffer(
		move(string(vertexBufferName)),
		(void *)arrayVertexData.data(),
		sizeof(uint8_t) * arrayVertexData.size(),
		VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

	buffer = bufferM->buildBuffer(
		move(string(indexBufferName)),
		(void *)arrayIndices.data(),
		sizeof(uint) * arrayIndices.size(),
		VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
}

/////////////////////////////////////////////////////////////////////////////////////////////

void GPUPipeline::setVertexInput(uint32_t dataStride)
{
	// The VkVertexInputBinding viIpBind, stores the rate at which the information will be
	// injected for vertex input.
	m_viIpBind.binding   = 0;
	m_viIpBind.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
	m_viIpBind.stride    = dataStride;

	// The VkVertexInputAttribute - Description) structure, store 
	// the information that helps in interpreting the data.
	m_viIpAttrb[0].binding  = 0;
	m_viIpAttrb[0].location = 0;
	m_viIpAttrb[0].format   = VK_FORMAT_R32G32B32_SFLOAT;
	m_viIpAttrb[0].offset   = 0;

	m_viIpAttrb[1].binding  = 0;
	m_viIpAttrb[1].location = 1;
	m_viIpAttrb[1].format   = VK_FORMAT_R32G32B32_UINT;
	m_viIpAttrb[1].offset   = 12; // After, 4 components - RGB  each of 4 bytes
}

/////////////////////////////////////////////////////////////////////////////////////////////

void GPUPipeline::initViewports(float width, float height, float offsetX, float offsetY, float minDepth, float maxDepth, VkCommandBuffer* cmd)
{
	VkViewport m_viewport;
	m_viewport.width    = width;
	m_viewport.height   = height;
	m_viewport.minDepth = minDepth;
	m_viewport.maxDepth = maxDepth;
	m_viewport.x        = offsetX;
	m_viewport.y        = offsetY;
	vkCmdSetViewport(*cmd, 0, NUMBER_OF_VIEWPORTS, &m_viewport);
}

/////////////////////////////////////////////////////////////////////////////////////////////

void GPUPipeline::initScissors(uint width, uint height, int offsetX, int offsetY, VkCommandBuffer* cmd)
{
	VkRect2D m_scissor;
	m_scissor.extent.width  = width;
	m_scissor.extent.height = height;
	m_scissor.offset.x      = offsetX;
	m_scissor.offset.y      = offsetY;
	vkCmdSetScissor(*cmd, 0, NUMBER_OF_SCISSORS, &m_scissor);
}

/////////////////////////////////////////////////////////////////////////////////////////////

void GPUPipeline::update()
{
	const vectorInt& vectorNodeIndexFrameUpdated = sceneM->getVectorNodeIndexFrameUpdated();

	if (vectorNodeIndexFrameUpdated.size() > 0)
	{
		const vectorNodePtr& vectorNode = sceneM->getVectorNode();

		int numElement = int(vectorNodeIndexFrameUpdated.size());
		vector<VkMappedMemoryRange> vectorMappedRange(numElement);

		size_t dynamicAllignment    = m_sceneUniformData->getDynamicAllignment();
		VkDeviceMemory deviceMemory = m_sceneUniformData->getDeviceMemory();

		int index;
		mat4 modelMatrix;
		forI(numElement)
		{
			index       = vectorNodeIndexFrameUpdated[i];
			modelMatrix = vectorNode[index]->getModelMat();

			m_sceneUniformData->refCPUBuffer().resetDataAtCell(index);
			m_sceneUniformData->refCPUBuffer().appendDataAtCell<mat4>(index, modelMatrix);

			vectorMappedRange[i] = VkMappedMemoryRange{ VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE, nullptr, deviceMemory, index * dynamicAllignment, dynamicAllignment };
		}

		m_sceneUniformData->uploadCPUBufferToGPU(vectorMappedRange);
	}

	// Update scene camera information
	// TODO: Avoid if the camera did not changed
	mat4 viewMatrix       = cameraM->refMainCamera()->getView();
	mat4 projectionMatrix = cameraM->refMainCamera()->getProjection();
	BBox3D& sceneAABB     = sceneM->refBox();
	vec3 sceneOffset      = vec3(.0f) - sceneAABB.getMin();
	vec3 sceneExtent      = sceneAABB.getMax() - sceneAABB.getMin();

	m_sceneCameraUniformData->refCPUBuffer().resetDataAtCell(0);
	m_sceneCameraUniformData->refCPUBuffer().appendDataAtCell<mat4>(0, viewMatrix);
	m_sceneCameraUniformData->refCPUBuffer().appendDataAtCell<mat4>(0, projectionMatrix);
	m_sceneCameraUniformData->refCPUBuffer().appendDataAtCell<vec4>(0, vec4(sceneOffset.x, sceneOffset.y, sceneOffset.z, 0.0f));
	m_sceneCameraUniformData->refCPUBuffer().appendDataAtCell<vec4>(0, vec4(sceneExtent.x, sceneExtent.y, sceneExtent.z, sceneM->getDeltaTime()));
	m_sceneCameraUniformData->uploadCPUBufferToGPU();
}

/////////////////////////////////////////////////////////////////////////////////////////////

void GPUPipeline::buildSceneBufferData(const vectorNodePtr& vectorNode, vectorUint& arrayIndices, vectorUint8& arrayVertexData)
{
	// All mesh per vertex data

	forIT(vectorNode)
	{
		// TODO: Refactor to use directly a pointer to the render component

		// First, per vertex data stored in Node::m_vertexData
		RenderComponent* renderComponent = (*it)->refRenderComponent();
		uint uIndexOffset                = uint(arrayVertexData.size()) / perVertexNumElement;

		renderComponent->setStartIndex(uint(arrayIndices.size()));
		arrayVertexData.insert(arrayVertexData.end(), renderComponent->refVertexData().begin(), renderComponent->refVertexData().end());
		assert(renderComponent->refVertexData().size() != 0);

		// Add the offset to the indices being used, as all the indices of all the scene meshes will be in the same buffer
		forJT(renderComponent->refIndices())
		{
			(*jt) += uIndexOffset;
		}

		arrayIndices.insert(arrayIndices.end(), renderComponent->refIndices().begin(), renderComponent->refIndices().end());
		renderComponent->setEndIndex(uint(arrayIndices.size()));
		renderComponent->setIndexSize(uint(renderComponent->refIndices().size()));
	}
}

/////////////////////////////////////////////////////////////////////////////////////////////

void GPUPipeline::buildSkeletalMeshBufferData(const vectorNodePtr& vectorNode, vectorFloat& vectorData)
{
	int numElement = 0;
	vectorSkinnedMeshRenderComponentPtr vectorSkinnedMeshComponent;

	forI(vectorNode.size())
	{
		if ((vectorNode[i]->getRenderComponent() != nullptr) && (vectorNode[i]->getRenderComponent()->getResourceType() == GenericResourceType::GRT_SKINNEDMESHRENDERCOMPONENT))
		{
			vectorSkinnedMeshComponent.push_back(static_cast<SkinnedMeshRenderComponent*>(vectorNode[i]->refRenderComponent()));
			numElement += vectorSkinnedMeshComponent.back()->getBoneID().size() * 4 + vectorSkinnedMeshComponent.back()->getBoneWeight().size() * 4;
		}
	}

	if (vectorNode.size() != vectorSkinnedMeshComponent.size())
	{
		cout << "ERROR: Not same amount of nodes and skinned mesh components in GPUPipeline::buildSkeletalMeshBufferData" << endl;
	}

	vectorData.resize(numElement);

	int counterTemp = 0;
	forI(vectorSkinnedMeshComponent.size())
	{
		const vectorUVec4& vectorBoneID    = vectorSkinnedMeshComponent[i]->getBoneID();
		const vectorVec4& vectorBoneWeight = vectorSkinnedMeshComponent[i]->getBoneWeight();
		forJ(vectorBoneID.size())
		{
			vectorData[counterTemp++] = float(vectorBoneID[j].x);
			vectorData[counterTemp++] = float(vectorBoneID[j].y);
			vectorData[counterTemp++] = float(vectorBoneID[j].z);
			vectorData[counterTemp++] = float(vectorBoneID[j].w);
			vectorData[counterTemp++] = float(vectorBoneWeight[j].x);
			vectorData[counterTemp++] = float(vectorBoneWeight[j].y);
			vectorData[counterTemp++] = float(vectorBoneWeight[j].z);
			vectorData[counterTemp++] = float(vectorBoneWeight[j].w);
		}
	}
}

/////////////////////////////////////////////////////////////////////////////////////////////

void GPUPipeline::createPipelineCache()
{
	VkPipelineCacheCreateInfo pipelineCacheInfo;
	pipelineCacheInfo.sType           = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;
	pipelineCacheInfo.pNext           = nullptr;
	pipelineCacheInfo.initialDataSize = 0;
	pipelineCacheInfo.pInitialData    = nullptr;
	pipelineCacheInfo.flags           = 0;

	VkResult result = vkCreatePipelineCache(coreM->getLogicalDevice(), &pipelineCacheInfo, nullptr, &m_pipelineCache);
	assert(result == VK_SUCCESS);
}

/////////////////////////////////////////////////////////////////////////////////////////////

void GPUPipeline::createSceneDataUniformBuffer()
{
	vectorNodePtr arrayModel = sceneM->getByMeshType(eMeshType::E_MT_ALL);

	m_sceneUniformData = uniformBufferM->buildUniformBuffer(move(string("sceneUniformBuffer")), sizeof(mat4), static_cast<int>((arrayModel.size())));

	forI(arrayModel.size())
	{
		mat4 temp = arrayModel[i]->getModelMat();
		m_sceneUniformData->refCPUBuffer().appendDataAtCell<mat4>(i, arrayModel[i]->getModelMat());
	}

	m_sceneUniformData->uploadCPUBufferToGPU();
}

/////////////////////////////////////////////////////////////////////////////////////////////

void GPUPipeline::createSceneCameraUniformBuffer()
{
	m_sceneCameraUniformData = uniformBufferM->buildUniformBuffer(move(string("sceneCameraUniformBuffer")), sizeof(mat4) * 3, 1);
	m_sceneCameraUniformData->uploadCPUBufferToGPU();
}

/////////////////////////////////////////////////////////////////////////////////////////////

VkDescriptorSet GPUPipeline::buildDescriptorSet(vector<VkDescriptorType> vectorDescriptorType,
												vector<uint32_t>           vectorBindingIndex,
												vector<VkShaderStageFlags> vectorStageFlags,
												vectorInt&                 vectorDescriptorCount,
												VkDescriptorSetLayout&     descriptorSetLayout,
												VkDescriptorPool&          descriptorPool)
{
	vector<VkDescriptorSetLayoutBinding> vectorDescriptorSetLayoutBinding;

	forI(vectorDescriptorType.size())
	{
		VkDescriptorSetLayoutBinding descriptorSetLayoutBinding;
		descriptorSetLayoutBinding.binding            = vectorBindingIndex[i];
		descriptorSetLayoutBinding.descriptorType     = vectorDescriptorType[i];
		descriptorSetLayoutBinding.descriptorCount    = vectorDescriptorCount[i];
		descriptorSetLayoutBinding.stageFlags         = vectorStageFlags[i];
		descriptorSetLayoutBinding.pImmutableSamplers = nullptr;

		vectorDescriptorSetLayoutBinding.push_back(descriptorSetLayoutBinding);
	}

	VkDescriptorSetLayoutCreateInfo descriptorLayout = {};
	descriptorLayout.sType        = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	descriptorLayout.pNext        = nullptr;
	descriptorLayout.bindingCount = uint(vectorDescriptorSetLayoutBinding.size());
	descriptorLayout.pBindings    = vectorDescriptorSetLayoutBinding.data();
	VkResult result = vkCreateDescriptorSetLayout(coreM->getLogicalDevice(), &descriptorLayout, nullptr, &descriptorSetLayout);
	assert(result == VK_SUCCESS);

	vector<VkDescriptorPoolSize> vectorDescriptorPoolSize;
	forI(vectorStageFlags.size())
	{
		VkDescriptorPoolSize descriptorTypePool = { vectorDescriptorType[i], 1 };
		vectorDescriptorPoolSize.push_back(descriptorTypePool);
	}

	VkDescriptorPoolCreateInfo descriptorPoolCreateInfo = {};
	descriptorPoolCreateInfo.sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	descriptorPoolCreateInfo.pNext         = nullptr;
	descriptorPoolCreateInfo.maxSets       = 1;
	descriptorPoolCreateInfo.flags         = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT; // 0;
	descriptorPoolCreateInfo.poolSizeCount = (uint32_t)vectorDescriptorPoolSize.size();
	descriptorPoolCreateInfo.pPoolSizes    = vectorDescriptorPoolSize.data();
	result = vkCreateDescriptorPool(coreM->getLogicalDevice(), &descriptorPoolCreateInfo, nullptr, &descriptorPool);
	assert(result == VK_SUCCESS);

	VkDescriptorSet descriptorSet;
	VkDescriptorSetAllocateInfo dsAllocInfo;
	dsAllocInfo.sType              = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	dsAllocInfo.pNext              = nullptr;
	dsAllocInfo.descriptorPool     = descriptorPool;
	dsAllocInfo.descriptorSetCount = 1;
	dsAllocInfo.pSetLayouts        = &descriptorSetLayout;
	result = vkAllocateDescriptorSets(coreM->getLogicalDevice(), &dsAllocInfo, &descriptorSet);
	assert(result == VK_SUCCESS);

	return descriptorSet;
}

/////////////////////////////////////////////////////////////////////////////////////////////

void GPUPipeline::updateDescriptorSet(
	const VkDescriptorSet&          descriptorSet,
	const vector<VkDescriptorType>& vectorDescriptorType,
	const vector<void*>&            vectorDescriptorInfo,
	const vector<int>&              vectorDescriptorInfoHint,
	const vectorInt&                vectorDescriptorCount,
	const vector<uint32_t>&         vectorBinding)
{
	vector<VkWriteDescriptorSet> vectorWriteDescriptorSet;

	forI(vectorDescriptorType.size())
	{
		VkWriteDescriptorSet writes;
		writes = {};
		writes.sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		writes.pNext           = vectorDescriptorInfoHint[i] == 3 ? vectorDescriptorInfo[i] : nullptr; // Acceleration structures go in the pNext field
		writes.dstSet          = descriptorSet;
		writes.descriptorCount = vectorDescriptorCount[i];
		writes.descriptorType  = vectorDescriptorType[i];
		writes.dstArrayElement = 0;
		writes.dstBinding      = vectorBinding[i];

		if (vectorDescriptorInfoHint[i] == 0)
		{
			writes.pBufferInfo = (VkDescriptorBufferInfo*)vectorDescriptorInfo[i];
		}
		else if (vectorDescriptorInfoHint[i] == 1)
		{
			writes.pImageInfo = (VkDescriptorImageInfo*)vectorDescriptorInfo[i];
		}
		else if (vectorDescriptorInfoHint[i] == 2)
		{
			writes.pTexelBufferView = (VkBufferView*)vectorDescriptorInfo[i];
		}
		else if (vectorDescriptorInfoHint[i] == 4)
		{
			// Combined image sampler, using extension GL_EXT_nonuniform_qualifier
			writes.pImageInfo = ((vectorDescriptorImageInfo*)(vectorDescriptorInfo[i]))->data();
		}

		vectorWriteDescriptorSet.push_back(writes);
	}

	vkUpdateDescriptorSets(coreM->getLogicalDevice(), uint32_t(vectorWriteDescriptorSet.size()), vectorWriteDescriptorSet.data(), 0, nullptr);
}

/////////////////////////////////////////////////////////////////////////////////////////////

void GPUPipeline::destroyPipelineCache()
{
	vkDestroyPipelineCache(coreM->getLogicalDevice(), m_pipelineCache, nullptr);
	m_pipelineCache = VK_NULL_HANDLE;
}

/////////////////////////////////////////////////////////////////////////////////////////////

void GPUPipeline::shutdown()
{
	this->~GPUPipeline();
}

/////////////////////////////////////////////////////////////////////////////////////////////

void GPUPipeline::destroyResources()
{
	forI(m_vectorRasterTechnique.size())
	{
		delete m_vectorRasterTechnique[i];
	}

	destroyPipelineCache();
}

/////////////////////////////////////////////////////////////////////////////////////////////

RasterTechnique *GPUPipeline::getRasterTechniqueByName(string &&name)
{
	forIT(m_vectorRasterTechnique)
	{
		if ((*it)->getName() == name)
		{
			return *it;
		}
	}

	return nullptr;
}

/////////////////////////////////////////////////////////////////////////////////////////////

int GPUPipeline::getRasterTechniqueIndex(RasterTechnique* technique)
{
	return findElementIndex(m_vectorRasterTechnique, technique);
}

/////////////////////////////////////////////////////////////////////////////////////////////

bool GPUPipeline::addRasterFlag(string&& flagName, int value)
{
	bool result = addIfNoPresent(move(flagName), value, m_mapRasterFlag);

	if (!result)
	{
		m_mapRasterFlag[flagName] = value;
		return false;
	}

	return true;
}

/////////////////////////////////////////////////////////////////////////////////////////////

bool GPUPipeline::removeRasterFlag(string&& flagName)
{
	auto it = m_mapRasterFlag.find(move(string(flagName)));

	if (it == m_mapRasterFlag.end())
	{
		return false;
	}

	return (m_mapRasterFlag.erase(flagName) > 0);
}

/////////////////////////////////////////////////////////////////////////////////////////////

bool GPUPipeline::setRasterFlag(string&& flagName, int value)
{
	map<string, int>::iterator it = m_mapRasterFlag.find(flagName);

	if (it == m_mapRasterFlag.end())
	{
		return false;
	}

	it->second = value;
	return true;
}

/////////////////////////////////////////////////////////////////////////////////////////////

int GPUPipeline::getRasterFlagValue(string&& flagName)
{
	map<string, int>::iterator it = m_mapRasterFlag.find(flagName);

	if (it == m_mapRasterFlag.end())
	{
		return -1;
	}

	return it->second;
}

/////////////////////////////////////////////////////////////////////////////////////////////

VkVertexInputAttributeDescription* GPUPipeline::refVertexInputAttributeDescription()
{
	return &m_viIpAttrb[0];
}

/////////////////////////////////////////////////////////////////////////////////////////////

void GPUPipeline::preSceneLoadResources()
{
	Texture* renderTargetColor = textureM->buildTexture(move(string("scenelightingcolor")),
														VK_FORMAT_R8G8B8A8_UNORM,
														{ uint32_t(coreM->getWidth()), uint32_t(coreM->getHeight()), 1 },
														VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
														VK_IMAGE_ASPECT_COLOR_BIT,
														VK_IMAGE_ASPECT_COLOR_BIT,
														VK_IMAGE_LAYOUT_UNDEFINED,
														VK_IMAGE_LAYOUT_GENERAL,
														VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
														VK_SAMPLE_COUNT_1_BIT,
														VK_IMAGE_TILING_OPTIMAL,
														VK_IMAGE_VIEW_TYPE_2D,
														0);
	
	Texture* renderTargetDepth = textureM->buildTexture(move(string("scenelightingdepth")),
														VK_FORMAT_D24_UNORM_S8_UINT,
														{ uint32_t(coreM->getWidth()), uint32_t(coreM->getHeight()), 1 },
														VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
														VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT,
														VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT,
														VK_IMAGE_LAYOUT_UNDEFINED,
														VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
														VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
														VK_SAMPLE_COUNT_1_BIT,
														VK_IMAGE_TILING_OPTIMAL,
														VK_IMAGE_VIEW_TYPE_2D,
														0);

	VkAttachmentReference* depthReference  = new VkAttachmentReference({ 1, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL });
	VkPipelineBindPoint* pipelineBindPoint = new VkPipelineBindPoint(VK_PIPELINE_BIND_POINT_GRAPHICS);

	vector<VkFormat>* vectorAttachmentFormat = new vector<VkFormat>;
	vectorAttachmentFormat->push_back(VK_FORMAT_R8G8B8A8_UNORM);
	vectorAttachmentFormat->push_back(VK_FORMAT_D24_UNORM_S8_UINT);

	vector<VkSampleCountFlagBits>* vectorAttachmentSamplesPerPixel = new vector<VkSampleCountFlagBits>;
	vectorAttachmentSamplesPerPixel->push_back(VK_SAMPLE_COUNT_1_BIT);
	vectorAttachmentSamplesPerPixel->push_back(VK_SAMPLE_COUNT_1_BIT);

	vector<VkImageLayout>* vectorAttachmentFinalLayout = new vector<VkImageLayout>;
	vectorAttachmentFinalLayout->push_back(VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);
	vectorAttachmentFinalLayout->push_back(VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);

	vector<VkAttachmentReference>* vectorColorReference = new vector<VkAttachmentReference>;
	vectorColorReference->push_back({ 0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL });

	MultiTypeUnorderedMap *attributeUM = new MultiTypeUnorderedMap();
	attributeUM->newElement<AttributeData<VkAttachmentReference*>*>        (new AttributeData<VkAttachmentReference*>        (string(g_renderPassAttachmentDepthReference),    move(depthReference)));
	attributeUM->newElement<AttributeData<VkPipelineBindPoint*>*>          (new AttributeData<VkPipelineBindPoint*>          (string(g_renderPassAttachmentPipelineBindPoint), move(pipelineBindPoint)));
	attributeUM->newElement<AttributeData<vector<VkFormat>*>*>             (new AttributeData<vector<VkFormat>*>             (string(g_renderPassAttachmentFormat),            move(vectorAttachmentFormat)));
	attributeUM->newElement<AttributeData<vector<VkSampleCountFlagBits>*>*>(new AttributeData<vector<VkSampleCountFlagBits>*>(string(g_renderPassAttachmentSamplesPerPixel),   move(vectorAttachmentSamplesPerPixel)));
	attributeUM->newElement<AttributeData<vector<VkImageLayout>*>*>        (new AttributeData<vector<VkImageLayout>*>        (string(g_renderPassAttachmentFinalLayout),       move(vectorAttachmentFinalLayout)));
	attributeUM->newElement<AttributeData<vector<VkAttachmentReference>*>*>(new AttributeData<vector<VkAttachmentReference>*>(string(g_renderPassAttachmentColorReference),    move(vectorColorReference)));

	renderPassM->buildRenderPass(move(string("scenelightingrenderpass")), attributeUM);

	vector<string> arrayAttachment;
	arrayAttachment.resize(2);
	arrayAttachment[0] = renderTargetColor->getName();
	arrayAttachment[1] = renderTargetDepth->getName();
	framebufferM->buildFramebuffer(move(string("scenelightingrenderpassFB")), coreM->getWidth(), coreM->getHeight(), move(string("scenelightingrenderpass")), move(arrayAttachment));

	// Build default material
	Texture* reflectance = textureM->build2DTextureFromFile(
		move(string("reflectanceTexture0")),
		move(string("../data/scenes/bistro_v5_1/Textures/MASTER_Roofing_Shingle_Green.dds")),
		VK_IMAGE_USAGE_SAMPLED_BIT,
		VK_FORMAT_BC7_SRGB_BLOCK);

	Texture* normal = textureM->build2DTextureFromFile(
		move(string("normalTexture0")),
		move(string("../data/scenes/bistro_v5_1/Textures/MASTER_Roofing_Shingle_Green_N.dds")),
		VK_IMAGE_USAGE_SAMPLED_BIT,
		VK_FORMAT_BC5_UNORM_BLOCK);

	MultiTypeUnorderedMap *attributeMaterial = new MultiTypeUnorderedMap();
	attributeMaterial->newElement<AttributeData<string>*>(new AttributeData<string>(string(g_reflectanceTextureResourceName), string("reflectanceTexture0")));
	attributeMaterial->newElement<AttributeData<string>*>(new AttributeData<string>(string(g_normalTextureResourceName), string("normalTexture0")));
	Material* defaultMaterial = materialM->buildMaterial(move(string("MaterialColorTexture")), move(string("DefaultMaterial")), attributeMaterial);
	Material* defaultMaterialInstanced = materialM->buildMaterial(move(string("MaterialIndirectColorTexture")), move(string("DefaultMaterialInstanced")), attributeMaterial);
}

/////////////////////////////////////////////////////////////////////////////////////////////
