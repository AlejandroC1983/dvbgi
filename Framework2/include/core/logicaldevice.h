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

#ifndef _LOGICALDEVICE_H_
#define _LOGICALDEVICE_H_

// GLOBAL INCLUDES

// PROJECT INCLUDES
#include "../../include/headers.h"
#include "../../include/util/getsetmacros.h"

// CLASS FORWARDING

// NAMESPACE

// DEFINES

/////////////////////////////////////////////////////////////////////////////////////////////

class LogicalDevice
{
public:
	/** Default constructor
	* @return nothing */
	LogicalDevice();

	/** Builds a new logical device, result is stored in m_device
	* @param extensions                    [in] extensions to add to the logical device
	* @param graphicQueueIndex             [in] graphics queue index
	* @param computeQueueIndex             [in] graphics queue index
	* @param physicalDevice                [in] physical device
	* @param lastRequestedExtensionFeature [in] pointer to the last requested extension feature (a linked list of structs with the extensions has been built, linked through the pNext field
	* @return nothing */
	void createDevice(vector<const char*>& extensions, uint32_t graphicQueueIndex, uint32_t computeQueueIndex, VkPhysicalDevice physicalDevice, void* lastRequestedExtensionFeature);

	/** Destroys the logical device used
	* @return nothing */
	void destroyDevice();

	/** Assigns to m_logicalDeviceGraphicsQueue the value of the graphics queue with presentation capabilities
	* @return nothing */
	void requestGraphicsQueue(uint32_t graphicsQueueIndex);

	/** Assigns to m_logicalDeviceComputeQueue the value of the graphics queue with presentation capabilities
	* @return nothing */
	void requestComputeQueue(uint32_t computeQueueIndex);

	GET(VkQueue, m_logicalDeviceGraphicsQueue, LogicalDeviceGraphicsQueue)
	GET(VkQueue, m_logicalDeviceComputeQueue, LogicalDeviceComputeQueue)
	GET(VkDevice, m_logicalDevice, LogicalDevice)

protected:
	VkQueue	 m_logicalDeviceGraphicsQueue; //!< Logical device graphics queue
	VkQueue	 m_logicalDeviceComputeQueue;  //!< Logical device compute queue
	VkDevice m_logicalDevice;	           //!< Logical device
};

/////////////////////////////////////////////////////////////////////////////////////////////

#endif _LOGICALDEVICE_H_
