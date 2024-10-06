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
#include "../../include/buffer/buffer.h"
#include "../../include/core/coremanager.h"
#include "../../include/texture/texture.h"

// NAMESPACE

// DEFINES

// STATIC MEMBER INITIALIZATION

/////////////////////////////////////////////////////////////////////////////////////////////

// TODO initialize properly
Buffer::Buffer(string &&name) : GenericResource(move(name), move(string("Buffer")), GenericResourceType::GRT_BUFFER)
	, m_mappingSize(0)
	, m_usage(VK_BUFFER_USAGE_FLAG_BITS_MAX_ENUM)
	, m_requirementsMask(0)
	, m_memory(VK_NULL_HANDLE)
	, m_buffer(VK_NULL_HANDLE)
	, m_dataPointer(nullptr)
	, m_dataSize(0)
	, m_mappedPointer(0)
	, m_mappedRange({ VK_STRUCTURE_TYPE_MAX_ENUM , nullptr, VK_NULL_HANDLE , 0, 0 })
	, m_descriptorBufferInfo({ VK_NULL_HANDLE , 0, 0 })
	, m_usePersistentMapping(false)
{

}

/////////////////////////////////////////////////////////////////////////////////////////////

void Buffer::destroyResources()
{
	vkDestroyBuffer(coreM->getLogicalDevice(), m_buffer, nullptr);
	vkFreeMemory(coreM->getLogicalDevice(), m_memory, nullptr);

	m_mappingSize          = 0;
	m_usage                = VK_BUFFER_USAGE_FLAG_BITS_MAX_ENUM;
	m_requirementsMask     = 0;
	m_memory               = VK_NULL_HANDLE;
	m_buffer               = VK_NULL_HANDLE;
	m_dataPointer          = nullptr;
	m_dataSize             = 0;
	m_mappedPointer        = 0;
	m_mappedRange          = { VK_STRUCTURE_TYPE_MAX_ENUM , nullptr, VK_NULL_HANDLE, 0, 0 };
	m_descriptorBufferInfo = { VK_NULL_HANDLE, 0, 0 };
}

/////////////////////////////////////////////////////////////////////////////////////////////

Buffer::~Buffer()
{
	destroyResources();
}

/////////////////////////////////////////////////////////////////////////////////////////////

bool Buffer::getContent(void* data)
{
	memcpy(data, m_mappedPointer, sizeof(uint));

	return true;
}

/////////////////////////////////////////////////////////////////////////////////////////////

bool Buffer::getContentCopy(vectorUint8& vectorData)
{
	if (m_usePersistentMapping)
	{
		cout << "ERROR BUFFER WITH PERSISTENT MAPPING CALLING Buffer::getContentCopy" << endl;
	}

	void* mappedMemory = nullptr;

	VkResult result = vkMapMemory(coreM->getLogicalDevice(), m_memory, 0, uint(m_dataSize), 0, &mappedMemory);
	assert(result == VK_SUCCESS);

	vectorData.resize(m_dataSize);
	memcpy((void*)vectorData.data(), mappedMemory, m_dataSize);

	vkUnmapMemory(coreM->getLogicalDevice(), m_memory);

	return (result == VK_SUCCESS);
}

/////////////////////////////////////////////////////////////////////////////////////////////

bool Buffer::setContentHostVisible(const void* dataPointer)
{
	bool resultToReturn = true;

	VkResult result;

	if (!m_usePersistentMapping)
	{
		result = vkMapMemory(coreM->getLogicalDevice(), m_memory, 0, m_mappingSize, 0, (void**)&m_mappedPointer);
		assert(result == VK_SUCCESS);
		resultToReturn &= (result == VK_SUCCESS);
	}

	memcpy(m_mappedPointer, dataPointer, m_dataSize);

	// Invalidate the range of mapped buffer in order to make it visible to the host. If the memory property is set with VK_MEMORY_PROPERTY_HOST_COHERENT_BIT then the driver may take care of this, otherwise for non-coherent  mapped memory vkInvalidateMappedMemoryRanges() needs to be called explicitly.
	result = vkInvalidateMappedMemoryRanges(coreM->getLogicalDevice(), 1, &m_mappedRange);
	assert(result == VK_SUCCESS);

	resultToReturn &= (result == VK_SUCCESS);

	if (!m_usePersistentMapping)
	{
		vkUnmapMemory(coreM->getLogicalDevice(), m_memory);
	}

	return resultToReturn;
}

/////////////////////////////////////////////////////////////////////////////////////////////

VkDeviceAddress Buffer::getBufferDeviceAddress()
{
	VkBufferDeviceAddressInfo bufferDeviceAddressInfo = VkBufferDeviceAddressInfo({ VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO, nullptr, m_buffer });
	return vkfpM->vkGetBufferDeviceAddress(coreM->getLogicalDevice(), &bufferDeviceAddressInfo);
}

/////////////////////////////////////////////////////////////////////////////////////////////