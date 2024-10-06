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
#include "../../include/rastertechnique/dynamiclightbouncevoxelirradiancetechnique.h"
#include "../../include/rastertechnique/lightbouncevoxelirradiancetechnique.h"
#include "../../include/rastertechnique/processcameravisibleresultstechnique.h"
#include "../../include/material/materialmanager.h"
#include "../../include/material/materiallightbouncedynamicvoxelirradiance.h"
#include "../../include/core/gpupipeline.h"
#include "../../include/camera/camera.h"
#include "../../include/camera/cameramanager.h"
#include "../../include/rastertechnique/staticscenevoxelizationtechnique.h"
#include "../../include/rastertechnique/resetlitvoxeldatatechnique.h"
#include "../../include/scene/scene.h"

// NAMESPACE

// DEFINES
#define NUMBER_RAY_PER_DYNAMIC_VOXEL_FACE 128

// STATIC MEMBER INITIALIZATION

/////////////////////////////////////////////////////////////////////////////////////////////

DynamicLightBounceVoxelIrradianceTechnique::DynamicLightBounceVoxelIrradianceTechnique(string &&name, string&& className) : 
	  RasterTechnique(move(name), move(className))
	, m_lightBounceVoxelIrradianceTechnique(nullptr)
	, m_processCameraVisibleResultsTechnique(nullptr)
	, m_resetLitvoxelData(nullptr)
	, m_emitterCamera(nullptr)
	, m_materialLightBounceDynamicVoxelIrradiance(nullptr)
	, m_dynamicLightBounceDebugBuffer(nullptr)
	, m_sceneMinAndCameraVisibleVoxel(vec4(0.0f))
	, m_sceneExtentAndNumElement(vec4(0.0f))
	, m_voxelizationWidth(0.0f)
	, m_numVisibleDynamicVoxel(0)
	, m_dynamicLightBounceShaderBindingTableBuffer(nullptr)
	, m_shaderBindingTableBuilt(false)
{
	m_active                 = false;
	m_needsToRecord          = false;
	m_executeCommand         = false;
	m_rasterTechniqueType    = RasterTechniqueType::RTT_GRAPHICS;
	m_computeHostSynchronize = true;
}

/////////////////////////////////////////////////////////////////////////////////////////////

void DynamicLightBounceVoxelIrradianceTechnique::init()
{
	m_dynamicLightBounceDebugBuffer = bufferM->buildBuffer(
		move(string("dynamicLightBounceDebugBuffer")),
		nullptr,
		//356000,
		1800000,
		VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

	m_materialLightBounceDynamicVoxelIrradiance = static_cast<MaterialLightBounceDynamicVoxelIrradiance*>(materialM->buildMaterial(move(string("MaterialLightBounceDynamicVoxelIrradiance")), move(string("MaterialLightBounceDynamicVoxelIrradiance")), nullptr));

	m_vectorMaterialName.push_back("MaterialLightBounceDynamicVoxelIrradiance");
	m_vectorMaterial.push_back(m_materialLightBounceDynamicVoxelIrradiance);

	StaticSceneVoxelizationTechnique* technique = static_cast<StaticSceneVoxelizationTechnique*>(gpuPipelineM->getRasterTechniqueByName(move(string("StaticSceneVoxelizationTechnique"))));
	m_voxelizationWidth                         = float(technique->getVoxelizedSceneWidth());

	BBox3D& box = sceneM->refBox();

	vec3 max3D;
	vec3 min3D;
	box.getCenteredBoxMinMax(min3D, max3D);

	vec3 extent3D                   = max3D - min3D;
	m_sceneMinAndCameraVisibleVoxel = vec4(min3D.x, min3D.y, min3D.z, 0.0f);
	m_sceneExtentAndNumElement      = vec4(extent3D.x, extent3D.y, extent3D.z, 0.0f);

	m_emitterCamera = cameraM->getElement(move(string("emitter")));

	m_lightBounceVoxelIrradianceTechnique = static_cast<LightBounceVoxelIrradianceTechnique*>(gpuPipelineM->getRasterTechniqueByName(move(string("LightBounceVoxelIrradianceTechnique"))));
	m_lightBounceVoxelIrradianceTechnique->refSignalLightBounceVoxelIrradianceCompletion().connect<DynamicLightBounceVoxelIrradianceTechnique, &DynamicLightBounceVoxelIrradianceTechnique::slotLightBounceVoxelIrradianceCompleted>(this);

	m_processCameraVisibleResultsTechnique = static_cast<ProcessCameraVisibleResultsTechnique*>(gpuPipelineM->getRasterTechniqueByName(move(string("ProcessCameraVisibleResultsTechnique"))));

	m_resetLitvoxelData = static_cast<ResetLitvoxelData*>(gpuPipelineM->getRasterTechniqueByName(move(string("ResetLitvoxelData"))));

	const VkPhysicalDeviceRayTracingPipelinePropertiesKHR& physicalDeviceRayTracingPipelineProperties = coreM->getkPhysicalDeviceRayTracingPipelineProperties();

	m_shaderGroupBaseAlignment = physicalDeviceRayTracingPipelineProperties.shaderGroupBaseAlignment;
	m_shaderGroupHandleSize    = physicalDeviceRayTracingPipelineProperties.shaderGroupHandleSize;
	m_shaderGroupSizeAligned   = uint32_t((m_shaderGroupHandleSize + (m_shaderGroupBaseAlignment - 1)) & ~uint32_t(m_shaderGroupBaseAlignment - 1));
	m_shaderGroupStride        = m_shaderGroupSizeAligned;
}

/////////////////////////////////////////////////////////////////////////////////////////////

void DynamicLightBounceVoxelIrradianceTechnique::prepare(float dt)
{
	if (sceneM->getIsVectorNodeFrameUpdated())
	{
		// NOTE: The call to RasterTechnique::preRecordLoop() sets m_needsToRecord to false, reevaluate the role of that line of code
		//       "m_needsToRecord  = (m_vectorCommand.size() != m_usedCommandBufferNumber);"
		m_needsToRecord = true;
	}

	// TODO: This is a temporal solution for a special case where the dynamic scene elements are not moving
	//       It is pending to, in those cases, cache the already processed dynamic voxels to avoid recompute the same irradiance value.
	//       This only happens when all dynamic scene elements are not moving, or moving ones are far enough to from not moving ones to 
	//       consider those voxels from not moving dynamic ones not being affected
	if (m_numVisibleDynamicVoxel > 0)
	{
		m_needsToRecord = true;
	}

	// TODO: REMOVE ONCE THE CHANGES TO FORCE AL LIGHT BOUNCE COMPUTATIONS PER FRAME REGARDLESS OF ANY SCENE CHANGE ARE REMOVED.
	//m_needsToRecord = true;
}

/////////////////////////////////////////////////////////////////////////////////////////////

VkCommandBuffer* DynamicLightBounceVoxelIrradianceTechnique::record(int currentImage, uint& commandBufferID, CommandBufferType& commandBufferType)
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

	vkCmdBindPipeline(*commandBuffer, VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR, m_materialLightBounceDynamicVoxelIrradiance->getPipeline()->getPipeline());

	uint dynamicAllignment  = materialM->getMaterialUBDynamicAllignment();
	uint32_t materialOffset = static_cast<uint32_t>(m_materialLightBounceDynamicVoxelIrradiance->getMaterialUniformBufferIndex() * dynamicAllignment);

	uint offsetData[1] = { materialOffset };
	vkCmdBindDescriptorSets(*commandBuffer, VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR, m_materialLightBounceDynamicVoxelIrradiance->getPipelineLayout(), 0, 1, &m_materialLightBounceDynamicVoxelIrradiance->refDescriptorSet(), 1, &offsetData[0]);

	VkDeviceAddress shaderBindingTableDeviceAddress = m_dynamicLightBounceShaderBindingTableBuffer->getBufferDeviceAddress();
	
	vector<VkStridedDeviceAddressRegionKHR> vectorStridedDeviceAddressRegion(4);
	vectorStridedDeviceAddressRegion[0] = VkStridedDeviceAddressRegionKHR{ shaderBindingTableDeviceAddress + 0u * m_shaderGroupSizeAligned, m_shaderGroupStride, m_shaderGroupSizeAligned * 1 };
	vectorStridedDeviceAddressRegion[1] = VkStridedDeviceAddressRegionKHR{ shaderBindingTableDeviceAddress + 1u * m_shaderGroupSizeAligned, m_shaderGroupStride, m_shaderGroupSizeAligned * 1 };
	vectorStridedDeviceAddressRegion[2] = VkStridedDeviceAddressRegionKHR{ shaderBindingTableDeviceAddress + 2u * m_shaderGroupSizeAligned, m_shaderGroupStride, m_shaderGroupSizeAligned * 1 };
	vectorStridedDeviceAddressRegion[3] = {0, 0, 0};

	vkfpM->vkCmdTraceRaysKHR(*commandBuffer, &vectorStridedDeviceAddressRegion[0], &vectorStridedDeviceAddressRegion[1], &vectorStridedDeviceAddressRegion[2], &vectorStridedDeviceAddressRegion[3], m_numVisibleDynamicVoxel, 6, 128);

#ifdef USE_TIMESTAMP
	vkCmdWriteTimestamp(*commandBuffer, VK_PIPELINE_STAGE_RAY_TRACING_SHADER_BIT_KHR, coreM->getGraphicsQueueQueryPool(), m_queryIndex1);
#endif

	coreM->endCommandBuffer(*commandBuffer);

	// NOTE: Clear command buffer if re-recorded
	m_vectorCommand.push_back(commandBuffer);

	return commandBuffer;
}

/////////////////////////////////////////////////////////////////////////////////////////////

void DynamicLightBounceVoxelIrradianceTechnique::postCommandSubmit()
{
	m_executeCommand = false;
	m_active         = false;
	m_needsToRecord  = false;

	m_signalDynamicLightBounceVoxelIrradianceCompletion.emit();
}

/////////////////////////////////////////////////////////////////////////////////////////////

void DynamicLightBounceVoxelIrradianceTechnique::slotLightBounceVoxelIrradianceCompleted()
{
	if (!m_shaderBindingTableBuilt)
	{
		buildShaderBindingTable();
		m_shaderBindingTableBuilt = true;
	}

	// The elements to process are the dynamic camera visible voxels
	m_numVisibleDynamicVoxel = m_processCameraVisibleResultsTechnique->getDynamicCameraVisibleVoxelNumber();

	if (m_numVisibleDynamicVoxel > 0)
	{
		m_sceneMinAndCameraVisibleVoxel.w = float(m_numVisibleDynamicVoxel);
		m_sceneExtentAndNumElement.w      = float(m_numVisibleDynamicVoxel);
		vec3 emitterPosition              = m_emitterCamera->getPosition();
		vec3 emitterLookAt                = m_emitterCamera->getLookAt();

		m_materialLightBounceDynamicVoxelIrradiance->setSceneMinAndCameraVisibleVoxel(m_sceneMinAndCameraVisibleVoxel);
		m_materialLightBounceDynamicVoxelIrradiance->setSceneExtentAndNumElement(m_sceneExtentAndNumElement);
		m_materialLightBounceDynamicVoxelIrradiance->setLightPositionAndVoxelSize(vec4(emitterPosition.x, emitterPosition.y, emitterPosition.z, m_voxelizationWidth));
		m_materialLightBounceDynamicVoxelIrradiance->setLightForwardEmitterRadiance(vec4(emitterLookAt.x, emitterLookAt.y, emitterLookAt.z, float(gpuPipelineM->getRasterFlagValue(move(string("EMITTER_RADIANCE"))))));

		m_active        = true;
		m_needsToRecord = true;
	}
	else
	{
		m_resetLitvoxelData->setTechniqueLock(false);
	}
}

/////////////////////////////////////////////////////////////////////////////////////////////

void DynamicLightBounceVoxelIrradianceTechnique::buildShaderBindingTable()
{
	vector<VkRayTracingShaderGroupCreateInfoKHR>& vectorRayTracingShaderGroupCreateInfo = m_materialLightBounceDynamicVoxelIrradiance->refShader()->refVectorRayTracingShaderGroupCreateInfo();

	uint numberShaderGroup = static_cast<uint>(vectorRayTracingShaderGroupCreateInfo.size());
	uint shaderBindingTableSize = numberShaderGroup * m_shaderGroupSizeAligned;

	// Retrieve shader handles
	vector<uint8_t> vectorShaderHandleStorage(shaderBindingTableSize);
	VkResult result = vkfpM->vkGetRayTracingShaderGroupHandlesKHR(coreM->getLogicalDevice(), m_materialLightBounceDynamicVoxelIrradiance->getPipeline()->getPipeline(), 0, numberShaderGroup, shaderBindingTableSize, vectorShaderHandleStorage.data());

	assert(result == VK_SUCCESS);

	vector<uint8_t> vectorShaderBindingTableBuffer(shaderBindingTableSize);
	uint8_t* pVectorShaderBindingTableBuffer = static_cast<uint8_t*>(vectorShaderBindingTableBuffer.data());

	forI(numberShaderGroup)
	{
		memcpy(pVectorShaderBindingTableBuffer, vectorShaderHandleStorage.data() + i * m_shaderGroupHandleSize, m_shaderGroupHandleSize);
		pVectorShaderBindingTableBuffer += m_shaderGroupSizeAligned;
	}

	m_dynamicLightBounceShaderBindingTableBuffer = bufferM->buildBuffer(move(string("dynamicLightBounceShaderBindingTableBuffer")),
		                                                     (void*)vectorShaderBindingTableBuffer.data(),
		                                                     sizeof(uint8_t) * vectorShaderBindingTableBuffer.size(),
		                                                     VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_SHADER_BINDING_TABLE_BIT_KHR,
		                                                     //VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
															 VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
}

/////////////////////////////////////////////////////////////////////////////////////////////
