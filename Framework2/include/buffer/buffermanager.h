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

#ifndef _BUFFERMANAGER_H_
#define _BUFFERMANAGER_H_

// GLOBAL INCLUDES

// PROJECT INCLUDES
#include "../../include/util/singleton.h"
#include "../../include/util/managertemplate.h"
#include "../headers.h"

// CLASS FORWARDING
class Buffer;

// NAMESPACE

// DEFINES
#define bufferM s_pBufferManager->instance()

/////////////////////////////////////////////////////////////////////////////////////////////

class BufferManager: public ManagerTemplate<Buffer>, public Singleton<BufferManager>
{
	friend class CoreManager;

public:
	/** Constructor
	* @return nothing */
	BufferManager();

	/** Destructor
	* @return nothing */
	virtual ~BufferManager();

	/** Builds a new buffer, a pointer to the buffer is returned, nullptr is returned if any errors while building it
	* @param instanceName     [in] name of the new instance (the m_sName member variable)
	* @param dataPointer      [in] void pointer to an array with the buffer data
	* @param dataSize         [in] size of the buffer data at *dataPointer
	* @param usage            [in] buffer usage
	* @param requirementsMask [in] buffer requirements
	* @return a pointer to the built texture, nullptr otherwise */
	Buffer* buildBuffer(string&& instanceName, void* dataPointer, VkDeviceSize dataSize, VkBufferUsageFlags usage, VkFlags requirementsMask);

	/** Builds a new device-local memory buffer using the information present in the staging buffer given as parameter, of the same size as the staging buffer
	* @param instanceName        [in] name of the new instance (the m_sName member variable)
	* @param stagingBufferAsData [in] pointer to the staging buffer with the information for the device-local buffer
	* @param usage               [in] buffer usage
	* @param requirementsMask    [in] buffer requirements
	* @return a pointer to the built texture, nullptr otherwise */
	Buffer* buildBuffer(string&& instanceName, Buffer* stagingBufferAsData, VkBufferUsageFlags usage, VkFlags requirementsMask);

	/** Builds a new buffer, a pointer to the buffer is returned, nullptr is returned if any errors while building it
	* @param instanceName        [in] name of the new instance (the m_sName member variable)
	* @param dataPointer         [in] void pointer to an array with the buffer data
	* @param dataSize            [in] size of the buffer data at *dataPointer
	* @param usage               [in] buffer usage
	* @param requirementsMask    [in] buffer requirements
	* @param stagingBufferAsData [in] if provided, the buffer will be used as staging buffer for the buffer to be built, which is supposed to be a device-local buffer with the contents and size from the staging one
	* @return a pointer to the built texture, nullptr otherwise */
	Buffer* buildBuffer(string&& instanceName, void* dataPointer, VkDeviceSize dataSize, VkBufferUsageFlags usage, VkFlags requirementsMask, Buffer* stagingBufferAsData);

	/** Destroys all elements in the manager
	* @return nothing */
	void destroyResources();

	/** Resizes the buffer with name guiven by instanceName, with the new size given as parameter
	* @param buffer      [in] buffer to resize
	* @param dataPointer [in] void pointer to an array with the buffer data
	* @param newSize     [in] new buffer size
	* @return true if the resizing was done properly, false otherwise */
	bool resize(Buffer* buffer, void* dataPointer, uint newSize);

	/** Prints information about each existing buffer resource (name and size)
	* @return nothing */
	void printBufferResourceInformation();

	/** Copy size bytes from the souce buffer into the destination buffer, with an offset in source of sourceOffset
	* bytes, and an ofset in the destination of destinationOffset bytes
	* @param source            [inout] command buffer to use to recird the copy commands. If nullptr is provided, an internal command buffer will be bult and submitted to graphics queue
	* @param source            [in]    source buffer to copy from
	* @param destination       [in]    destination buffer to copy to
	* @param sourceOffset      [in]    offset to apply to the source buffer to copy from
	* @param destinationOffset [in]    offset to apply to the destination buffer to copy to
	* @param size              [in]    number of bytes to copy
	* @return nothing */
	void copyBuffer(VkCommandBuffer* commandBufferParameter, Buffer* source, Buffer* destination, int sourceOffset, int destinationOffset, int size);

	/** Destroys the buffer given as parameter
	* @param buffer [in] buffer to destroy
	* @return true if the buffer was destroyed succesfully, false otherwise */
	bool destroyBuffer(Buffer* buffer);

protected:
	/** Builds a new VkBuffer, with size and usage given as parameter
	* @param size   [in] buffer size
	* @param usage  [in] buffer usage
	* @return a VkBuffer for the built buffer */
	VkBuffer buildBuffer(VkDeviceSize size, VkBufferUsageFlags usage);

	/** Builds memory for the buffer given as parameter, taking the memory requirements from the already built buffer
	* @param requirementsMask [in] mask with the memory requirements
	* @param memory           [in] memory allocated for the buffer
	* @param buffer           [in] buffer handle
	* @param bufferUsageFlags [in] buffer usage flags
	* @return the size of the mapped memory of the built buffer */
	VkDeviceSize buildBufferMemory(VkFlags requirementsMask, VkDeviceMemory& memory, VkBuffer& buffer, VkBufferUsageFlags bufferUsageFlags);

	/** Fills the memory of a buffer, given by the memory parameter, with the data present at dataPointer, with size mappingSize,
	* the parameter of the mapped buffer memory size, mappingSize, is needed for the vkMapMemory primitive
	* @param memory      [in] buffer memory to fill
	* @param mappingSize [in] buffer mapped memory size
	* @param dataPointer [in] pointer to the data to fill the memory with
	* @param dataSize    [in] pointer to the size of the data to fill the memory with
	* @return nothing */
	void fillBufferMemory(VkDeviceMemory& memory, VkDeviceSize mappingSize, const void* dataPointer, VkDeviceSize dataSize);

	/** Builds the buffer GPU resources
	* @param buffer              [in] buffer for which build the resources
	* @param stagingBufferAsData [in] if this parameter is not nullptr, then the buffer to be built will be done from the one provided and not from the buffer data
	* @return nothing */
	void buildBufferResource(Buffer* buffer, Buffer* stagingBufferAsData);
};

static BufferManager* s_pBufferManager;

/////////////////////////////////////////////////////////////////////////////////////////////

#endif _BUFFERMANAGER_H_
