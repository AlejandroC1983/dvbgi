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
#include "../../include/rastertechnique/staticneighbourinformationtechnique.h"
#include "../../include/core/gpupipeline.h"
#include "../../include/material/materialstaticneighbourinformation.h"
#include "../../include/parameter/attributedefines.h"
#include "../../include/parameter/attributedata.h"
#include "../../include/texture/texture.h"
#include "../../include/texture/texturemanager.h"
#include "../../include/rastertechnique/voxelvisibilityraytracingtechnique.h"
#include "../../include/core/coremanager.h"
#include "../../include/buffer/buffermanager.h"
#include "../../include/buffer/buffer.h"
#include "../../include/material/materialbuildstaticvoxeltilebuffer.h"

// NAMESPACE
using namespace attributedefines;

// DEFINES

// STATIC MEMBER INITIALIZATION

/////////////////////////////////////////////////////////////////////////////////////////////

StaticNeighbourInformationTechnique::StaticNeighbourInformationTechnique(string&& name, string&& className) : BufferProcessTechnique(move(name), move(className))
	, m_voxelizationResolution(0)
	, m_staticVoxelNeighbourInfo(nullptr)
	, m_materialStaticNeighbourInformation(nullptr)
	, m_voxelVisibilityRayTracingTechnique(nullptr)
	, m_occupiedStaticVoxelTileBuffer(nullptr)
	, m_occupiedStaticVoxelTileCounterBuffer(nullptr)
	, m_occupiedStaticVoxelTileCounter(0)
	, m_tileSide(gpuPipelineM->getRasterFlagValue(move(string("TILE_SIDE"))))
	, m_materialBuildStaticVoxelTileBuffer(nullptr)
	, m_occupiedStaticDebugBuffer(nullptr)
{
	m_numElementPerLocalWorkgroupThread = 1;
	m_numThreadPerLocalWorkgroup        = 32;
	m_rasterTechniqueType               = RasterTechniqueType::RTT_COMPUTE;
	m_computeHostSynchronize            = true;
	m_voxelizationResolution            = gpuPipelineM->getRasterFlagValue(move(string("SCENE_VOXELIZATION_RESOLUTION")));
	m_bufferNumElement                  = m_voxelizationResolution * m_voxelizationResolution * m_voxelizationResolution;
	m_active                            = false;
	m_needsToRecord                     = false;
	m_executeCommand                    = false;
}

/////////////////////////////////////////////////////////////////////////////////////////////

void StaticNeighbourInformationTechnique::init()
{
	m_staticVoxelNeighbourInfo = textureM->buildTexture(
		move(string("staticVoxelNeighbourInfo")),
		VK_FORMAT_R32_UINT,
		VkExtent3D({ uint32_t(m_voxelizationResolution), uint32_t(m_voxelizationResolution), uint32_t(m_voxelizationResolution) }),
		VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
		VK_IMAGE_ASPECT_COLOR_BIT,
		VK_IMAGE_ASPECT_COLOR_BIT,
		VK_IMAGE_LAYOUT_UNDEFINED,
		VK_IMAGE_LAYOUT_GENERAL,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		VK_SAMPLE_COUNT_1_BIT,
		VK_IMAGE_TILING_OPTIMAL,
		VK_IMAGE_VIEW_TYPE_3D,
		// The option below is needed to build the image compatible with 2D array so the image view can be built as a 2D array with layers = texture dimension,
		// so it can be used as attachemnt in a framebuffer to be cleared
		VK_IMAGE_CREATE_2D_ARRAY_COMPATIBLE_BIT);

	int totalTileAmount = (m_voxelizationResolution * m_voxelizationResolution * m_voxelizationResolution) / (m_tileSide * m_tileSide * m_tileSide);

	m_occupiedStaticVoxelTileBuffer = bufferM->buildBuffer(
		move(string("occupiedStaticVoxelTileBuffer")),
		nullptr,
		totalTileAmount * sizeof(uint),
		VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

	m_occupiedStaticVoxelTileCounterBuffer = bufferM->buildBuffer(
		move(string("occupiedStaticVoxelTileCounterBuffer")),
		(void*)(&m_occupiedStaticVoxelTileCounter),
		4,
		VK_BUFFER_USAGE_STORAGE_BUFFER_BIT  | VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

	m_occupiedStaticDebugBuffer = bufferM->buildBuffer(
		move(string("occupiedStaticDebugBuffer")),
		nullptr,
		256,
		VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

	m_materialStaticNeighbourInformation = static_cast<MaterialStaticNeighbourInformation*>(materialM->buildMaterial(move(string("MaterialStaticNeighbourInformation")), move(string("MaterialStaticNeighbourInformation")), nullptr));
	m_materialStaticNeighbourInformation->obtainDispatchWorkGroupCount(m_bufferNumElement);
	m_materialStaticNeighbourInformation->setVoxelizationResolution(m_voxelizationResolution);

	m_materialBuildStaticVoxelTileBuffer = static_cast<MaterialBuildStaticVoxelTileBuffer*>(materialM->buildMaterial(move(string("MaterialBuildStaticVoxelTileBuffer")), move(string("MaterialBuildStaticVoxelTileBuffer")), nullptr));
	m_materialBuildStaticVoxelTileBuffer->obtainDispatchWorkGroupCount(totalTileAmount);
	m_materialBuildStaticVoxelTileBuffer->setVoxelizationResolution(m_voxelizationResolution);

	m_vectorMaterialName.push_back("MaterialStaticNeighbourInformation");
	m_vectorMaterialName.push_back("MaterialBuildStaticVoxelTileBuffer");

	m_vectorMaterial.push_back(m_materialStaticNeighbourInformation);
	m_vectorMaterial.push_back(m_materialBuildStaticVoxelTileBuffer);

	m_voxelVisibilityRayTracingTechnique = static_cast<VoxelVisibilityRayTracingTechnique*>(gpuPipelineM->getRasterTechniqueByName(move(string("VoxelVisibilityRayTracingTechnique"))));
	m_voxelVisibilityRayTracingTechnique->refSignalVoxelVisibilityRayTracing().connect<StaticNeighbourInformationTechnique, &StaticNeighbourInformationTechnique::slotVoxelVisibilityRayTracingComplete>(this);
}

/////////////////////////////////////////////////////////////////////////////////////////////

VkCommandBuffer* StaticNeighbourInformationTechnique::record(int currentImage, uint& commandBufferID, CommandBufferType& commandBufferType)
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

	uint32_t offsetData;
	vkCmdBindPipeline(*commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, m_materialStaticNeighbourInformation->getPipeline()->getPipeline());
	offsetData = static_cast<uint32_t>(m_materialStaticNeighbourInformation->getMaterialUniformBufferIndex() * dynamicAllignment);
	vkCmdBindDescriptorSets(*commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, m_materialStaticNeighbourInformation->getPipelineLayout(), 0, 1, &m_materialStaticNeighbourInformation->refDescriptorSet(), 1, &offsetData);
	vkCmdDispatch(*commandBuffer, m_materialStaticNeighbourInformation->getLocalWorkGroupsXDimension(), m_materialStaticNeighbourInformation->getLocalWorkGroupsYDimension(), 1); // Compute shader global workgroup https://www.khronos.org/registry/vulkan/specs/1.1-extensions/html/vkspec.html

	vkCmdBindPipeline(*commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, m_materialBuildStaticVoxelTileBuffer->getPipeline()->getPipeline());
	offsetData = static_cast<uint32_t>(m_materialBuildStaticVoxelTileBuffer->getMaterialUniformBufferIndex() * dynamicAllignment);
	vkCmdBindDescriptorSets(*commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, m_materialBuildStaticVoxelTileBuffer->getPipelineLayout(), 0, 1, &m_materialBuildStaticVoxelTileBuffer->refDescriptorSet(), 1, &offsetData);
	vkCmdDispatch(*commandBuffer, m_materialBuildStaticVoxelTileBuffer->getLocalWorkGroupsXDimension(), m_materialBuildStaticVoxelTileBuffer->getLocalWorkGroupsYDimension(), 1); // Compute shader global workgroup https://www.khronos.org/registry/vulkan/specs/1.1-extensions/html/vkspec.html

#ifdef USE_TIMESTAMP
	vkCmdWriteTimestamp(*commandBuffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, coreM->getComputeQueueQueryPool(), m_queryIndex1);
#endif

	recordBarriersEnd(commandBuffer);

	coreM->endCommandBuffer(*commandBuffer);
	
	m_vectorCommand.push_back(commandBuffer);

	return commandBuffer;
}

/////////////////////////////////////////////////////////////////////////////////////////////

void StaticNeighbourInformationTechnique::recordBarriersEnd(VkCommandBuffer* commandBuffer)
{
	VkImageSubresourceRange subresourceRange = {};
	subresourceRange.aspectMask   = VK_IMAGE_ASPECT_COLOR_BIT;
	subresourceRange.baseMipLevel = 0;
	subresourceRange.levelCount   = 1;
	subresourceRange.layerCount   = 1;

	textureM->setImageLayout(*commandBuffer, m_staticVoxelNeighbourInfo, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, subresourceRange, coreM->getComputeQueueIndex());
}

/////////////////////////////////////////////////////////////////////////////////////////////

void StaticNeighbourInformationTechnique::postCommandSubmit()
{
	m_executeCommand = false;
	m_active         = false;
	m_needsToRecord  = false;

	m_occupiedStaticVoxelTileCounterBuffer->getContent((void*)(&m_occupiedStaticVoxelTileCounter));

	cout << "StaticNeighbourInformationTechnique::postCommandSubmit: There are " << m_occupiedStaticVoxelTileCounter << " occupied tiles" << endl;

	m_signaStaticNeighbourInformationCompletion.emit();
}

/////////////////////////////////////////////////////////////////////////////////////////////

void StaticNeighbourInformationTechnique::slotVoxelVisibilityRayTracingComplete()
{
	m_active        = true;
	m_needsToRecord = true;
}

/////////////////////////////////////////////////////////////////////////////////////////////
