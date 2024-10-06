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

#ifndef _BUFFER_H_
#define _BUFFER_H_

// GLOBAL INCLUDES

// PROJECT INCLUDES
#include "../headers.h"
#include "../../include/util/genericresource.h"

// CLASS FORWARDING

// NAMESPACE

// DEFINES

/////////////////////////////////////////////////////////////////////////////////////////////

class Buffer : public GenericResource
{
	friend class BufferManager;

protected:
	/** Parameter constructor
	* @param [in] buffer's name
	* @return nothing */
	Buffer(string &&name);

	/** Destroy the resources used by the buffer
	* @return nothing */
	void destroyResources();

public:
	/** Destructor
	* @return nothing */
	virtual ~Buffer();

	/** Copies to the address given by the data parameter the content of the buffer, if host visible
	* @return true if the copy operation was made successfully, false otherwise */
	bool getContent(void* data);

	/** Returns a vector of byte with the information of the buffer
	* @return true if the copy operation was made successfully, false otherwise */
	bool getContentCopy(vectorUint8& vectorData);

	/** Fills the memory of a buffer with the data present at dataPointer, for those buffers which are host visible
	* The range of mapped buffer memory is invalidated to make it visible to the host. If the memory property is set
	* with VK_MEMORY_PROPERTY_HOST_COHERENT_BIT then the driver may take care of this, otherwise for non-coherent mapped memory
	* vkInvalidateMappedMemoryRanges() needs to be called explicitly.
	* @param dataPointer [in] pointer to the data to fill the memory with (must match in size with the memory size of the buffer)
	* @return true if the set content was made successfully, false otherwise */
	bool setContentHostVisible(const void* dataPointer);

	/** Returns the device address of the buffer
	* @return buffer device address */
	VkDeviceAddress getBufferDeviceAddress();

	GET(VkDeviceSize, m_mappingSize, MappingSize)
	GET(VkBufferUsageFlags, m_usage, Usage)
	GET(VkFlags, m_requirementsMask, RequirementsMask)
	GET(VkDeviceMemory, m_memory, Memory)
	GET(VkBuffer, m_buffer, Buffer)
	REF_PTR(void, m_dataPointer, DataPointer)
	GET(VkDeviceSize, m_dataSize, DataSize)
	REF_PTR(uint8_t, m_mappedPointer, MappedPointer)
	GET(VkMappedMemoryRange, m_mappedRange, MappedRange)
	GET(VkDescriptorBufferInfo, m_descriptorBufferInfo, DescriptorBufferInfo)
	REF(VkDescriptorBufferInfo, m_descriptorBufferInfo, DescriptorBufferInfo)
	GETCOPY(bool, m_usePersistentMapping, UsePersistentMapping)

public:
	VkDeviceSize             m_mappingSize;            //!< Size of the memory of this uniform buffer
	VkBufferUsageFlags       m_usage;                  //!< Flag with the usage of the buffer, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT for instance for the scene transform and material uniform buffers
	VkFlags                  m_requirementsMask;       //!< Enum of type VkMemoryPropertyFlagBits, to determine if the buffer is host visible, device local, or any other possivle option
	VkDeviceMemory           m_memory;                 //!< Allocated buffer memory
	VkBuffer                 m_buffer;                 //!< Handle to the built buffer
	void*                    m_dataPointer;            //!< Alligned host memory of the buffer, to write to and the upload it to the GPU
	VkDeviceSize             m_dataSize;               //!< Size of the host memory buffer
	uint8_t*                 m_mappedPointer;          //!< Host memory pointer to write buffer information, this is used when persistent mapping is enabled
	VkMappedMemoryRange      m_mappedRange;            //!< Range of m_memory mapped
	VkDescriptorBufferInfo   m_descriptorBufferInfo;   //!< Struct to build a descriptor set for this buffer
	bool                     m_usePersistentMapping;   //!< Flag to know if the buffer was build using persistent mapping (i.e., keeping a pointer to the mapped memory in m_mappedPointer and not unmapping it)
};

/////////////////////////////////////////////////////////////////////////////////////////////

#endif _BUFFER_H_
