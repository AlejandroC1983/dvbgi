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
#include "../../include/rastertechnique/processcameravisibleresultstechnique.h"
#include "../../include/material/materialmanager.h"
#include "../../include/core/gpupipeline.h"
#include "../../include/rastertechnique/bufferprefixsumtechnique.h"
#include "../../include/rastertechnique/raytracingdeferredshadowstechnique.h"
#include "../../include/material/materialprocesscameravisibleresults.h"
#include "../../include/material/materialtagcameravisiblevoxelneighbourtile.h"
#include "../../include/rastertechnique/staticscenevoxelizationtechnique.h"
#include "../../include/util/vulkanstructinitializer.h"
#include "../../include/material/materialtemporalfilteringcleandynamic3dtextures.h"

// NAMESPACE

// DEFINES

// STATIC MEMBER INITIALIZATION

/////////////////////////////////////////////////////////////////////////////////////////////

ProcessCameraVisibleResultsTechnique::ProcessCameraVisibleResultsTechnique(string &&name, string&& className) : BufferProcessTechnique(move(name), move(className))
	, m_prefixSumCompleted(false)
	, m_rayTracingDeferredShadowsTechnique(nullptr)
	, m_staticVoxelVisibleCounterBuffer(nullptr)
	, m_dynamicVoxelVisibleCounterBuffer(nullptr)
	, m_staticCameraVisibleVoxelNumber(0)
	, m_dynamicCameraVisibleVoxelNumber(0)
	, m_processCameraVisibleResultsDebugBuffer(nullptr)
	, m_irradiancePaddingTagTilesCounterBuffer(nullptr)
	, m_irradianceFilteringTagTilesCounterBuffer(nullptr)
	, m_irradiancePaddingTagTilesIndexBuffer(nullptr)
	, m_irradianceFilteringTagTilesIndexBuffer(nullptr)
	, m_tagCameraVisibleDebugBuffer(nullptr)
	, m_dynamicIrradianceTrackingBuffer(nullptr)
	, m_staticIrradianceTrackingBuffer(nullptr)
	, m_dynamicIrradianceTrackingDebugBuffer(nullptr)
	, m_dynamicIrradianceTrackingCounterDebugBuffer(nullptr)
	, m_irradiancePaddingTagTilesNumber(0)
	, m_irradianceFilteringTagTilesNumber(0)
	, m_materialProcessCameraVisibleResults(nullptr)
	, m_materialTagCameraVisibleVoxelNeighbourTile(nullptr)
	, m_materialTemporalFilteringCleanDynamic3DTextures(nullptr)
{
	m_numElementPerLocalWorkgroupThread = 1;
	m_numThreadPerLocalWorkgroup        = 32;
	m_active                            = false;
	m_needsToRecord                     = false;
	m_executeCommand                    = false;
	m_rasterTechniqueType               = RasterTechniqueType::RTT_COMPUTE;
	m_computeHostSynchronize            = true;
}

/////////////////////////////////////////////////////////////////////////////////////////////

void ProcessCameraVisibleResultsTechnique::init()
{
	StaticSceneVoxelizationTechnique* technique = static_cast<StaticSceneVoxelizationTechnique*>(gpuPipelineM->getRasterTechniqueByName(move(string("StaticSceneVoxelizationTechnique"))));
	int voxelizationWidth                       = technique->getVoxelizedSceneWidth();
	int totalNumberVoxel                        = voxelizationWidth * voxelizationWidth * voxelizationWidth;

	uint tempInitValue = 0;

	m_staticVoxelVisibleCounterBuffer = bufferM->buildBuffer(
		move(string("staticVoxelVisibleCounterBuffer")),
		(void*)(&tempInitValue),
		4,
		VK_BUFFER_USAGE_STORAGE_BUFFER_BIT  | VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

	m_dynamicVoxelVisibleCounterBuffer = bufferM->buildBuffer(
		move(string("dynamicVoxelVisibleCounterBuffer")),
		(void*)(&tempInitValue),
		4,
		VK_BUFFER_USAGE_STORAGE_BUFFER_BIT  | VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

	m_processCameraVisibleResultsDebugBuffer = bufferM->buildBuffer(
		move(string("processCameraVisibleResultsDebugBuffer")),
		nullptr,
		300000000,
		VK_BUFFER_USAGE_STORAGE_BUFFER_BIT  | VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

	m_irradiancePaddingTagTilesCounterBuffer = bufferM->buildBuffer(
		move(string("irradiancePaddingTagTilesCounterBuffer")),
		(void*)(&tempInitValue),
		4,
		VK_BUFFER_USAGE_STORAGE_BUFFER_BIT  | VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

	m_irradianceFilteringTagTilesCounterBuffer = bufferM->buildBuffer(
		move(string("irradianceFilteringTagTilesCounterBuffer")),
		(void*)(&tempInitValue),
		4,
		VK_BUFFER_USAGE_STORAGE_BUFFER_BIT  | VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

	int tileSize = gpuPipelineM->getRasterFlagValue(move(string("TILE_SIZE_TOTAL")));

	m_irradiancePaddingTagTilesIndexBuffer = bufferM->buildBuffer(
		move(string("irradiancePaddingTagTilesIndexBuffer")),
		nullptr,
		(totalNumberVoxel * sizeof(uint)) / tileSize, // Buffer size to allow all tiles's indices to be stored, current tile size is 2^3 voxels
		VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

	m_irradianceFilteringTagTilesIndexBuffer = bufferM->buildBuffer(
		move(string("irradianceFilteringTagTilesIndexBuffer")),
		nullptr,
		(totalNumberVoxel * sizeof(uint)) / tileSize, // Buffer size to allow all tiles's indices to be stored, current tile size is 2^3 voxels
		VK_BUFFER_USAGE_STORAGE_BUFFER_BIT  | VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

	m_tagCameraVisibleDebugBuffer = bufferM->buildBuffer(
		move(string("tagCameraVisibleDebugBuffer")),
		nullptr,
		256,
		VK_BUFFER_USAGE_STORAGE_BUFFER_BIT  | VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

	// One byte per voxelization voxel
	vectorUint8 vectorData(totalNumberVoxel);

	m_dynamicIrradianceTrackingBuffer = bufferM->buildBuffer(
		move(string("dynamicIrradianceTrackingBuffer")),
		vectorData.data(),
		vectorData.size(),
		VK_BUFFER_USAGE_STORAGE_BUFFER_BIT  | VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

	m_staticIrradianceTrackingBuffer = bufferM->buildBuffer(
		move(string("staticIrradianceTrackingBuffer")),
		vectorData.data(),
		vectorData.size(),
		VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

	m_dynamicIrradianceTrackingDebugBuffer = bufferM->buildBuffer(
		move(string("dynamicIrradianceTrackingDebugBuffer")),
		nullptr,
		2500000,
		VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

	m_dynamicIrradianceTrackingCounterDebugBuffer = bufferM->buildBuffer(
		move(string("dynamicIrradianceTrackingCounterDebugBuffer")),
		(void*)(&tempInitValue),
		4,
		VK_BUFFER_USAGE_STORAGE_BUFFER_BIT  | VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

	m_materialProcessCameraVisibleResults = static_cast<MaterialProcessCameraVisibleResults*>(materialM->buildMaterial(move(string("MaterialProcessCameraVisibleResults")), move(string("MaterialProcessCameraVisibleResults")), nullptr));

	// Size of litTextVoxelPerByteBuffer and litTestDynamicVoxelPerByteBuffer considering uints to init each uint to 0
	m_bufferNumElement = (totalNumberVoxel / 8);
	m_materialProcessCameraVisibleResults->setNumberElementToProcess(m_bufferNumElement);
	m_materialProcessCameraVisibleResults->obtainDispatchWorkGroupCount(m_bufferNumElement);

	m_materialTagCameraVisibleVoxelNeighbourTile = static_cast<MaterialTagCameraVisibleVoxelNeighbourTile*>(materialM->buildMaterial(move(string("MaterialTagCameraVisibleVoxelNeighbourTile")), move(string("MaterialTagCameraVisibleVoxelNeighbourTile")), nullptr));
	m_materialTagCameraVisibleVoxelNeighbourTile->setNumberElementToProcess(m_bufferNumElement);
	m_materialTagCameraVisibleVoxelNeighbourTile->obtainDispatchWorkGroupCount(m_bufferNumElement);
	m_materialTagCameraVisibleVoxelNeighbourTile->setVoxelizationSize(voxelizationWidth);

	m_materialTemporalFilteringCleanDynamic3DTextures = static_cast<MaterialTemporalFilteringCleanDynamic3DTextures*>(materialM->buildMaterial(move(string("MaterialTemporalFilteringCleanDynamic3DTextures")), move(string("MaterialTemporalFilteringCleanDynamic3DTextures")), nullptr));
	m_materialTemporalFilteringCleanDynamic3DTextures->setNumberElementToProcess(totalNumberVoxel);
	m_materialTemporalFilteringCleanDynamic3DTextures->obtainDispatchWorkGroupCount(totalNumberVoxel);
	m_materialTemporalFilteringCleanDynamic3DTextures->setVoxelizationSize(voxelizationWidth);
	
	m_vectorMaterialName.push_back("MaterialProcessCameraVisibleResults");
	m_vectorMaterialName.push_back("MaterialTagCameraVisibleVoxelNeighbourTile");
	m_vectorMaterialName.push_back("MaterialTemporalFilteringCleanDynamic3DTextures");
	m_vectorMaterial.push_back(m_materialProcessCameraVisibleResults);
	m_vectorMaterial.push_back(m_materialTagCameraVisibleVoxelNeighbourTile);
	m_vectorMaterial.push_back(m_materialTemporalFilteringCleanDynamic3DTextures);
	
	m_rayTracingDeferredShadowsTechnique = static_cast<RayTracingDeferredShadowsTechnique*>(gpuPipelineM->getRasterTechniqueByName(move(string("RayTracingDeferredShadowsTechnique"))));
	m_rayTracingDeferredShadowsTechnique->refSignalRayTracingDeferredShadows().connect<ProcessCameraVisibleResultsTechnique, &ProcessCameraVisibleResultsTechnique::slotCameraVisibleRayTracingComplete>(this);
}

/////////////////////////////////////////////////////////////////////////////////////////////

void ProcessCameraVisibleResultsTechnique::prepare(float dt)
{
	// TODO: Put this information in the scene camera uniform buffer
	m_materialTemporalFilteringCleanDynamic3DTextures->setElapsedTime(dt * 1000.0f);
}

/////////////////////////////////////////////////////////////////////////////////////////////

VkCommandBuffer* ProcessCameraVisibleResultsTechnique::record(int currentImage, uint& commandBufferID, CommandBufferType& commandBufferType)
{
	commandBufferType = CommandBufferType::CBT_COMPUTE_QUEUE;

	VkCommandBuffer* commandBuffer;
	commandBuffer = addRecordedCommandBuffer(commandBufferID);
	addCommandBufferQueueType(commandBufferID, commandBufferType);
	coreM->allocCommandBuffer(&coreM->getLogicalDevice(), coreM->getComputeCommandPool(), commandBuffer);
	coreM->setObjectName(uint64_t(*commandBuffer), VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT, getName());

	coreM->beginCommandBuffer(*commandBuffer);

#ifdef USE_TIMESTAMP
	vkCmdResetQueryPool(*commandBuffer, coreM->getComputeQueueQueryPool(), m_queryIndex0, 1);
	vkCmdResetQueryPool(*commandBuffer, coreM->getComputeQueueQueryPool(), m_queryIndex1, 1);
	vkCmdWriteTimestamp(*commandBuffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, coreM->getComputeQueueQueryPool(), m_queryIndex0);
#endif

	recordBarriers(commandBuffer);

	uint dynamicAllignment = materialM->getMaterialUBDynamicAllignment();

	uint32_t offsetData[2];

	VulkanStructInitializer::insertBufferMemoryBarrier(bufferM->getElement(move(string("cameraVisibleDynamicVoxelPerByteBuffer"))),
													   VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT,
													   VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT,
													   VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
													   VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
													   commandBuffer);

	vkCmdBindPipeline(*commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, m_materialTagCameraVisibleVoxelNeighbourTile->getPipeline()->getPipeline());
	offsetData[0] = static_cast<uint32_t>(m_materialTagCameraVisibleVoxelNeighbourTile->getMaterialUniformBufferIndex() * dynamicAllignment);
	vkCmdBindDescriptorSets(*commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, m_materialTagCameraVisibleVoxelNeighbourTile->getPipelineLayout(), 0, 1, &m_materialTagCameraVisibleVoxelNeighbourTile->refDescriptorSet(), 1, &offsetData[0]);
	vkCmdDispatch(*commandBuffer, m_materialTagCameraVisibleVoxelNeighbourTile->getLocalWorkGroupsXDimension(), m_materialTagCameraVisibleVoxelNeighbourTile->getLocalWorkGroupsYDimension(), 1); // Compute shader global workgroup https://www.khronos.org/registry/vulkan/specs/1.1-extensions/html/vkspec.html

	VulkanStructInitializer::insertBufferMemoryBarrier(bufferM->getElement(move(string("irradianceFilteringTagTilesBuffer"))),
													   VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT,
													   VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT,
													   VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
													   VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
													   commandBuffer);

	VulkanStructInitializer::insertBufferMemoryBarrier(bufferM->getElement(move(string("irradiancePaddingTagTilesBuffer"))),
													   VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT,
													   VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT,
													   VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
													   VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
													   commandBuffer);

	VulkanStructInitializer::insertBufferMemoryBarrier(bufferM->getElement(move(string("cameraVisibleDynamicVoxelPerByteTagProcessBuffer"))),
													   VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT,
													   VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT,
													   VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
													   VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
													   commandBuffer);

	VulkanStructInitializer::insertBufferMemoryBarrier(m_dynamicIrradianceTrackingBuffer,
													   VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT,
													   VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT,
													   VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
													   VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
													   commandBuffer);

	VulkanStructInitializer::insertBufferMemoryBarrier(m_staticIrradianceTrackingBuffer,
													   VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT,
													   VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT,
													   VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
													   VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
													   commandBuffer);

	// Use the results in cameraVisibleDynamicVoxelPerByteTagProcessBuffer to update the cool down / fading of the 
	vkCmdBindPipeline(*commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, m_materialTemporalFilteringCleanDynamic3DTextures->getPipeline()->getPipeline());
	offsetData[0] = 0;
	offsetData[1] = static_cast<uint32_t>(m_materialTemporalFilteringCleanDynamic3DTextures->getMaterialUniformBufferIndex() * dynamicAllignment);
	vkCmdBindDescriptorSets(*commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, m_materialTemporalFilteringCleanDynamic3DTextures->getPipelineLayout(), 0, 1, &m_materialTemporalFilteringCleanDynamic3DTextures->refDescriptorSet(), 2, &offsetData[0]);
	vkCmdDispatch(*commandBuffer, m_materialTemporalFilteringCleanDynamic3DTextures->getLocalWorkGroupsXDimension(), m_materialTemporalFilteringCleanDynamic3DTextures->getLocalWorkGroupsYDimension(), 1); // Compute shader global workgroup https://www.khronos.org/registry/vulkan/specs/1.1-extensions/html/vkspec.html

	vkCmdBindPipeline(*commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, m_materialProcessCameraVisibleResults->getPipeline()->getPipeline());
	offsetData[0] = static_cast<uint32_t>(m_materialProcessCameraVisibleResults->getMaterialUniformBufferIndex() * dynamicAllignment);
	vkCmdBindDescriptorSets(*commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, m_materialProcessCameraVisibleResults->getPipelineLayout(), 0, 1, &m_materialProcessCameraVisibleResults->refDescriptorSet(), 1, &offsetData[0]);
	vkCmdDispatch(*commandBuffer, m_materialProcessCameraVisibleResults->getLocalWorkGroupsXDimension(), m_materialProcessCameraVisibleResults->getLocalWorkGroupsYDimension(), 1); // Compute shader global workgroup https://www.khronos.org/registry/vulkan/specs/1.1-extensions/html/vkspec.html

#ifdef USE_TIMESTAMP
	vkCmdWriteTimestamp(*commandBuffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, coreM->getComputeQueueQueryPool(), m_queryIndex1);
#endif

	recordBarriersEnd(commandBuffer);

	coreM->endCommandBuffer(*commandBuffer);
	
	m_vectorCommand.push_back(commandBuffer);

	return commandBuffer;
}

/////////////////////////////////////////////////////////////////////////////////////////////

void ProcessCameraVisibleResultsTechnique::postCommandSubmit()
{
	m_executeCommand = false;
	m_active         = false;
	m_needsToRecord  = false;

	uint tempUint = 0;
	m_staticVoxelVisibleCounterBuffer->getContent((void*)(&m_staticCameraVisibleVoxelNumber));
	m_staticVoxelVisibleCounterBuffer->setContentHostVisible(&tempUint);

	m_dynamicVoxelVisibleCounterBuffer->getContent((void*)(&m_dynamicCameraVisibleVoxelNumber));
	m_dynamicVoxelVisibleCounterBuffer->setContentHostVisible(&tempUint);

	m_irradiancePaddingTagTilesCounterBuffer->getContent((void*)(&m_irradiancePaddingTagTilesNumber));
	m_irradiancePaddingTagTilesCounterBuffer->setContentHostVisible(&tempUint);

	m_irradianceFilteringTagTilesCounterBuffer->getContent((void*)(&m_irradianceFilteringTagTilesNumber));
	m_irradianceFilteringTagTilesCounterBuffer->setContentHostVisible(&tempUint);

	m_signalProcessCameraVisibleCompletion.emit();
}

/////////////////////////////////////////////////////////////////////////////////////////////

void ProcessCameraVisibleResultsTechnique::slotCameraVisibleRayTracingComplete()
{
	m_active         = true;
	m_executeCommand = true;
}

/////////////////////////////////////////////////////////////////////////////////////////////
