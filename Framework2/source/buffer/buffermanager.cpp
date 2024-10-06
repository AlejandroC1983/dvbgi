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
#include "../../include/buffer/buffermanager.h"
#include "../../include/buffer/buffer.h"
#include "../../include/core/coremanager.h"
#include "../../include/core/physicaldevice.h"
#include "../../include/parameter/attributedefines.h"
#include "../../include/util/vulkanstructinitializer.h"
#include "../../include/util/logutil.h"

// NAMESPACE
using namespace attributedefines;

// DEFINES

// STATIC MEMBER INITIALIZATION

/////////////////////////////////////////////////////////////////////////////////////////////

BufferManager::BufferManager()
{
	m_managerName = g_bufferManager;
}

/////////////////////////////////////////////////////////////////////////////////////////////

BufferManager::~BufferManager()
{
	destroyResources();
}

/////////////////////////////////////////////////////////////////////////////////////////////

void BufferManager::destroyResources()
{
	forIT(m_mapElement)
	{
		delete it->second;
		it->second = nullptr;
	}
}

/////////////////////////////////////////////////////////////////////////////////////////////

Buffer* BufferManager::buildBuffer(string&& instanceName, void* dataPointer, VkDeviceSize dataSize, VkBufferUsageFlags usage, VkFlags requirementsMask)
{
	return buildBuffer(move(instanceName), dataPointer, dataSize, usage, requirementsMask, nullptr);
}

/////////////////////////////////////////////////////////////////////////////////////////////

Buffer* BufferManager::buildBuffer(string&& instanceName, Buffer* stagingBufferAsData, VkBufferUsageFlags usage, VkFlags requirementsMask)
{
	return buildBuffer(move(instanceName), nullptr, 0, usage, requirementsMask, stagingBufferAsData);
}

/////////////////////////////////////////////////////////////////////////////////////////////

Buffer* BufferManager::buildBuffer(string&& instanceName, void* dataPointer, VkDeviceSize dataSize, VkBufferUsageFlags usage, VkFlags requirementsMask, Buffer* stagingBufferAsData)
{
	if (existsElement(move(string(instanceName))))
	{
		return getElement(move(instanceName));
	}

	Buffer* buffer = new Buffer(move(string(instanceName)));

	buffer->m_dataSize                    = (stagingBufferAsData != nullptr) ? stagingBufferAsData->getDataSize() : dataSize;
	buffer->m_dataPointer                 = dataPointer;
	buffer->m_usage                       = usage;
	buffer->m_requirementsMask            = requirementsMask;
	buffer->m_usePersistentMapping        = ((requirementsMask & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) > 0);
	buffer->m_descriptorBufferInfo.offset = 0;
	buffer->m_descriptorBufferInfo.range  = buffer->m_dataSize;

	if (((requirementsMask & (VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT)) > 0) && (stagingBufferAsData != nullptr))
	{
		cout << "ERROR: Call to BufferManager::buildBuffer with flag usePersistentMapping being true and staging buffer being not nullptr" << endl;
		assert(false);
	}

	buildBufferResource(buffer, stagingBufferAsData);

	buffer->m_descriptorBufferInfo.buffer = buffer->m_buffer;

	addElement(move(string(instanceName)), buffer);
	buffer->m_name = move(instanceName);
	buffer->m_ready = true;

	coreM->setObjectName(uint64_t(buffer->m_buffer), VK_DEBUG_REPORT_OBJECT_TYPE_BUFFER_EXT, buffer->getName());

	return buffer;
}

/////////////////////////////////////////////////////////////////////////////////////////////

bool BufferManager::resize(Buffer* buffer, void* dataPointer, uint newSize)
{
	VkBufferUsageFlags usage = buffer->m_usage;
	VkFlags requirementsMask = buffer->m_requirementsMask;

	buffer->m_ready = false;
	buffer->destroyResources();

	buffer->m_dataSize         = newSize;
	buffer->m_dataPointer      = dataPointer;
	buffer->m_usage            = usage;
	buffer->m_requirementsMask = requirementsMask;

	buildBufferResource(buffer, nullptr);

	buffer->m_descriptorBufferInfo.buffer = buffer->m_buffer;
	buffer->m_descriptorBufferInfo.offset = 0;
	buffer->m_descriptorBufferInfo.range  = buffer->m_dataSize;

	coreM->setObjectName(uint64_t(buffer->m_buffer), VK_DEBUG_REPORT_OBJECT_TYPE_BUFFER_EXT, buffer->getName());

	buffer->m_ready = true;

	if (gpuPipelineM->getPipelineInitialized())
	{
		emitSignalElement(move(string(buffer->m_name)), ManagerNotificationType::MNT_CHANGED);
	}

	return true;
}

/////////////////////////////////////////////////////////////////////////////////////////////

void BufferManager::printBufferResourceInformation()
{
	vectorString bufferName = {
		"cameraVisibleDynamicVoxelBuffer",
		"cameraVisibleDynamicVoxelIrradianceBuffer",
		"cameraVisibleDynamicVoxelPerByteBuffer",
		"cameraVisibleDynamicVoxelPerByteTagProcessBuffer",
		"cameraVisibleVoxelCompactedBuffer",
		"cameraVisibleVoxelPerByteBuffer",
		"dynamicVisibleVoxelCounterBuffer",
		"dynamicVoxelBuffer",
		"dynamicVoxelCounterBuffer",
		"dynamicVoxelVisibilityFlagBuffer",
		"dynamicVoxelVisibleCounterBuffer",
		"irradianceFilteringTagTilesBuffer",
		"irradianceFilteringTagTilesCounterBuffer",
		"irradianceFilteringTagTilesIndexBuffer",
		"irradiancePaddingTagTilesBuffer",
		"irradiancePaddingTagTilesCounterBuffer",
		"irradiancePaddingTagTilesIndexBuffer",
		"lightBounceProcessedVoxelBuffer",
		"lightBounceVoxelIrradianceBuffer",
		"lightBounceVoxelIrradianceTempBuffer",
		"litTestDynamicVoxelPerByteBuffer",
		"litTestVoxelPerByteBuffer",
		"staticVoxelVisibleCounterBuffer",
		"voxelHashedPositionCompactedBuffer",
		"voxelOccupiedBuffer",
		"voxelOccupiedDynamicBuffer",
		"voxelVisibility4BytesBuffer",
		"voxelVisibilityDynamic4ByteBuffer",
		"dynamicVoxelVisibilityBuffer"
	};

	mapStringInt mapData;
	forIT(bufferName)
	{
		auto itResult = m_mapElement.find(*it);
		mapData.insert(pair<string, int>(itResult->first, itResult->second->getDataSize()));
	}

	LogUtil::printInformationTabulated(mapData);
}

/////////////////////////////////////////////////////////////////////////////////////////////

void BufferManager::copyBuffer(VkCommandBuffer *commandBufferParameter, Buffer* source, Buffer* destination, int sourceOffset, int destinationOffset, int size)
{
	VkCommandBuffer commandBuffer;
	if (commandBufferParameter == nullptr)
	{
		coreM->allocCommandBuffer(&coreM->getLogicalDevice(), coreM->getGraphicsCommandPool(), &commandBuffer);
		coreM->beginCommandBuffer(commandBuffer);
	}
	else
	{
		commandBuffer = *commandBufferParameter;
	}

	VkBufferCopy bufferCopyRegion = { sourceOffset, destinationOffset, VkDeviceSize(size) };
	vkCmdCopyBuffer(commandBuffer, source->getBuffer(), destination->getBuffer(), 1, &bufferCopyRegion);

	VulkanStructInitializer::insertBufferMemoryBarrier(destination,
													   VK_ACCESS_TRANSFER_READ_BIT | VK_ACCESS_TRANSFER_WRITE_BIT,
													   VK_ACCESS_TRANSFER_READ_BIT | VK_ACCESS_TRANSFER_WRITE_BIT, //VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT,
													   VK_PIPELINE_STAGE_TRANSFER_BIT,
													   VK_PIPELINE_STAGE_TRANSFER_BIT, //VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
													   &commandBuffer);

	if (commandBufferParameter == nullptr)
	{
		coreM->endCommandBuffer(commandBuffer);
		coreM->submitCommandBuffer(coreM->getLogicalDeviceGraphicsQueue(), &commandBuffer);
		vkFreeCommandBuffers(coreM->getLogicalDevice(), coreM->getGraphicsCommandPool(), 1, &commandBuffer);
	}
}

/////////////////////////////////////////////////////////////////////////////////////////////

bool BufferManager::destroyBuffer(Buffer* buffer)
{
	return removeElement(move(string(buffer->getName())));
}

/////////////////////////////////////////////////////////////////////////////////////////////

VkBuffer BufferManager::buildBuffer(VkDeviceSize size, VkBufferUsageFlags usage)
{
	// Create a staging buffer resource states using.
	// Indicate it be the source of the transfer command.
	VkBufferCreateInfo bufferCreateInfo = {};
	bufferCreateInfo.sType                 = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferCreateInfo.pNext                 = NULL;
	bufferCreateInfo.size                  = size;
	bufferCreateInfo.usage                 = usage;
	bufferCreateInfo.sharingMode           = VK_SHARING_MODE_EXCLUSIVE;
	bufferCreateInfo.queueFamilyIndexCount = 0;
	bufferCreateInfo.pQueueFamilyIndices   = NULL;
	bufferCreateInfo.flags                 = 0;

	// Create a buffer resource (host-visible) -
	VkBuffer buffer;
	VkResult error = vkCreateBuffer(coreM->getLogicalDevice(), &bufferCreateInfo, NULL, &buffer);
	assert(!error);

	return buffer;
}

/////////////////////////////////////////////////////////////////////////////////////////////

VkDeviceSize BufferManager::buildBufferMemory(VkFlags requirementsMask, VkDeviceMemory& memory, VkBuffer& buffer, VkBufferUsageFlags bufferUsageFlags)
{
	VkMemoryRequirements memRqrmnt;
	vkGetBufferMemoryRequirements(coreM->getLogicalDevice(), buffer, &memRqrmnt);

	if (memRqrmnt.size == 0)
	{
		return 0;
	}

	VkMemoryAllocateInfo memAllocInfo = {};
	memAllocInfo.sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	memAllocInfo.pNext           = NULL;
	memAllocInfo.memoryTypeIndex = 0;
	memAllocInfo.allocationSize  = memRqrmnt.size; // is equal to sizeof(MVP)???

	VkMemoryAllocateFlagsInfo memoryAllocateFlagsInfo = {};
	if (bufferUsageFlags & VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT)
	{
		memoryAllocateFlagsInfo.sType      = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_FLAGS_INFO;
		memoryAllocateFlagsInfo.flags      = VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT;
		memoryAllocateFlagsInfo.deviceMask = 0;
		memAllocInfo.pNext                 = &memoryAllocateFlagsInfo;
	}

	bool memoryTypeResult = coreM->memoryTypeFromProperties(memRqrmnt.memoryTypeBits, requirementsMask, coreM->getPhysicalDeviceMemoryProperties().memoryTypes, memAllocInfo.memoryTypeIndex);
	assert(memoryTypeResult);

	VkResult result = vkAllocateMemory(coreM->getLogicalDevice(), &memAllocInfo, NULL, &(memory));
	assert(result == VK_SUCCESS);

	result = vkBindBufferMemory(coreM->getLogicalDevice(), buffer, memory, 0);
	assert(result == VK_SUCCESS);

	return memRqrmnt.size;
}

/////////////////////////////////////////////////////////////////////////////////////////////

void BufferManager::fillBufferMemory(VkDeviceMemory& memory, VkDeviceSize mappingSize, const void* dataPointer, VkDeviceSize dataSize)
{
	uint8_t* mappedPointer;
	VkResult result = vkMapMemory(coreM->getLogicalDevice(), memory, 0, mappingSize, 0, (void **)&mappedPointer);
	assert(result == VK_SUCCESS);

	memcpy(mappedPointer, dataPointer, dataSize);

	vkUnmapMemory(coreM->getLogicalDevice(), memory);
}

/////////////////////////////////////////////////////////////////////////////////////////////

void BufferManager::buildBufferResource(Buffer* buffer, Buffer* stagingBufferAsData)
{
	buffer->m_buffer      = buildBuffer(buffer->m_dataSize, buffer->m_usage);
	buffer->m_mappingSize = buildBufferMemory(buffer->m_requirementsMask, buffer->m_memory, buffer->m_buffer, buffer->m_usage);

	VkMappedMemoryRange mappedRange;
	mappedRange.sType     = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
	mappedRange.memory    = buffer->m_memory;
	mappedRange.offset    = 0;
	mappedRange.size      = VK_WHOLE_SIZE;
	mappedRange.pNext     = nullptr;
	buffer->m_mappedRange = mappedRange;

	if ((buffer->m_usePersistentMapping == true) && (stagingBufferAsData != nullptr))
	{
		cout << "ERROR: Call to BufferManager::buildBufferResource with flag usePersistentMapping being true and staging buffer being not nullptr" << endl;
		assert(false);
	}

	if ((buffer->m_dataPointer != nullptr) && (stagingBufferAsData != nullptr))
	{
		cout << "ERROR: Call to BufferManager::buildBufferResource with flag m_dataPointer not nullptr and staging buffer being not nullptr" << endl;
		assert(false);
	}

	if (buffer->m_usePersistentMapping)
	{
		// Persistent mapping https://zeux.io/2020/02/27/writing-an-efficient-vulkan-renderer/
		vkMapMemory(coreM->getLogicalDevice(), buffer->getMemory(), 0, buffer->getMappingSize(), 0, (void**)&buffer->m_mappedPointer);
	}

	if (buffer->m_dataPointer != nullptr)
	{
		if (buffer->m_requirementsMask & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT)
		{
			// If the buffer has data and is host visible, then update the data directly
			buffer->setContentHostVisible(buffer->m_dataPointer);
		}
		else
		{
			// If the buffer is not host visible, then prepare a staging, host visible buffer and copy
			// the data to the staging buffer, then copy to the final buffer
			Buffer* stagingBufferTemp = bufferM->getElement(move(string("tempBuffer")));

			// Make sure no buffer with name tempBuffer exists
			assert(stagingBufferTemp == nullptr);

			stagingBufferTemp = bufferM->buildBuffer(move(string("tempBuffer")),
												 buffer->m_dataPointer,
												 buffer->m_dataSize,
												 VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
												 VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);

			copyBuffer(nullptr, stagingBufferTemp, buffer, 0, 0, buffer->m_dataSize);

			destroyBuffer(stagingBufferTemp);
		}
	}
	else if (stagingBufferAsData != nullptr)
	{
		copyBuffer(nullptr, stagingBufferAsData, buffer, 0, 0, buffer->m_dataSize);
	}
}

/////////////////////////////////////////////////////////////////////////////////////////////
