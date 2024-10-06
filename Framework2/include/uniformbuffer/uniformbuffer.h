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

#ifndef _UNIFORMBUFFER_H_
#define _UNIFORMBUFFER_H_

// GLOBAL INCLUDES

// PROJECT INCLUDES
#include "../headers.h"
#include "../../include/util/genericresource.h"
#include "../../include/uniformbuffer/cpubuffer.h"

// CLASS FORWARDING
class Buffer;

// NAMESPACE

// DEFINES

/////////////////////////////////////////////////////////////////////////////////////////////

class UniformBuffer : public GenericResource
{
	friend class UniformBufferManager;

protected:
	/** Parameter constructor
	* @param [in] shader's name
	* @return nothing */
	UniformBuffer(string &&name);

	/** Destructor
	* @return nothing */
	virtual ~UniformBuffer();

	/** Builds the GPU uniform buffer, the CPU buffer must exist first, since the information from m_UBHostMemory needs to be passed as
	* parameter
	* @return nothing */
	void buildGPUBufer();

	/** Destroy the resources allocated by the buffer and initializes the member variables
	* @return nothing */
	void destroyResources();

public:
	/** Uploads to the GPU the buffer information present in the CPU copy, given by m_UBHostMemory
	* @return nothing */
	void uploadCPUBufferToGPU();

	/** Uploads to the GPU the buffer information present in the CPU copy, given by m_UBHostMemory
	* @param vectorMappedRange [in] vector with the parts of the buffer to update
	* @return nothing */
	void uploadCPUBufferToGPU(const vector<VkMappedMemoryRange>& vectorMappedRange);

	GETCOPY(size_t, m_dynamicAllignment, DynamicAllignment)
	REF(VkDescriptorBufferInfo, m_bufferInfo, BufferInfo)
	GETCOPY_SET(int, m_minCellSize, MinCellSize)
	REF_PTR(Buffer, m_bufferInstance, BufferInstance)
	REF(CPUBuffer, m_CPUBuffer, CPUBuffer)
	GETCOPY_SET(VkDeviceMemory, m_deviceMemory, DeviceMemory)
	GETCOPY(bool, m_usePersistentMapping, UsePersistentMapping)

protected:
	VkBuffer                    m_vulkanBuffer;         //!< Vulkan buffer resource object handler
	VkDeviceMemory              m_deviceMemory;         //!< Buffer resource object's allocated device memory handler
	VkDescriptorBufferInfo      m_bufferInfo;           //!< Buffer info that need to supplied into write descriptor set (VkWriteDescriptorSet)
	vector<VkMappedMemoryRange> m_mappedRange;          //!< Metadata of memory mapped objects, currently the whole buffer is mapped from this uniform buffer wrapper
	uint8_t*                    m_data;                 //!< Host pointer containing the mapped device address which is used to write data into.
	size_t                      m_dynamicAllignment;    //!< Real cell size used to place elements
	int                         m_minCellSize;          //!< Minimum cell size requested when building the buffer
	Buffer*                     m_bufferInstance;       //!< Pointer to the buffer instance in the Vulkan buffer manager
	CPUBuffer                   m_CPUBuffer;            //!< CPU bufer mapping the information of the GPU buffer
	bool                        m_usePersistentMapping; //!< Flag to know if the buffer m_bufferInstance was build using persistent mapping (i.e., keeping a pointer to the mapped memory in m_mappedPointer and not unmapping it)
};

/////////////////////////////////////////////////////////////////////////////////////////////

#endif _UNIFORMBUFFER_H_
