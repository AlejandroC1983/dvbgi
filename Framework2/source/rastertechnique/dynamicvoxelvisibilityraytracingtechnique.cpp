/*
Copyright 2022 Alejandro Cosin

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
#include "../../include/rastertechnique/dynamicvoxelvisibilityraytracingtechnique.h"
#include "../../include/scene/scene.h"
#include "../../include/core/coremanager.h"
#include "../../include/accelerationstructure/accelerationstructure.h"
#include "../../include/accelerationstructure/accelerationstructuremanager.h"
#include "../../include/material/materialmanager.h"
#include "../../include/material/materialdynamicvoxelvisibilityraytracing.h"
#include "../../include/parameter/attributedefines.h"
#include "../../include/parameter/attributedata.h"
#include "../../include/node/node.h"
#include "../../include/texture/texturemanager.h"
#include "../../include/texture/texture.h"
#include "../../include/rastertechnique/staticscenevoxelizationtechnique.h"
#include "../../include/rastertechnique/bufferprefixsumtechnique.h"
#include "../../include/util/vulkanstructinitializer.h"
#include "../../include/util/bufferverificationhelper.h"
#include "../../include/rastertechnique/voxelvisibilityraytracingtechnique.h"
#include "../../include/rastertechnique/processcameravisibleresultstechnique.h"
#include "../../include/camera/cameramanager.h"
#include "../../include/camera/camera.h"

// NAMESPACE
using namespace attributedefines;

// DEFINES
// TODO: Unify definition, it is replicated in several places
struct SceneDescription
{
	int objId;
	uint flags;
	mat4 transform;
	mat4 transformIT;
};

// STATIC MEMBER INITIALIZATION

/////////////////////////////////////////////////////////////////////////////////////////////

DynamicVoxelVisibilityRayTracingTechnique::DynamicVoxelVisibilityRayTracingTechnique(string &&name, string&& className) :
	  RasterTechnique(move(name), move(className))
	, m_voxelVisibilityDynamic4ByteBuffer(nullptr)
	, m_materialDynamicVoxelVisibilityRayTracing(nullptr)
	, m_dynamicAccelerationStructure(nullptr)
	, m_accelerationStructure(nullptr)
	, m_sceneDescriptorDynamicBuffer(nullptr)
	, m_dynamicShaderBindingTableBuffer(nullptr)
	, m_shaderBindingTableBuilt(false)
	, m_shaderGroupBaseAlignment(0)
	, m_shaderGroupHandleSize(0)
	, m_shaderGroupSizeAligned(0)
	, m_shaderGroupStride(0)
	, m_sceneMin(vec4(0.0f))
	, m_sceneExtent(vec4(0.0f))
	, m_prefixSumCompleted(true)
	, m_numStaticOccupiedVoxel(0)
	, m_numElementToProcess(0)
	, m_cameraVisibleVoxelNumber(0)
	, m_processCameraVisibleResultsTechnique(nullptr)
	, m_emitterCamera(nullptr)
{
	m_active                                      = true;
	VoxelVisibilityRayTracingTechnique* technique = static_cast<VoxelVisibilityRayTracingTechnique*>(gpuPipelineM->getRasterTechniqueByName(move(string("VoxelVisibilityRayTracingTechnique"))));
	m_numRaysPerVoxelFace                         = technique->getNumRaysPerVoxelFace();
}

/////////////////////////////////////////////////////////////////////////////////////////////

DynamicVoxelVisibilityRayTracingTechnique::~DynamicVoxelVisibilityRayTracingTechnique()
{
	m_active         = false;
	m_needsToRecord  = false;
	m_executeCommand = false;
}

/////////////////////////////////////////////////////////////////////////////////////////////

void DynamicVoxelVisibilityRayTracingTechnique::init()
{
	textureM->buildTexture(move(string("raytracingoffscreen")),
												   VK_FORMAT_R16G16B16A16_SFLOAT,
												   { uint32_t(coreM->getWidth()), uint32_t(coreM->getHeight()), 1 },
												   VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
												   VK_IMAGE_ASPECT_COLOR_BIT,
												   VK_IMAGE_ASPECT_COLOR_BIT,
												   VK_IMAGE_LAYOUT_UNDEFINED,
												   VK_IMAGE_LAYOUT_GENERAL,
												   VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
												   VK_SAMPLE_COUNT_1_BIT,
												   VK_IMAGE_TILING_OPTIMAL,
												   VK_IMAGE_VIEW_TYPE_2D,
												   0);

	m_voxelVisibilityDynamic4ByteBuffer = bufferM->buildBuffer(
		move(string("voxelVisibilityDynamic4ByteBuffer")),
		nullptr,
		256,
		VK_BUFFER_USAGE_STORAGE_BUFFER_BIT  | VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

	m_dynamicVoxelVisibilityDebugBuffer = bufferM->buildBuffer(
		move(string("dynamicVoxelVisibilityDebugBuffer")),
		nullptr,
		830000, //85000,
		VK_BUFFER_USAGE_STORAGE_BUFFER_BIT  | VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

	// NOTE: This buffer could be set as not host visible, once all debug operations have been completed
	m_dynamicVoxelVisibilityFlagBuffer = bufferM->buildBuffer(move(string("dynamicVoxelVisibilityFlagBuffer")),
		nullptr,
		256,
		VK_BUFFER_USAGE_STORAGE_BUFFER_BIT  | VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

	StaticSceneVoxelizationTechnique* sceneVoxelizationTechnique = static_cast<StaticSceneVoxelizationTechnique*>(gpuPipelineM->getRasterTechniqueByName(move(string("StaticSceneVoxelizationTechnique"))));
	
	BBox3D& box = sceneM->refBox();
	
	vec3 max3D;
	vec3 min3D;
	box.getCenteredBoxMinMax(min3D, max3D);
	
	vec3 extent3D = max3D - min3D;
	m_sceneMin    = vec4(min3D.x,    min3D.y,    min3D.z, 0.0f);
	m_sceneExtent = vec4(extent3D.x, extent3D.y, extent3D.z, float(sceneVoxelizationTechnique->getVoxelizedSceneWidth()));

	m_techniquePrefixSum = static_cast<BufferPrefixSumTechnique*>(gpuPipelineM->getRasterTechniqueByName(move(string("BufferPrefixSumTechnique"))));
	m_techniquePrefixSum->refPrefixSumComplete().connect<DynamicVoxelVisibilityRayTracingTechnique, &DynamicVoxelVisibilityRayTracingTechnique::slotPrefixSumComplete>(this);

	m_processCameraVisibleResultsTechnique = static_cast<ProcessCameraVisibleResultsTechnique*>(gpuPipelineM->getRasterTechniqueByName(move(string("ProcessCameraVisibleResultsTechnique"))));
	m_processCameraVisibleResultsTechnique->refSignalProcessCameraVisibleCompletion().connect<DynamicVoxelVisibilityRayTracingTechnique, &DynamicVoxelVisibilityRayTracingTechnique::slotCameraVisibleVoxelCompleted>(this);

	vector<Node*> vectorNode = sceneM->getByNodeType(eNodeType::E_NT_DYNAMIC_ELEMENT | eNodeType::E_NT_SKINNEDMESH_ELEMENT);

	// TODO: Remove dynamic elements from VoxelVisibilityRayTracingTechnique
	m_dynamicAccelerationStructure = accelerationStructureM->getElement(move(string("dynamicraytracedaccelerationstructure")));

	// Use the SceneDescription::flags field to encode the material index
	vectorString vectorTextureResourceName;
	vectorMaterialPtr vectorMaterial = materialM->getElementByClassName(move(string("MaterialIndirectColorTexture")));

	vector<SceneDescription> vectorSceneDescription(vectorNode.size());

	// TODO: Refactor / make method, it is used here twice and in VoxelVisibilityRayTracingTechnique::init
	forI(vectorNode.size())
	{
		vectorSceneDescription[i].objId       = i;
		vectorMaterialPtr::iterator it        = find(vectorMaterial.begin(), vectorMaterial.end(), vectorNode[i]->getRenderComponent()->getMaterialInstanced());
		assert(it != vectorMaterial.end());
		uint distance                         = std::distance(vectorMaterial.begin(), it);
		uint isDynamic                        = (vectorNode[i]->getNodeType() == eNodeType::E_NT_STATIC_ELEMENT) ? 0 : 1;

		// Encode in the last 16 bits the index of the material, and in the first bit, a flag to know whether the scene node is
		// static (0) or dynamic (1). The material index refers to an absolute index of all materials. All textures are added to the combined
		// image sampler "textureArrayCombined" in the ray closest hit so preparing acceleration structures with only some scene elements will 
		// not be a problem to sample the correct texture as long as the indices refer to the whole set of materials used in the scene
		distance                            <<= 16;
		distance                             |= isDynamic;
		vectorSceneDescription[i].flags       = distance;
		vectorSceneDescription[i].transform   = vectorNode[i]->getModelMat();
		vectorSceneDescription[i].transformIT = glm::inverse(vectorNode[i]->getModelMat());
	}

	m_sceneDescriptorDynamicBuffer = bufferM->buildBuffer(move(string("sceneDescriptorDynamicBuffer")),
		(void*)vectorSceneDescription.data(),
		sizeof(SceneDescription) * vectorSceneDescription.size(),
		VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

	m_accelerationStructure = accelerationStructureM->getElement(move(string("raytracedaccelerationstructure")));

	m_materialDynamicVoxelVisibilityRayTracing = static_cast<MaterialDynamicVoxelVisibilityRayTracing*>(materialM->buildMaterial(move(string("MaterialDynamicVoxelVisibilityRayTracing")), move(string("MaterialDynamicVoxelVisibilityRayTracing")), nullptr));
	m_vectorMaterialName.push_back("MaterialDynamicVoxelVisibilityRayTracing");
	m_vectorMaterial.push_back(static_cast<Material*>(m_materialDynamicVoxelVisibilityRayTracing));

	m_materialDynamicVoxelVisibilityRayTracing->setSceneExtentAndVoxelSize(m_sceneExtent);

	const VkPhysicalDeviceRayTracingPipelinePropertiesKHR& physicalDeviceRayTracingPipelineProperties = coreM->getkPhysicalDeviceRayTracingPipelineProperties();

	m_shaderGroupBaseAlignment = physicalDeviceRayTracingPipelineProperties.shaderGroupBaseAlignment;
	m_shaderGroupHandleSize    = physicalDeviceRayTracingPipelineProperties.shaderGroupHandleSize;
	m_shaderGroupSizeAligned   = uint32_t((m_shaderGroupHandleSize + (m_shaderGroupBaseAlignment - 1)) & ~uint32_t(m_shaderGroupBaseAlignment - 1));
	m_shaderGroupStride        = m_shaderGroupSizeAligned;

	m_emitterCamera            = cameraM->getElement(move(string("emitter")));
}

/////////////////////////////////////////////////////////////////////////////////////////////

void DynamicVoxelVisibilityRayTracingTechnique::prepare(float dt)
{
	// TODO: Refactor with VoxelVisibilityRayTracingTechnique as much as possible
	// TODO: Listen from CameraVisibleVoxelTechnique completion and process ony visible voxels
	if (!m_shaderBindingTableBuilt)
	{
		buildShaderBindingTable();
		m_shaderBindingTableBuilt = true;
	}

	// TODO: Put this as a callback from scene when updating it
	if (sceneM->getIsVectorNodeFrameUpdated())
	{
		const vectorNodePtr& vectorNode = sceneM->getVectorNodeFrameUpdated();
		m_dynamicAccelerationStructure->updateTLAS(&vectorNode);
		m_accelerationStructure->updateTLAS(&vectorNode);

		// TODO: Review all update of TLAS and make sure they only use the partial case, also review the update of BLAS
	}

	if (sceneM->getIsVectorNodeFrameUpdated())
	{
		// NOTE: The call to RasterTechnique::preRecordLoop() sets m_needsToRecord to false, reevaluate the role of that line of code
		//       "m_needsToRecord  = (m_vectorCommand.size() != m_usedCommandBufferNumber);"
		m_needsToRecord = true;
		m_materialDynamicVoxelVisibilityRayTracing->setLightPosition(vec4(m_emitterCamera->getPosition(), 0.0f));
	}
}

/////////////////////////////////////////////////////////////////////////////////////////////

VkCommandBuffer* DynamicVoxelVisibilityRayTracingTechnique::record(int currentImage, uint& commandBufferID, CommandBufferType& commandBufferType)
{
	commandBufferType = CommandBufferType::CBT_GRAPHICS_QUEUE;

	VkCommandBuffer* commandBuffer;
	commandBuffer = addRecordedCommandBuffer(commandBufferID);
	addCommandBufferQueueType(commandBufferID, commandBufferType);
	coreM->allocCommandBuffer(&coreM->getLogicalDevice(), coreM->getGraphicsCommandPool(), commandBuffer);
	coreM->setObjectName(uint64_t(*commandBuffer), VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT, getName());

	coreM->beginCommandBuffer(*commandBuffer);

	VulkanStructInitializer::insertBufferMemoryBarrier(bufferM->getElement(move(string("cameraVisibleVoxelCompactedBuffer"))),
													   VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT,
													   VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT,
													   VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
													   VK_PIPELINE_STAGE_RAY_TRACING_SHADER_BIT_KHR,
													   commandBuffer);

#ifdef USE_TIMESTAMP
	vkCmdResetQueryPool(*commandBuffer, coreM->getGraphicsQueueQueryPool(), m_queryIndex0, 1);
	vkCmdResetQueryPool(*commandBuffer, coreM->getGraphicsQueueQueryPool(), m_queryIndex1, 1);
	vkCmdWriteTimestamp(*commandBuffer, VK_PIPELINE_STAGE_RAY_TRACING_SHADER_BIT_KHR, coreM->getGraphicsQueueQueryPool(), m_queryIndex0);
#endif

	MaterialDynamicVoxelVisibilityRayTracing* temp = static_cast<MaterialDynamicVoxelVisibilityRayTracing*>(m_vectorMaterial[0]);

	vkCmdBindPipeline(*commandBuffer, VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR, m_materialDynamicVoxelVisibilityRayTracing->getPipeline()->getPipeline());

	uint dynamicAllignment  = materialM->getMaterialUBDynamicAllignment();
	uint32_t materialOffset = static_cast<uint32_t>(m_materialDynamicVoxelVisibilityRayTracing->getMaterialUniformBufferIndex() * dynamicAllignment);

	uint offsetData[2] = { 0, materialOffset };
	vkCmdBindDescriptorSets(*commandBuffer, VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR, m_materialDynamicVoxelVisibilityRayTracing->getPipelineLayout(), 0, 1, &m_materialDynamicVoxelVisibilityRayTracing->refDescriptorSet(), 2, &offsetData[0]);

	VkDeviceAddress shaderBindingTableDeviceAddress = m_dynamicShaderBindingTableBuffer->getBufferDeviceAddress();
	
	vector<VkStridedDeviceAddressRegionKHR> vectorStridedDeviceAddressRegion(4);
	vectorStridedDeviceAddressRegion[0] = VkStridedDeviceAddressRegionKHR{ shaderBindingTableDeviceAddress + 0u * m_shaderGroupSizeAligned, m_shaderGroupStride, m_shaderGroupSizeAligned * 1 };
	vectorStridedDeviceAddressRegion[1] = VkStridedDeviceAddressRegionKHR{ shaderBindingTableDeviceAddress + 1u * m_shaderGroupSizeAligned, m_shaderGroupStride, m_shaderGroupSizeAligned * 1 };
	vectorStridedDeviceAddressRegion[2] = VkStridedDeviceAddressRegionKHR{ shaderBindingTableDeviceAddress + 2u * m_shaderGroupSizeAligned, m_shaderGroupStride, m_shaderGroupSizeAligned * 1 };
	vectorStridedDeviceAddressRegion[3] = {0, 0, 0};

	vkfpM->vkCmdTraceRaysKHR(*commandBuffer, &vectorStridedDeviceAddressRegion[0], &vectorStridedDeviceAddressRegion[1], &vectorStridedDeviceAddressRegion[2], &vectorStridedDeviceAddressRegion[3], m_cameraVisibleVoxelNumber, 1, 6);

#ifdef USE_TIMESTAMP
	vkCmdWriteTimestamp(*commandBuffer, VK_PIPELINE_STAGE_RAY_TRACING_SHADER_BIT_KHR, coreM->getGraphicsQueueQueryPool(), m_queryIndex1);
#endif

	coreM->endCommandBuffer(*commandBuffer);

	// NOTE: Clear command buffer if re-recorded
	m_vectorCommand.push_back(commandBuffer);

	return commandBuffer;
}

/////////////////////////////////////////////////////////////////////////////////////////////

void DynamicVoxelVisibilityRayTracingTechnique::postCommandSubmit()
{
	m_signalDynamicVoxelVisibilityRayTracing.emit();

	m_executeCommand = false;
	m_active         = false;
	m_needsToRecord  = (m_vectorCommand.size() != m_usedCommandBufferNumber);
}

/////////////////////////////////////////////////////////////////////////////////////////////

void DynamicVoxelVisibilityRayTracingTechnique::buildShaderBindingTable()
{
	// TODO: Refactor with the static acceleration structure part
	MaterialDynamicVoxelVisibilityRayTracing* temp = static_cast<MaterialDynamicVoxelVisibilityRayTracing*>(m_vectorMaterial[0]);

	vector<VkRayTracingShaderGroupCreateInfoKHR>& vectorRayTracingShaderGroupCreateInfo = m_materialDynamicVoxelVisibilityRayTracing->refShader()->refVectorRayTracingShaderGroupCreateInfo();

	uint numberShaderGroup      = static_cast<uint>(vectorRayTracingShaderGroupCreateInfo.size());
	uint shaderBindingTableSize = numberShaderGroup * m_shaderGroupSizeAligned;

	// Retrieve shader handles
	vector<uint8_t> vectorShaderHandleStorage(shaderBindingTableSize);
	VkResult result = vkfpM->vkGetRayTracingShaderGroupHandlesKHR(coreM->getLogicalDevice(), m_materialDynamicVoxelVisibilityRayTracing->getPipeline()->getPipeline(), 0, numberShaderGroup, shaderBindingTableSize, vectorShaderHandleStorage.data());

	assert(result == VK_SUCCESS);

	vector<uint8_t> vectorShaderBindingTableBuffer(shaderBindingTableSize);
	uint8_t* pVectorShaderBindingTableBuffer = static_cast<uint8_t*>(vectorShaderBindingTableBuffer.data());

	forI(numberShaderGroup)
	{
		memcpy(pVectorShaderBindingTableBuffer, vectorShaderHandleStorage.data() + i * m_shaderGroupHandleSize, m_shaderGroupHandleSize);
		pVectorShaderBindingTableBuffer += m_shaderGroupSizeAligned;
	}

	m_dynamicShaderBindingTableBuffer = bufferM->buildBuffer(move(string("dynamicShaderBindingTableBuffer")),
		(void*)vectorShaderBindingTableBuffer.data(),
		sizeof(uint8_t) * vectorShaderBindingTableBuffer.size(),
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT |VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_SHADER_BINDING_TABLE_BIT_KHR,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
}

/////////////////////////////////////////////////////////////////////////////////////////////

void DynamicVoxelVisibilityRayTracingTechnique::slotPrefixSumComplete()
{
	// TODO: m_numStaticOccupiedVoxel and m_numElementToProcess should be taken from the callback from the camera visible technique
	m_numStaticOccupiedVoxel = m_techniquePrefixSum->getFirstIndexOccupiedElement();
	m_numElementToProcess    = m_numStaticOccupiedVoxel * m_numRaysPerVoxelFace * 6; // Each local workgroup will work one side of each voxel in the scene
	m_sceneMin.w             = float(m_numStaticOccupiedVoxel);

	MaterialDynamicVoxelVisibilityRayTracing* temp = static_cast<MaterialDynamicVoxelVisibilityRayTracing*>(m_vectorMaterial[0]);

	m_materialDynamicVoxelVisibilityRayTracing->setSceneMinAndNumberVoxel(m_sceneMin);

	vector<uint> vectorData;
	vectorData.resize(m_numElementToProcess);
	memset(vectorData.data(), maxValue, vectorData.size() * size_t(sizeof(uint)));

	bufferM->resize(m_dynamicVoxelVisibilityFlagBuffer,  nullptr,           m_numStaticOccupiedVoxel * 4 * 6 * sizeof(uint)); // Number of static occupied voxels * number of uints used to map visibility (4 in this case) * number of voxel faces
	bufferM->resize(m_voxelVisibilityDynamic4ByteBuffer, vectorData.data(), m_numElementToProcess            * sizeof(uint));

	m_prefixSumCompleted = true;
}

/////////////////////////////////////////////////////////////////////////////////////////////

void DynamicVoxelVisibilityRayTracingTechnique::slotCameraVisibleVoxelCompleted()
{
	if (m_prefixSumCompleted)
	{
		m_cameraVisibleVoxelNumber = m_processCameraVisibleResultsTechnique->getStaticCameraVisibleVoxelNumber();
		m_sceneMin.w               = float(m_cameraVisibleVoxelNumber);
		m_materialDynamicVoxelVisibilityRayTracing->setSceneMinAndNumberVoxel(m_sceneMin);

		m_active        = true;
		m_needsToRecord = true;
	}
}

/////////////////////////////////////////////////////////////////////////////////////////////

