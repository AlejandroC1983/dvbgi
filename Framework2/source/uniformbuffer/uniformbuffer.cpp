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
#include "../../include/uniformbuffer/uniformbuffer.h"
#include "../../include/core/coremanager.h"
#include "../../include/buffer/buffermanager.h"
#include "../../include/buffer/buffer.h"
#include "../../include/util/mathutil.h"

// NAMESPACE

// DEFINES

// STATIC MEMBER INITIALIZATION

/////////////////////////////////////////////////////////////////////////////////////////////

UniformBuffer::UniformBuffer(string &&name) : GenericResource(move(name), move(string("UniformBuffer")), GenericResourceType::GRT_UNIFORMBUFFER)
	, m_vulkanBuffer(VK_NULL_HANDLE)
	, m_deviceMemory(VK_NULL_HANDLE)
	, m_bufferInfo({ VK_NULL_HANDLE, 0, 0 })
	, m_data(nullptr)
	, m_dynamicAllignment(0)
	, m_minCellSize(0)
	, m_bufferInstance(nullptr)
	, m_usePersistentMapping(false)
{
}

/////////////////////////////////////////////////////////////////////////////////////////////

UniformBuffer::~UniformBuffer()
{
	destroyResources();
}

/////////////////////////////////////////////////////////////////////////////////////////////

void UniformBuffer::buildGPUBufer()
{
	size_t minUniformBufferObjectOffsetAlignment = coreM->getPhysicalDeviceProperties().limits.minUniformBufferOffsetAlignment;
	m_dynamicAllignment                          = int32_t(ceil(float(m_minCellSize) / float(minUniformBufferObjectOffsetAlignment)) * float(minUniformBufferObjectOffsetAlignment));

	if (!MathUtil::isPowerOfTwo(uint(m_dynamicAllignment)))
	{
		m_dynamicAllignment = MathUtil::getNextPowerOfTwo(uint(m_dynamicAllignment));
	}

	// TODO: remove duplicated information, like m_vulkanBuffer
	m_bufferInstance    = bufferM->buildBuffer(move(string(m_name)), m_CPUBuffer.refUBHostMemory(), m_CPUBuffer.getUBHostMemorySize(), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, nullptr);
	m_vulkanBuffer      = m_bufferInstance->getBuffer();
	m_bufferInfo.buffer = m_bufferInstance->getBuffer();
	m_bufferInfo.offset = 0;
	m_bufferInfo.range  = m_dynamicAllignment; //m_bufferInstance->getDataSize();
	m_deviceMemory      = m_bufferInstance->getMemory();
	m_data              = m_bufferInstance->refMappedPointer();

	m_mappedRange.push_back(m_bufferInstance->getMappedRange());
}

/////////////////////////////////////////////////////////////////////////////////////////////

void UniformBuffer::uploadCPUBufferToGPU()
{
	if (!m_usePersistentMapping)
	{
		vkMapMemory(coreM->getLogicalDevice(), m_deviceMemory, 0, m_bufferInstance->getMappingSize(), 0, (void**)&m_data);
	}
	
	memcpy(m_bufferInstance->m_mappedPointer, m_CPUBuffer.refUBHostMemory(), m_CPUBuffer.getUBHostMemorySize());
	VkResult result = vkFlushMappedMemoryRanges(coreM->getLogicalDevice(), 1, &m_bufferInstance->getMappedRange());
	assert(result == VK_SUCCESS);

	if (!m_usePersistentMapping)
	{
		vkUnmapMemory(coreM->getLogicalDevice(), m_deviceMemory);
	}
}

/////////////////////////////////////////////////////////////////////////////////////////////

void UniformBuffer::uploadCPUBufferToGPU(const vector<VkMappedMemoryRange>& vectorMappedRange)
{
	uint8_t* pointerToCPUBuffer = (uint8_t*)(m_CPUBuffer.refUBHostMemory());

	if (!m_usePersistentMapping)
	{
		forI(vectorMappedRange.size())
		{
			const VkMappedMemoryRange& mappedMemoryRange = vectorMappedRange[i];
			VkResult result = vkMapMemory(coreM->getLogicalDevice(), m_deviceMemory, mappedMemoryRange.offset, mappedMemoryRange.size, 0, (void**)&m_data);
			assert(result == VK_SUCCESS);

			//VkResult res = vkInvalidateMappedMemoryRanges(coreM->getLogicalDevice(), 1, &mappedMemoryRange);
			memcpy(m_data, (void*)(pointerToCPUBuffer + mappedMemoryRange.offset), mappedMemoryRange.size);

			result = vkFlushMappedMemoryRanges(coreM->getLogicalDevice(), 1, &mappedMemoryRange);
			assert(result == VK_SUCCESS);

			vkUnmapMemory(coreM->getLogicalDevice(), m_deviceMemory);
		}
	}
	else
	{
		// Persistent mapping, see https://zeux.io/2020/02/27/writing-an-efficient-vulkan-renderer/ Section "Mapping memory"

		forI(vectorMappedRange.size())
		{
			const VkMappedMemoryRange& mappedMemoryRange = vectorMappedRange[i];
			memcpy(m_bufferInstance->m_mappedPointer + mappedMemoryRange.offset, (void*)(pointerToCPUBuffer + mappedMemoryRange.offset), mappedMemoryRange.size);

			VkResult result = vkFlushMappedMemoryRanges(coreM->getLogicalDevice(), 1, &mappedMemoryRange);
			assert(result == VK_SUCCESS);
		}
	}
}

/////////////////////////////////////////////////////////////////////////////////////////////

void UniformBuffer::destroyResources()
{
	m_vulkanBuffer      = VK_NULL_HANDLE;
	m_deviceMemory      = VK_NULL_HANDLE;
	m_bufferInfo        = {VK_NULL_HANDLE, 0, 0};
	m_data              = nullptr;
	m_dynamicAllignment = 0;
	m_bufferInstance    = nullptr;
	m_mappedRange.clear();
}

////////////////////////////////////////////////////////////////////////////////////////
