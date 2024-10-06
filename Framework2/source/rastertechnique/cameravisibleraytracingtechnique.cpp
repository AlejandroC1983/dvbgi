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
#include "../../include/rastertechnique/cameravisibleraytracingtechnique.h"
#include "../../include/scene/scene.h"
#include "../../include/core/coremanager.h"
#include "../../include/material/materialmanager.h"
#include "../../include/parameter/attributedefines.h"
#include "../../include/parameter/attributedata.h"
#include "../../include/texture/texturemanager.h"
#include "../../include/texture/texture.h"
#include "../../include/buffer/buffer.h"
#include "../../include/buffer/buffermanager.h"
#include "../../include/material/materialcameravisibleraytracing.h"
#include "../../include/camera/camera.h"
#include "../../include/camera/cameramanager.h"
#include "../../include/rastertechnique/staticscenevoxelizationtechnique.h"
#include "../../include/rastertechnique/litvoxeltechnique.h"
#include "../../include/util/vulkanstructinitializer.h"
#include "../../include/rastertechnique/voxelvisibilityraytracingtechnique.h"
#include "../../include/rastertechnique/bufferprefixsumtechnique.h"

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

CameraVisibleRayTracingTechnique::CameraVisibleRayTracingTechnique(string &&name, string&& className) :
	  RasterTechnique(move(name), move(className))
	, m_materialCameraVisibleRayTracing(nullptr)
	, m_cameraVisibleShaderBindingTableBuffer(nullptr)
	, m_shaderBindingTableBuilt(false)
	, m_shaderGroupBaseAlignment(0)
	, m_shaderGroupHandleSize(0)
	, m_shaderGroupStride(0)
	, m_offscreenWidth(0)
	, m_offscreenHeight(0)
	, m_mainCamera(nullptr)
	, m_cameraVisibleDebugBuffer(nullptr)
	, m_litVoxelTechnique(nullptr)
	, m_cameraVisibleVoxelCompactedBuffer(nullptr)
	, m_cameraVisibleVoxelDebugBuffer(nullptr)
	, m_cameraVisibleCounterBuffer(nullptr)
	, m_cameraVisibleDynamicVoxelBuffer(nullptr)
	, m_voxelVisibilityRayTracingTechnique(nullptr)
	, m_cameraVisibleDynamicVoxelIrradianceBuffer(nullptr)
	, m_dynamicVisibleVoxelCounterBuffer(nullptr)
	, m_cameraVisibleNormal(nullptr)
	, m_cameraVisibleReflectance(nullptr)
	, m_cameraVisiblePosition(nullptr)
	, m_numOccupiedVoxel(0)
	, m_bufferPrefixSumTechnique(nullptr)
{
	m_active                 = false;
	m_needsToRecord          = false;
	m_executeCommand         = false;

	// TODO: Add an enum for ray tracing and an potion to host syncronize results
	//m_rasterTechniqueType    = RasterTechniqueType::RTT_COMPUTE;
	m_computeHostSynchronize = true;
}

/////////////////////////////////////////////////////////////////////////////////////////////

CameraVisibleRayTracingTechnique::~CameraVisibleRayTracingTechnique()
{
	m_active         = false;
	m_needsToRecord  = false;
	m_executeCommand = false;
}

/////////////////////////////////////////////////////////////////////////////////////////////

void CameraVisibleRayTracingTechnique::init()
{
	m_offscreenWidth  = coreM->getWidth();
	m_offscreenHeight = coreM->getHeight();

	m_cameraVisibleNormal = textureM->buildTexture(move(string("cameraVisibleNormal")),
						   VK_FORMAT_R8G8B8A8_UNORM,
						   { m_offscreenWidth, m_offscreenHeight, 1 },
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

	m_cameraVisibleReflectance = textureM->buildTexture(move(string("cameraVisibleReflectance")),
						   VK_FORMAT_R8G8B8A8_UNORM,
						   { m_offscreenWidth, m_offscreenHeight, 1 },
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

	m_cameraVisiblePosition = textureM->buildTexture(move(string("cameraVisiblePosition")),
						   VK_FORMAT_R16G16B16A16_SFLOAT,
						   { m_offscreenWidth, m_offscreenHeight, 1 },
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

	m_cameraVisibleVoxelCompactedBuffer = bufferM->buildBuffer(
		move(string("cameraVisibleVoxelCompactedBuffer")),
		nullptr,
		256,
		VK_BUFFER_USAGE_STORAGE_BUFFER_BIT  | VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

	m_cameraVisibleVoxelDebugBuffer = bufferM->buildBuffer(
		move(string("cameraVisibleVoxelDebugBuffer")),
		nullptr,
		256,
		VK_BUFFER_USAGE_STORAGE_BUFFER_BIT  | VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

	uint tempInitValue = 0;

	m_cameraVisibleCounterBuffer = bufferM->buildBuffer(
		move(string("cameraVisibleCounterBuffer")),
		(void*)(&tempInitValue),
		4,
		VK_BUFFER_USAGE_STORAGE_BUFFER_BIT  | VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

	m_voxelVisibilityRayTracingTechnique = static_cast<VoxelVisibilityRayTracingTechnique*>(gpuPipelineM->getRasterTechniqueByName(move(string("VoxelVisibilityRayTracingTechnique"))));
	int maxDynamicVoxel                  = m_voxelVisibilityRayTracingTechnique->getMaxDynamicVoxel();

	m_cameraVisibleDynamicVoxelBuffer = bufferM->buildBuffer(
		move(string("cameraVisibleDynamicVoxelBuffer")),
		nullptr,
		4 * maxDynamicVoxel,
		VK_BUFFER_USAGE_STORAGE_BUFFER_BIT  | VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

	m_cameraVisibleDynamicVoxelIrradianceBuffer = bufferM->buildBuffer(
		move(string("cameraVisibleDynamicVoxelIrradianceBuffer")),
		nullptr,
		4 * maxDynamicVoxel * 6 * 3, // Same size as cameraVisibleDynamicVoxelBuffer but considering space to store each voxel face's irradiance for each one of the three rgb components
		VK_BUFFER_USAGE_STORAGE_BUFFER_BIT  | VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

	m_dynamicVisibleVoxelCounterBuffer = bufferM->buildBuffer(
		move(string("dynamicVisibleVoxelCounterBuffer")),
		(void*)(&tempInitValue),
		4,
		VK_BUFFER_USAGE_STORAGE_BUFFER_BIT  | VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

	m_cameraVisibleDebugBuffer = bufferM->buildBuffer(move(string("cameraVisibleDebugBuffer")),
		                                              nullptr,
		                                              500000000,
		                                              VK_BUFFER_USAGE_STORAGE_BUFFER_BIT  | VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
		                                              VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

	m_materialCameraVisibleRayTracing = static_cast<MaterialCameraVisibleRayTracing*>(materialM->buildMaterial(move(string("MaterialCameraVisibleRayTracing")), move(string("MaterialCameraVisibleRayTracing")), nullptr));
	m_vectorMaterialName.push_back("MaterialCameraVisibleRayTracing");
	m_vectorMaterial.push_back(static_cast<Material*>(m_materialCameraVisibleRayTracing));

	StaticSceneVoxelizationTechnique* sceneVoxelizationTechnique = static_cast<StaticSceneVoxelizationTechnique*>(gpuPipelineM->getRasterTechniqueByName(move(string("StaticSceneVoxelizationTechnique"))));

	BBox3D& box = sceneM->refBox();

	vec3 max3D;
	vec3 min3D;
	box.getCenteredBoxMinMax(min3D, max3D);

	vec3 extent3D                    = max3D - min3D;
	vec4 sceneMin                    = vec4(min3D.x, min3D.y, min3D.z, 0.0f);
	vec4 sceneExtentVoxelizationSize = vec4(extent3D.x, extent3D.y, extent3D.z, float(sceneVoxelizationTechnique->getVoxelizedSceneWidth()));

	m_materialCameraVisibleRayTracing->setSceneMin(sceneMin);
	m_materialCameraVisibleRayTracing->setSceneExtentVoxelizationSize(sceneExtentVoxelizationSize);

	const VkPhysicalDeviceRayTracingPipelinePropertiesKHR& physicalDeviceRayTracingPipelineProperties = coreM->getkPhysicalDeviceRayTracingPipelineProperties();

	m_shaderGroupBaseAlignment = physicalDeviceRayTracingPipelineProperties.shaderGroupBaseAlignment;
	m_shaderGroupHandleSize    = physicalDeviceRayTracingPipelineProperties.shaderGroupHandleSize;
	m_shaderGroupSizeAligned   = uint32_t((m_shaderGroupHandleSize + (m_shaderGroupBaseAlignment - 1)) & ~uint32_t(m_shaderGroupBaseAlignment - 1));
	m_shaderGroupStride        = m_shaderGroupSizeAligned;

	m_mainCamera               = cameraM->getElement(move(string("maincamera")));

	m_litVoxelTechnique = static_cast<LitVoxelTechnique*>(gpuPipelineM->getRasterTechniqueByName(move(string("LitVoxelTechnique"))));
	m_litVoxelTechnique->refSignalLitVoxelCompletion().connect<CameraVisibleRayTracingTechnique, &CameraVisibleRayTracingTechnique::slotLitVoxelComplete>(this);
	
	m_bufferPrefixSumTechnique = static_cast<BufferPrefixSumTechnique*>(gpuPipelineM->getRasterTechniqueByName(move(string("BufferPrefixSumTechnique"))));
	m_bufferPrefixSumTechnique->refPrefixSumComplete().connect<CameraVisibleRayTracingTechnique, &CameraVisibleRayTracingTechnique::slotPrefixSumComplete>(this);
}

/////////////////////////////////////////////////////////////////////////////////////////////

void CameraVisibleRayTracingTechnique::prepare(float dt)
{
	// TODO: Refactor with VoxelVisibilityRayTracingTechnique as much as possible
	// TODO: Listen from CameraVisibleVoxelTechnique completion and process ony visible voxels
	if (!m_shaderBindingTableBuilt)
	{
		buildShaderBindingTable();
		m_shaderBindingTableBuilt = true;
	}

	Camera* shadowMappingCamera = cameraM->getElement(move(string("emitter")));
	vec3 position               = shadowMappingCamera->getPosition();
	vec3 cameraForward          = shadowMappingCamera->getLookAt();

	m_materialCameraVisibleRayTracing->setLightPosition(vec4(position.x, position.y, position.z, 0.0f));
	m_materialCameraVisibleRayTracing->setLightForward(vec4(cameraForward.x, cameraForward.y, cameraForward.z, 0.0f));
}

/////////////////////////////////////////////////////////////////////////////////////////////

VkCommandBuffer* CameraVisibleRayTracingTechnique::record(int currentImage, uint& commandBufferID, CommandBufferType& commandBufferType)
{
	commandBufferType = CommandBufferType::CBT_GRAPHICS_QUEUE;

	VkCommandBuffer* commandBuffer;
	commandBuffer = addRecordedCommandBuffer(commandBufferID);
	addCommandBufferQueueType(commandBufferID, commandBufferType);
	coreM->allocCommandBuffer(&coreM->getLogicalDevice(), coreM->getGraphicsCommandPool(), commandBuffer);
	coreM->setObjectName(uint64_t(*commandBuffer), VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT, getName());

	coreM->beginCommandBuffer(*commandBuffer);

#ifdef USE_TIMESTAMP
	vkCmdResetQueryPool(*commandBuffer, coreM->getGraphicsQueueQueryPool(), m_queryIndex0, 1);
	vkCmdResetQueryPool(*commandBuffer, coreM->getGraphicsQueueQueryPool(), m_queryIndex1, 1);
	vkCmdWriteTimestamp(*commandBuffer, VK_PIPELINE_STAGE_RAY_TRACING_SHADER_BIT_KHR, coreM->getGraphicsQueueQueryPool(), m_queryIndex0);
#endif

	VkImageSubresourceRange subresourceRange = {};
	subresourceRange.aspectMask   = VK_IMAGE_ASPECT_COLOR_BIT;
	subresourceRange.baseMipLevel = 0;
	subresourceRange.levelCount   = 1;
	subresourceRange.layerCount   = 1;

	VkBufferMemoryBarrier bufferMemoryBarrier;

	Buffer* buffer                          = bufferM->getElement(move(string("voxelOccupiedBuffer")));
	bufferMemoryBarrier.sType               = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
	bufferMemoryBarrier.pNext               = nullptr;
	bufferMemoryBarrier.srcAccessMask       = VK_ACCESS_SHADER_WRITE_BIT;
	bufferMemoryBarrier.dstAccessMask       = VK_ACCESS_SHADER_READ_BIT;
	bufferMemoryBarrier.srcQueueFamilyIndex = coreM->getGraphicsQueueIndex();
	bufferMemoryBarrier.dstQueueFamilyIndex = coreM->getGraphicsQueueIndex();
	bufferMemoryBarrier.offset              = 0;
	bufferMemoryBarrier.buffer              = buffer->getBuffer();
	bufferMemoryBarrier.size                = buffer->getDataSize();
	vkCmdPipelineBarrier(*commandBuffer, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_PIPELINE_STAGE_RAY_TRACING_SHADER_BIT_KHR, 0, 0, NULL, 1, &bufferMemoryBarrier, 0, NULL);

	buffer                                  = bufferM->getElement(move(string("voxelOccupiedDynamicBuffer")));
	bufferMemoryBarrier.buffer              = buffer->getBuffer();
	bufferMemoryBarrier.size                = buffer->getDataSize();
	vkCmdPipelineBarrier(*commandBuffer, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_PIPELINE_STAGE_RAY_TRACING_SHADER_BIT_KHR, 0, 0, NULL, 1, &bufferMemoryBarrier, 0, NULL);

	MaterialCameraVisibleRayTracing* temp = static_cast<MaterialCameraVisibleRayTracing*>(m_vectorMaterial[0]);

	vkCmdBindPipeline(*commandBuffer, VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR, m_materialCameraVisibleRayTracing->getPipeline()->getPipeline());

	uint dynamicAllignment  = materialM->getMaterialUBDynamicAllignment();
	uint32_t materialOffset = static_cast<uint32_t>(m_materialCameraVisibleRayTracing->getMaterialUniformBufferIndex() * dynamicAllignment);

	uint offsetData[1] = { materialOffset };
	vkCmdBindDescriptorSets(*commandBuffer, VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR, m_materialCameraVisibleRayTracing->getPipelineLayout(), 0, 1, &m_materialCameraVisibleRayTracing->refDescriptorSet(), 1, &offsetData[0]);

	VkDeviceAddress shaderBindingTableDeviceAddress = m_cameraVisibleShaderBindingTableBuffer->getBufferDeviceAddress();
	
	vector<VkStridedDeviceAddressRegionKHR> vectorStridedDeviceAddressRegion(4);
	vectorStridedDeviceAddressRegion[0] = VkStridedDeviceAddressRegionKHR{ shaderBindingTableDeviceAddress + 0u * m_shaderGroupSizeAligned, m_shaderGroupStride, m_shaderGroupSizeAligned * 1 };
	vectorStridedDeviceAddressRegion[1] = VkStridedDeviceAddressRegionKHR{ shaderBindingTableDeviceAddress + 1u * m_shaderGroupSizeAligned, m_shaderGroupStride, m_shaderGroupSizeAligned * 1 };
	vectorStridedDeviceAddressRegion[2] = VkStridedDeviceAddressRegionKHR{ shaderBindingTableDeviceAddress + 2u * m_shaderGroupSizeAligned, m_shaderGroupStride, m_shaderGroupSizeAligned * 1 };
	vectorStridedDeviceAddressRegion[3] = {0, 0, 0};

	vkfpM->vkCmdTraceRaysKHR(*commandBuffer, &vectorStridedDeviceAddressRegion[0], &vectorStridedDeviceAddressRegion[1], &vectorStridedDeviceAddressRegion[2], &vectorStridedDeviceAddressRegion[3], m_offscreenWidth, m_offscreenHeight, 1);

	VulkanStructInitializer::insertBufferMemoryBarrier(bufferM->getElement(move(string("cameraVisibleVoxelPerByteBuffer"))),
													   VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT,
													   VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT,
													   VK_PIPELINE_STAGE_RAY_TRACING_SHADER_BIT_KHR,
													   VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
													   commandBuffer);

	VulkanStructInitializer::insertBufferMemoryBarrier(bufferM->getElement(move(string("cameraVisibleDynamicVoxelPerByteBuffer"))),
													   VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT,
													   VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT,
													   VK_PIPELINE_STAGE_RAY_TRACING_SHADER_BIT_KHR,
													   VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
													   commandBuffer);

#ifdef USE_TIMESTAMP
	vkCmdWriteTimestamp(*commandBuffer, VK_PIPELINE_STAGE_RAY_TRACING_SHADER_BIT_KHR, coreM->getGraphicsQueueQueryPool(), m_queryIndex1);
#endif

	coreM->endCommandBuffer(*commandBuffer);

	// NOTE: Clear command buffer if re-recorded
	m_vectorCommand.push_back(commandBuffer);

	return commandBuffer;
}

/////////////////////////////////////////////////////////////////////////////////////////////

void CameraVisibleRayTracingTechnique::postCommandSubmit()
{
	m_executeCommand = false;
	m_active         = false;
	m_needsToRecord  = false;
	m_needsToRecord  = (m_vectorCommand.size() != m_usedCommandBufferNumber);

	m_signalCameraVisibleRayTracing.emit();
}

/////////////////////////////////////////////////////////////////////////////////////////////

void CameraVisibleRayTracingTechnique::buildShaderBindingTable()
{
	// TODO: Refactor with the static acceleration structure part
	MaterialCameraVisibleRayTracing* temp = static_cast<MaterialCameraVisibleRayTracing*>(m_vectorMaterial[0]);

	vector<VkRayTracingShaderGroupCreateInfoKHR>& vectorRayTracingShaderGroupCreateInfo = m_materialCameraVisibleRayTracing->refShader()->refVectorRayTracingShaderGroupCreateInfo();

	uint numberShaderGroup      = static_cast<uint>(vectorRayTracingShaderGroupCreateInfo.size());
	uint shaderBindingTableSize = numberShaderGroup * m_shaderGroupSizeAligned;

	// Retrieve shader handles
	vector<uint8_t> vectorShaderHandleStorage(shaderBindingTableSize);
	VkResult result = vkfpM->vkGetRayTracingShaderGroupHandlesKHR(coreM->getLogicalDevice(), m_materialCameraVisibleRayTracing->getPipeline()->getPipeline(), 0, numberShaderGroup, shaderBindingTableSize, vectorShaderHandleStorage.data());

	assert(result == VK_SUCCESS);

	vector<uint8_t> vectorShaderBindingTableBuffer(shaderBindingTableSize);
	uint8_t* pVectorShaderBindingTableBuffer = static_cast<uint8_t*>(vectorShaderBindingTableBuffer.data());

	forI(numberShaderGroup)
	{
		memcpy(pVectorShaderBindingTableBuffer, vectorShaderHandleStorage.data() + i * m_shaderGroupHandleSize, m_shaderGroupHandleSize);
		pVectorShaderBindingTableBuffer += m_shaderGroupSizeAligned;
	}

	m_cameraVisibleShaderBindingTableBuffer = bufferM->buildBuffer(move(string("cameraVisibleShaderBindingTableBuffer")),
		(void*)vectorShaderBindingTableBuffer.data(),
		sizeof(uint8_t) * vectorShaderBindingTableBuffer.size(),
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_SHADER_BINDING_TABLE_BIT_KHR,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
}

/////////////////////////////////////////////////////////////////////////////////////////////

void CameraVisibleRayTracingTechnique::slotPrefixSumComplete()
{
	m_numOccupiedVoxel = m_bufferPrefixSumTechnique->getFirstIndexOccupiedElement();
	bufferM->resize(m_cameraVisibleVoxelCompactedBuffer, nullptr, m_numOccupiedVoxel * sizeof(uint));
}

/////////////////////////////////////////////////////////////////////////////////////////////

void CameraVisibleRayTracingTechnique::slotLitVoxelComplete()
{
	m_materialCameraVisibleRayTracing->setViewMatrixInverse(glm::inverse(m_mainCamera->getView()));
	m_materialCameraVisibleRayTracing->setProjectionMatrixInverse(glm::inverse(m_mainCamera->getProjection()));
	m_materialCameraVisibleRayTracing->setCameraPosition(vec4(m_mainCamera->getPosition(), 0.0));
		
	m_active        = true;
	m_needsToRecord = true;
}

/////////////////////////////////////////////////////////////////////////////////////////////