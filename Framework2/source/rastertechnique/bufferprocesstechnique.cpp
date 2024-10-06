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
#include "../../include/rastertechnique/bufferprocesstechnique.h"
#include "../../include/material/materialmanager.h"
#include "../../include/material/material.h"
#include "../../include/core/coremanager.h"
#include "../../include/uniformbuffer/uniformbuffer.h"

// NAMESPACE

// DEFINES

// STATIC MEMBER INITIALIZATION

/////////////////////////////////////////////////////////////////////////////////////////////

BufferProcessTechnique::BufferProcessTechnique(string &&name, string&& className) : RasterTechnique(move(name), move(className))
	, m_buffer(nullptr)
	, m_bufferNumElement(0)
	, m_numElementPerLocalWorkgroupThread(0)
	, m_numThreadPerLocalWorkgroup(0)
	, m_localWorkGroupsXDimension(0)
	, m_localWorkGroupsYDimension(0)
	, m_numThreadExecuted(0)
	, m_material(nullptr)
	, m_newPassRequested(false)
	, m_localSizeX(1)
	, m_localSizeY(1)
{
	m_recordPolicy  = CommandRecordPolicy::CRP_SINGLE_TIME;
	m_active        = false;
	m_needsToRecord = false;
}

/////////////////////////////////////////////////////////////////////////////////////////////

BufferProcessTechnique::~BufferProcessTechnique()
{

}

/////////////////////////////////////////////////////////////////////////////////////////////

void BufferProcessTechnique::init()
{
	
}

/////////////////////////////////////////////////////////////////////////////////////////////

void BufferProcessTechnique::recordBarriers(VkCommandBuffer* commandBuffer)
{

}

/////////////////////////////////////////////////////////////////////////////////////////////

VkCommandBuffer* BufferProcessTechnique::record(int currentImage, uint& commandBufferID, CommandBufferType& commandBufferType)
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
	vkCmdBindPipeline(*commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, m_material->getPipeline()->getPipeline());
	offsetData = static_cast<uint32_t>(m_material->getMaterialUniformBufferIndex() * dynamicAllignment);
	vkCmdBindDescriptorSets(*commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, m_material->getPipelineLayout(), 0, 1, &m_material->refDescriptorSet(), 1, &offsetData);
	vkCmdDispatch(*commandBuffer, m_localWorkGroupsXDimension, m_localWorkGroupsYDimension, 1); // Compute shader global workgroup https://www.khronos.org/registry/vulkan/specs/1.1-extensions/html/vkspec.html

#ifdef USE_TIMESTAMP
	vkCmdWriteTimestamp(*commandBuffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, coreM->getComputeQueueQueryPool(), m_queryIndex1);
#endif

	recordBarriersEnd(commandBuffer);

	coreM->endCommandBuffer(*commandBuffer);
	
	m_vectorCommand.push_back(commandBuffer);

	return commandBuffer;
}

/////////////////////////////////////////////////////////////////////////////////////////////

void BufferProcessTechnique::recordBarriersEnd(VkCommandBuffer* commandBuffer)
{

}

/////////////////////////////////////////////////////////////////////////////////////////////

void BufferProcessTechnique::buildShaderThreadMapping()
{
	uvec3 minComputeWorkGroupSize = coreM->getMinComputeWorkGroupSize();
	float tempValue               = float(m_numThreadPerLocalWorkgroup) / float(minComputeWorkGroupSize.y);
	if (tempValue >= 1.0f)
	{
		m_localSizeX = int(ceil(tempValue));
		m_localSizeY = minComputeWorkGroupSize.y;
	}
	else
	{
		m_localSizeX = m_numThreadPerLocalWorkgroup;
		m_localSizeY = 1;
	}

	m_computeShaderThreadMapping += "\n\n";
	m_computeShaderThreadMapping += "layout(local_size_x = " + to_string(m_localSizeX) + ", local_size_y = " + to_string(m_localSizeY) + ", local_size_z = 1) in;\n\n";
	m_computeShaderThreadMapping += "void main()\n";
	m_computeShaderThreadMapping += "{\n";
	m_computeShaderThreadMapping += "\tconst uint ELEMENT_PER_THREAD         = " + to_string(m_numElementPerLocalWorkgroupThread) + ";\n";
	m_computeShaderThreadMapping += "\tconst uint THREAD_PER_LOCAL_WORKGROUP = " + to_string(m_numThreadPerLocalWorkgroup) + ";\n";
	m_computeShaderThreadMapping += "\tconst uint LOCAL_SIZE_X_VALUE         = " + to_string(m_localSizeX) + ";\n";
	m_computeShaderThreadMapping += "\tconst uint LOCAL_SIZE_Y_VALUE         = " + to_string(m_localSizeY) + ";\n";
}

/////////////////////////////////////////////////////////////////////////////////////////////

void BufferProcessTechnique::obtainDispatchWorkGroupCount()
{
	// TODO: Unify with static class method obtainDispatchWorkGroupCount

	uvec3 maxComputeWorkGroupCount     = coreM->getMaxComputeWorkGroupCount();
	uint maxLocalWorkGroupXDimension   = maxComputeWorkGroupCount.x;
	uint maxLocalWorkGroupYDimension   = maxComputeWorkGroupCount.y;
	uint numElementPerLocalWorkgroup   = m_numThreadPerLocalWorkgroup * m_numElementPerLocalWorkgroupThread;
	m_numThreadExecuted                = uint(ceil(float(m_bufferNumElement) / float(m_numElementPerLocalWorkgroupThread)));

	float numLocalWorkgroupsToDispatch = ceil(float(m_bufferNumElement) / float(numElementPerLocalWorkgroup));
	if (numLocalWorkgroupsToDispatch <= maxLocalWorkGroupYDimension)
	{
		m_localWorkGroupsXDimension = 1;
		m_localWorkGroupsYDimension = uint(numLocalWorkgroupsToDispatch);
	}
	else
	{
		float integerPart;
		float fractional            = glm::modf(float(numLocalWorkgroupsToDispatch) / float(maxLocalWorkGroupYDimension), integerPart);
		m_localWorkGroupsXDimension = uint(ceil(integerPart + 1.0f));
		m_localWorkGroupsYDimension = uint(ceil(float(numLocalWorkgroupsToDispatch) / float(m_localWorkGroupsXDimension)));
	}
}

/////////////////////////////////////////////////////////////////////////////////////////////

void BufferProcessTechnique::obtainDispatchWorkGroupCount(uint  numThreadPerLocalWorkgroup,
														  uint  numElementPerLocalWorkgroupThread,
														  uint  bufferNumElement,
														  uint& localWorkGroupsXDimension,
														  uint& localWorkGroupsYDimension)
{
	// TODO: Unify with class method obtainDispatchWorkGroupCount

	uvec3 maxComputeWorkGroupCount   = coreM->getMaxComputeWorkGroupCount();
	uint maxLocalWorkGroupXDimension = maxComputeWorkGroupCount.x;
	uint maxLocalWorkGroupYDimension = maxComputeWorkGroupCount.y;
	uint numElementPerLocalWorkgroup = numThreadPerLocalWorkgroup * numElementPerLocalWorkgroupThread;

	float numLocalWorkgroupsToDispatch = ceil(float(bufferNumElement) / float(numElementPerLocalWorkgroup));
	if (numLocalWorkgroupsToDispatch <= maxLocalWorkGroupYDimension)
	{
		localWorkGroupsXDimension = 1;
		localWorkGroupsYDimension = uint(numLocalWorkgroupsToDispatch);
	}
	else
	{
		float integerPart;
		float fractional            = glm::modf(float(numLocalWorkgroupsToDispatch) / float(maxLocalWorkGroupYDimension), integerPart);
		localWorkGroupsXDimension = uint(ceil(integerPart + 1.0f));
		localWorkGroupsYDimension = uint(ceil(float(numLocalWorkgroupsToDispatch) / float(localWorkGroupsXDimension)));
	}
}

/////////////////////////////////////////////////////////////////////////////////////////////
