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
#include "../../include/rastertechnique/dynamicvoxelcopytobuffertechnique.h"
#include "../../include/core/gpupipeline.h"
#include "../../include/material/materialdynamicvoxelcopytobuffer.h"
#include "../../include/material/materialmanager.h"
#include "../../include/parameter/attributedefines.h"
#include "../../include/parameter/attributedata.h"
#include "../../include/util/vulkanstructinitializer.h"
#include "../../include/util/bufferverificationhelper.h"
#include "../../include/rastertechnique/resetlitvoxeldatatechnique.h"

// NAMESPACE
using namespace attributedefines;

// DEFINES

// STATIC MEMBER INITIALIZATION

/////////////////////////////////////////////////////////////////////////////////////////////

DynamicVoxelCopyToBufferTechnique::DynamicVoxelCopyToBufferTechnique(string&& name, string&& className) : BufferProcessTechnique(move(name), move(className))
	, m_voxelizationResolution(0)
	, m_materialDynamicVoxelCopyToBuffer(nullptr)
	, m_dynamicVoxelCounterBuffer(nullptr)
	, m_dynamicVoxelCounter(0)
{
	m_numElementPerLocalWorkgroupThread = 1;
	m_numThreadPerLocalWorkgroup        = 32;
	m_rasterTechniqueType               = RasterTechniqueType::RTT_COMPUTE;
	m_computeHostSynchronize            = true;
	m_voxelizationResolution            = gpuPipelineM->getRasterFlagValue(move(string("SCENE_VOXELIZATION_RESOLUTION")));
	m_bufferNumElement                  = m_voxelizationResolution * m_voxelizationResolution * m_voxelizationResolution;

	m_active                 = false;
	m_needsToRecord          = false;
	m_executeCommand         = false;
}

/////////////////////////////////////////////////////////////////////////////////////////////

void DynamicVoxelCopyToBufferTechnique::init()
{
	m_dynamicVoxelCounterBuffer = bufferM->buildBuffer(
		move(string("dynamicVoxelCounterBuffer")),
		(void*)(&m_dynamicVoxelCounter),
		4,
		VK_BUFFER_USAGE_STORAGE_BUFFER_BIT  | VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

	m_dynamicvoxelCopyDebugBuffer = bufferM->buildBuffer(
		move(string("dynamicvoxelCopyDebugBuffer")),
		nullptr,
		32768 * 260 * 4, // 32.5MB
		VK_BUFFER_USAGE_STORAGE_BUFFER_BIT  | VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

	// Assuming each thread will take care of a whole row / column
	buildShaderThreadMapping();

	obtainDispatchWorkGroupCount();

	MultiTypeUnorderedMap* attributeMaterial = new MultiTypeUnorderedMap();
	attributeMaterial->newElement<AttributeData<string>*>(new AttributeData<string>(string(g_shaderCodeChunk), string(m_computeShaderThreadMapping)));
	m_material = materialM->buildMaterial(move(string("MaterialDynamicVoxelCopyToBuffer")), move(string("MaterialDynamicVoxelCopyToBuffer")), attributeMaterial);
	m_materialDynamicVoxelCopyToBuffer = static_cast<MaterialDynamicVoxelCopyToBuffer*>(m_material);
	m_vectorMaterialName.push_back("MaterialDynamicVoxelCopyToBuffer");
	m_vectorMaterial.push_back(m_material);

	m_materialDynamicVoxelCopyToBuffer = static_cast<MaterialDynamicVoxelCopyToBuffer*>(m_vectorMaterial[0]);
	m_materialDynamicVoxelCopyToBuffer->setLocalWorkGroupsXDimension(m_localWorkGroupsXDimension);
	m_materialDynamicVoxelCopyToBuffer->setLocalWorkGroupsYDimension(m_localWorkGroupsYDimension);
	m_materialDynamicVoxelCopyToBuffer->setNumThreadExecuted(m_numThreadExecuted);

	m_resetLitvoxelData = static_cast<ResetLitvoxelData*>(gpuPipelineM->getRasterTechniqueByName(move(string("ResetLitvoxelData"))));
	m_resetLitvoxelData->refSignalResetLitvoxelDataCompletion().connect<DynamicVoxelCopyToBufferTechnique, &DynamicVoxelCopyToBufferTechnique::slotResetLitVoxelComplete>(this);
}

/////////////////////////////////////////////////////////////////////////////////////////////

VkCommandBuffer* DynamicVoxelCopyToBufferTechnique::record(int currentImage, uint& commandBufferID, CommandBufferType& commandBufferType)
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
	
	uint dynamicAllignment = materialM->getMaterialUBDynamicAllignment();

	uint32_t offsetData;
	vkCmdBindPipeline(*commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, m_material->getPipeline()->getPipeline());
	offsetData = static_cast<uint32_t>(m_material->getMaterialUniformBufferIndex() * dynamicAllignment);
	vkCmdBindDescriptorSets(*commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, m_material->getPipelineLayout(), 0, 1, &m_material->refDescriptorSet(), 1, &offsetData);
	vkCmdDispatch(*commandBuffer, m_localWorkGroupsXDimension, m_localWorkGroupsYDimension, 1); // Compute shader global workgroup https://www.khronos.org/registry/vulkan/specs/1.1-extensions/html/vkspec.html

#ifdef USE_TIMESTAMP
	vkCmdWriteTimestamp(*commandBuffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, coreM->getComputeQueueQueryPool(), m_queryIndex1);
#endif

	// TODO: Review if those barriers are needed, in the VoxelRasterInScenarioTechnique technique
	//       some cases show what could be a not properly updated dynamicVoxelBuffer buffer content (trail of voxels)

	// TODO: An extra pipeline stage flag for compute might be needed, current one for vertex is just for voxel visualization
	VulkanStructInitializer::insertBufferMemoryBarrier(bufferM->getElement(move(string("dynamicVoxelBuffer"))),
		VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT,
		VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT,
		VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
		VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
		commandBuffer);

	coreM->endCommandBuffer(*commandBuffer);
	
	m_vectorCommand.push_back(commandBuffer);

	return commandBuffer;
}

/////////////////////////////////////////////////////////////////////////////////////////////

void DynamicVoxelCopyToBufferTechnique::postCommandSubmit()
{
	m_executeCommand = false;
	m_active         = false;
	m_needsToRecord  = false;

	m_dynamicVoxelCounterBuffer->getContent((void*)(&m_dynamicVoxelCounter));

	uint tempUint = 0;
	m_dynamicVoxelCounterBuffer->setContentHostVisible(&tempUint);

	m_signalDynamicVoxelCopyToBufferCompletion.emit();
}

/////////////////////////////////////////////////////////////////////////////////////////////

void DynamicVoxelCopyToBufferTechnique::slotResetLitVoxelComplete()
{
	m_active        = true;
	m_needsToRecord = true;
}

/////////////////////////////////////////////////////////////////////////////////////////////
