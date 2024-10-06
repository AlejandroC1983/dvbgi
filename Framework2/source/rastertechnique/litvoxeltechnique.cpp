/*
Copyright 2021 Alejandro Cosin

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
#include "../../include/rastertechnique/litvoxeltechnique.h"
#include "../../include/material/materialmanager.h"
#include "../../include/material/materiallitvoxel.h"
#include "../../include/core/coremanager.h"
#include "../../include/parameter/attributedefines.h"
#include "../../include/parameter/attributedata.h"
#include "../../include/scene/scene.h"
#include "../../include/rastertechnique/staticscenevoxelizationtechnique.h"
#include "../../include/camera/camera.h"
#include "../../include/camera/cameramanager.h"
#include "../../include/rastertechnique/distanceshadowmappingtechnique.h"
#include "../../include/util/bufferverificationhelper.h"
#include "../../include/rastertechnique/bufferprefixsumtechnique.h"
#include "../../include/uniformbuffer/uniformbuffer.h"
#include "../../include/rastertechnique/dynamicvoxelcopytobuffertechnique.h"
#include "../../include/rastertechnique/resetlitvoxeldatatechnique.h"
#include "../../include/material/materialneighbourlitvoxelinformation.h"
#include "../../include/util/vulkanstructinitializer.h"

// NAMESPACE
using namespace attributedefines;

// DEFINES

// STATIC MEMBER INITIALIZATION

/////////////////////////////////////////////////////////////////////////////////////////////

LitVoxelTechnique::LitVoxelTechnique(string &&name, string&& className) : BufferProcessTechnique(move(name), move(className))
	
	, m_litVoxelDebugBuffer(nullptr)
	, m_distanceShadowMappingTechnique(nullptr)
	, m_litVoxelTechnique(nullptr)
	, m_bufferPrefixSumTechnique(nullptr)
	, m_voxelShadowMappingCamera(nullptr)
	, m_prefixSumCompleted(false)
	, m_emitterRadiance(0.0f)
	, m_materialLitVoxel(nullptr)
	, m_dynamicVoxelCounter(0)
	, m_dynamicVoxelDispatchXDimension(0)
	, m_dynamicVoxelDispatchYDimension(0)
	, m_sceneDirty(false)
	, m_materialNeighbourLitVoxelInformation(nullptr)
	, m_neighbourLitVoxelDebugBuffer(nullptr)
	, m_neighbourLitVoxelInformationBuffer(nullptr)
{
	m_active                 = false;
	m_needsToRecord          = false;
	m_executeCommand         = false;

	// TODO: Add an enum for ray tracing and an potion to host syncronize results
	m_rasterTechniqueType    = RasterTechniqueType::RTT_COMPUTE;
	m_computeHostSynchronize = true;
}

/////////////////////////////////////////////////////////////////////////////////////////////

void LitVoxelTechnique::init()
{
	StaticSceneVoxelizationTechnique* technique = static_cast<StaticSceneVoxelizationTechnique*>(gpuPipelineM->getRasterTechniqueByName(move(string("StaticSceneVoxelizationTechnique"))));
	m_voxelizationWidth = technique->getVoxelizedSceneWidth();

	m_litVoxelDebugBuffer = bufferM->buildBuffer(
		move(string("litVoxelDebugBuffer")),
		nullptr,
		256,
		VK_BUFFER_USAGE_STORAGE_BUFFER_BIT  | VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

	// Assuming each thread will take care of a whole row / column
	m_material                             = materialM->buildMaterial(move(string("MaterialLitVoxel")), move(string("MaterialLitVoxel")), nullptr);
	m_materialNeighbourLitVoxelInformation = static_cast<MaterialNeighbourLitVoxelInformation*>(materialM->buildMaterial(move(string("MaterialNeighbourLitVoxelInformation")), move(string("MaterialNeighbourLitVoxelInformation")), nullptr));

	m_vectorMaterialName.push_back("MaterialLitVoxel");
	m_vectorMaterialName.push_back("MaterialNeighbourLitVoxelInformation");
	m_vectorMaterial.push_back(m_material);
	m_vectorMaterial.push_back(m_materialNeighbourLitVoxelInformation);

	BBox3D& box = sceneM->refBox();
	vec3 min3D;
	vec3 max3D;
	box.getCenteredBoxMinMax(min3D, max3D);

	vec3 extent3D = max3D - min3D;
	vec4 min      = vec4(min3D.x,    min3D.y,    min3D.z,    0.0f);
	vec4 max      = vec4(max3D.x,    max3D.y,    max3D.z,    0.0f);
	vec4 extent   = vec4(extent3D.x, extent3D.y, extent3D.z, 0.0f);

	m_materialLitVoxel = static_cast<MaterialLitVoxel*>(m_material);
	m_materialLitVoxel->setSceneMin(min);
	m_materialLitVoxel->setSceneMax(max);
	m_materialLitVoxel->setSceneExtent(extent);
	
	m_bufferPrefixSumTechnique = static_cast<BufferPrefixSumTechnique*>(gpuPipelineM->getRasterTechniqueByName(move(string("BufferPrefixSumTechnique"))));
	m_bufferPrefixSumTechnique->refPrefixSumComplete().connect<LitVoxelTechnique, &LitVoxelTechnique::slotPrefixSumComplete>(this);

	m_materialLitVoxel->setVoxelSize(float(m_voxelizationWidth));

	m_materialNeighbourLitVoxelInformation->setVoxelSize(float(m_voxelizationWidth));

	m_sceneMin = vec4(min3D.x, min3D.y, min3D.z, 0.0f);
	m_sceneExtentAndVoxelSize = vec4(extent3D.x, extent3D.y, extent3D.z, float(m_voxelizationWidth));

	m_neighbourLitVoxelDebugBuffer = bufferM->buildBuffer(
		move(string("neighbourLitVoxelDebugBuffer")),
		nullptr,
		4000000,
		VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

	m_neighbourLitVoxelInformationBuffer = bufferM->buildBuffer(
		move(string("neighbourLitVoxelInformationBuffer")),
		nullptr,
		256,
		VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

	sceneM->refSignalSceneDirtyNotification().connect<LitVoxelTechnique, &LitVoxelTechnique::slotSceneDirty>(this);

	ResetLitvoxelData* resetLitvoxelData = static_cast<ResetLitvoxelData*>(gpuPipelineM->getRasterTechniqueByName(move(string("ResetLitvoxelData"))));
	resetLitvoxelData->refSignalResetLitvoxelDataCompletion().connect<LitVoxelTechnique, &LitVoxelTechnique::slotDirtyUpdateValues>(this);
}

/////////////////////////////////////////////////////////////////////////////////////////////

void LitVoxelTechnique::prepare(float dt)
{
	Camera* shadowMappingCamera = cameraM->getElement(move(string("emitter")));
	mat4 viewprojectionMatrix   = shadowMappingCamera->getProjection() * shadowMappingCamera->getView();
	m_materialLitVoxel->setShadowViewProjection(viewprojectionMatrix);

	// TODO: Put this as a callback from scene when updating it
	if (sceneM->getIsVectorNodeFrameUpdated())
	{
		DynamicVoxelCopyToBufferTechnique* technique = static_cast<DynamicVoxelCopyToBufferTechnique*>(gpuPipelineM->getRasterTechniqueByName(move(string("DynamicVoxelCopyToBufferTechnique"))));
		m_dynamicVoxelCounter = technique->getDynamicVoxelCounter();
		
		BufferProcessTechnique::obtainDispatchWorkGroupCount(m_materialLitVoxel->getNumThreadPerLocalWorkgroup(),
															 m_materialLitVoxel->getNumElementPerLocalWorkgroupThread(),
															 m_dynamicVoxelCounter,
															 m_dynamicVoxelDispatchXDimension,
															 m_dynamicVoxelDispatchYDimension);

		// NOTE: The call to RasterTechnique::preRecordLoop() sets m_needsToRecord to false, reevaluate the role of that line of code
		//       "m_needsToRecord  = (m_vectorCommand.size() != m_usedCommandBufferNumber);"
		m_needsToRecord = true;
	}

	// TODO: Add logic to update only when needed
	m_cameraPosition  = m_voxelShadowMappingCamera->getPosition();
	vec3 lightForward = m_voxelShadowMappingCamera->getLookAt();

	m_materialLitVoxel->setLightPosition(vec4(m_cameraPosition.x, m_cameraPosition.y, m_cameraPosition.z, 0.0f));
	m_materialLitVoxel->setLightForward(vec4(lightForward.x, lightForward.y, lightForward.z, 0.0f));
}

/////////////////////////////////////////////////////////////////////////////////////////////

VkCommandBuffer* LitVoxelTechnique::record(int currentImage, uint& commandBufferID, CommandBufferType& commandBufferType)
{
	commandBufferType = CommandBufferType::CBT_COMPUTE_QUEUE;

	VkCommandBuffer* commandBuffer;
	commandBuffer = addRecordedCommandBuffer(commandBufferID);
	addCommandBufferQueueType(commandBufferID, commandBufferType);
	coreM->allocCommandBuffer(&coreM->getLogicalDevice(), coreM->getComputeCommandPool(), commandBuffer);
	coreM->setObjectName(uint64_t(*commandBuffer), VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT, getName());

	VkImageSubresourceRange subresourceRange = {};
	subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	subresourceRange.baseMipLevel = 0;
	subresourceRange.levelCount = 1;
	subresourceRange.layerCount = 1;

	coreM->beginCommandBuffer(*commandBuffer);

	// This might need to be done for the scene distance shadow map, was commented when stopped using the voxel shadow map
	//textureM->setImageLayout(*commandBuffer, texture->getImage(), VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, subresourceRange, coreM->getComputeQueueIndex());

#ifdef USE_TIMESTAMP
	vkCmdResetQueryPool(*commandBuffer, coreM->getComputeQueueQueryPool(), m_queryIndex0, 1);
	vkCmdResetQueryPool(*commandBuffer, coreM->getComputeQueueQueryPool(), m_queryIndex1, 1);
	vkCmdWriteTimestamp(*commandBuffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, coreM->getComputeQueueQueryPool(), m_queryIndex0);
#endif

	uint32_t offsetData;

	uint dynamicAllignment = materialM->getMaterialUBDynamicAllignment();

	m_materialLitVoxel->setPushConstantIsTestingStaticVoxel(vec4(1.0f, float(m_bufferNumElement), 0.0f, 0.0f));
	m_materialLitVoxel->updatePushConstantCPUBuffer();
	CPUBuffer& cpuBufferNewCenter = m_materialLitVoxel->refShader()->refPushConstant().refCPUBuffer();

	vkCmdPushConstants(
		*commandBuffer,
		m_materialLitVoxel->getPipelineLayout(),
		VK_SHADER_STAGE_COMPUTE_BIT,
		0,
		uint32_t(m_materialLitVoxel->getPushConstantExposedStructFieldSize()),
		cpuBufferNewCenter.refUBHostMemory());

	vkCmdBindPipeline(*commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, m_materialLitVoxel->getPipeline()->getPipeline());
	offsetData = static_cast<uint32_t>(m_materialLitVoxel->getMaterialUniformBufferIndex() * dynamicAllignment);
	vkCmdBindDescriptorSets(*commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, m_materialLitVoxel->getPipelineLayout(), 0, 1, &m_materialLitVoxel->refDescriptorSet(), 1, &offsetData);

	// First dispatch for the static voxles in the scene, present in voxelHashedPositionCompactedBuffer
	vkCmdDispatch(*commandBuffer, m_materialLitVoxel->getLocalWorkGroupsXDimension(), m_materialLitVoxel->getLocalWorkGroupsYDimension(), 1); // Compute shader global workgroup https://www.khronos.org/registry/vulkan/specs/1.1-extensions/html/vkspec.html

	// Second dispatch for the dynamic voxels in the scene, present in dynamicVoxelBuffer 
	m_materialLitVoxel->setPushConstantIsTestingStaticVoxel(vec4(0.0f, float(m_dynamicVoxelCounter), 0.0f, 0.0f));
	m_materialLitVoxel->updatePushConstantCPUBuffer();

	vkCmdPushConstants(
		*commandBuffer,
		m_materialLitVoxel->getPipelineLayout(),
		VK_SHADER_STAGE_COMPUTE_BIT,
		0,
		uint32_t(m_materialLitVoxel->getPushConstantExposedStructFieldSize()),
		cpuBufferNewCenter.refUBHostMemory());

	vkCmdDispatch(*commandBuffer, m_dynamicVoxelDispatchXDimension, m_dynamicVoxelDispatchYDimension, 1); // Compute shader global workgroup https://www.khronos.org/registry/vulkan/specs/1.1-extensions/html/vkspec.html

	// Dispatch to fill the information in m_neighbourLitVoxelInformationBuffer
	VulkanStructInitializer::insertBufferMemoryBarrier(bufferM->getElement(move(string("litTestVoxelPerByteBuffer"))),
													   VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT,
													   VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT,
													   VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
													   VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
													   commandBuffer);

	vkCmdBindPipeline(*commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, m_materialNeighbourLitVoxelInformation->getPipeline()->getPipeline());

	offsetData = static_cast<uint32_t>(m_materialNeighbourLitVoxelInformation->getMaterialUniformBufferIndex() * dynamicAllignment);
	vkCmdBindDescriptorSets(*commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, m_materialNeighbourLitVoxelInformation->getPipelineLayout(), 0, 1, &m_materialNeighbourLitVoxelInformation->refDescriptorSet(), 1, &offsetData);
	vkCmdDispatch(*commandBuffer, m_materialNeighbourLitVoxelInformation->getLocalWorkGroupsXDimension(), m_materialNeighbourLitVoxelInformation->getLocalWorkGroupsYDimension(), 1); // Compute shader global workgroup https://www.khronos.org/registry/vulkan/specs/1.1-extensions/html/vkspec.html

#ifdef USE_TIMESTAMP
	vkCmdWriteTimestamp(*commandBuffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, coreM->getComputeQueueQueryPool(), m_queryIndex1);
#endif

	coreM->endCommandBuffer(*commandBuffer);

	// NOTE: Clear command buffer if re-recorded
	m_vectorCommand.push_back(commandBuffer);

	return commandBuffer;
}

/////////////////////////////////////////////////////////////////////////////////////////////

void LitVoxelTechnique::postCommandSubmit()
{
	m_executeCommand = false;
	m_active         = false;
	m_needsToRecord  = false;

	m_sceneDirty = false;

	m_signalLitVoxelCompletion.emit();
}

/////////////////////////////////////////////////////////////////////////////////////////////

void LitVoxelTechnique::slotPrefixSumComplete()
{
	m_bufferNumElement = m_bufferPrefixSumTechnique->getFirstIndexOccupiedElement();

	m_materialLitVoxel->obtainDispatchWorkGroupCount(m_bufferNumElement);

	uint numThreadPerLocalWorkgroup   = m_materialLitVoxel->getNumThreadPerLocalWorkgroup();
	uint numThreadToInitializeBuffers = m_materialLitVoxel->getLocalWorkGroupsXDimension() * m_materialLitVoxel->getLocalWorkGroupsYDimension() * numThreadPerLocalWorkgroup;

	// Make sure the amount of threads dispatched to test lit voxels for the static voxels, is enough to initialize the values of
	// both litTextVoxelPerByteBuffer and litTestDynamicVoxelPerByteBuffer buffers
	assert((numThreadToInitializeBuffers * numThreadPerLocalWorkgroup) > ((m_voxelizationWidth * m_voxelizationWidth * m_voxelizationWidth) / numThreadPerLocalWorkgroup));

	bufferM->resize(m_litVoxelDebugBuffer, nullptr, 250 * 60000 * sizeof(uint));

	m_voxelShadowMappingCamera = cameraM->getElement(move(string("emitter")));
	mat4 viewprojectionMatrix  = m_voxelShadowMappingCamera->getProjection() * m_voxelShadowMappingCamera->getView();
	m_materialLitVoxel->setShadowViewProjection(viewprojectionMatrix);

	m_prefixSumCompleted = true;

	m_voxelShadowMappingCamera->refCameraDirtySignal().connect<LitVoxelTechnique, &LitVoxelTechnique::slotCameraDirty>(this);

	m_materialNeighbourLitVoxelInformation->obtainDispatchWorkGroupCount(m_bufferNumElement);
	m_materialNeighbourLitVoxelInformation->setNumElementToProcess(m_bufferNumElement);
	bufferM->resize(m_neighbourLitVoxelInformationBuffer, nullptr, m_bufferNumElement * sizeof(uint));

	m_newPassRequested = true;
	m_active           = true;
	m_needsToRecord    = true;
}

/////////////////////////////////////////////////////////////////////////////////////////////

void LitVoxelTechnique::slotCameraDirty()
{

}

/////////////////////////////////////////////////////////////////////////////////////////////

void LitVoxelTechnique::slotSceneDirty()
{
	m_sceneDirty = true;
}

/////////////////////////////////////////////////////////////////////////////////////////////

void LitVoxelTechnique::slotDirtyUpdateValues()
{
	// This callback is for the shadow map camera, execute as well if the scene is dirty (ideally, if any dynamic scene element
	// can affect the irradiance in camera, maybe just in case any dynamic scene element is in camera or quite close to the frustum?
	if (m_prefixSumCompleted)
	{
		m_cameraPosition  = m_voxelShadowMappingCamera->getPosition();
		m_cameraForward   = m_voxelShadowMappingCamera->getLookAt();
		m_emitterRadiance = float(gpuPipelineM->getRasterFlagValue(move(string("EMITTER_RADIANCE"))));
		vec3 lightForward = m_voxelShadowMappingCamera->getLookAt();

		m_materialLitVoxel->setLightPosition(vec4(m_cameraPosition.x, m_cameraPosition.y, m_cameraPosition.z, 0.0f));
		m_materialLitVoxel->setLightForward(vec4(lightForward.x, lightForward.y, lightForward.z, 0.0f));

		m_active         = true;
		m_executeCommand = true;

		if(m_sceneDirty)
		{
			m_needsToRecord = true;
		}
	}
}

/////////////////////////////////////////////////////////////////////////////////////////////
