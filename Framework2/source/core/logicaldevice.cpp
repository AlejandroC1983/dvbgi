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
#include "../../include/core/logicaldevice.h"
#include "../../include/core/coremanager.h"

// NAMESPACE

// DEFINES

// STATIC MEMBER INITIALIZATION

/////////////////////////////////////////////////////////////////////////////////////////////

LogicalDevice::LogicalDevice():
	  m_logicalDeviceGraphicsQueue(VK_NULL_HANDLE)
	, m_logicalDeviceComputeQueue(VK_NULL_HANDLE)
	, m_logicalDevice(VK_NULL_HANDLE)
{

}

/////////////////////////////////////////////////////////////////////////////////////////////

void LogicalDevice::createDevice(vector<const char*>& extensions, uint32_t graphicQueueIndex, uint32_t computeQueueIndex, VkPhysicalDevice physicalDevice, void* lastRequestedExtensionFeature)
{
	// Create Device with available queue information.
	vector<VkDeviceQueueCreateInfo> vectorQueueInfo;

	VkResult result;
	float queuePriorities[1] = { 0.0 };
	VkDeviceQueueCreateInfo queueInfo = {};
	queueInfo.queueFamilyIndex = graphicQueueIndex;
	queueInfo.sType            = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
	queueInfo.pNext            = NULL;
	queueInfo.queueCount       = 1;
	queueInfo.pQueuePriorities = queuePriorities;
	vectorQueueInfo.push_back(queueInfo);

	queueInfo.queueFamilyIndex = computeQueueIndex;
	vectorQueueInfo.push_back(queueInfo);

	VkPhysicalDeviceFeatures setEnabledFeatures = { VK_FALSE };

	// TODO: automatize this?
	// TODO: query for effective availability of this features after making the logical device
	setEnabledFeatures.geometryShader                         = VK_TRUE;
	setEnabledFeatures.fragmentStoresAndAtomics               = VK_TRUE;
	setEnabledFeatures.shaderTessellationAndGeometryPointSize = VK_TRUE;
	setEnabledFeatures.shaderImageGatherExtended              = VK_TRUE;
	setEnabledFeatures.shaderStorageImageExtendedFormats      = VK_TRUE;
	setEnabledFeatures.shaderStorageImageMultisample          = VK_TRUE;
	setEnabledFeatures.shaderStorageImageReadWithoutFormat    = VK_TRUE;
	setEnabledFeatures.shaderStorageImageWriteWithoutFormat   = VK_TRUE;
	setEnabledFeatures.shaderFloat64                          = VK_TRUE;
	setEnabledFeatures.shaderInt64                            = VK_TRUE;

	VkDeviceCreateInfo deviceInfo = {};
	deviceInfo.sType                   = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	deviceInfo.pNext                   = lastRequestedExtensionFeature;
	deviceInfo.queueCreateInfoCount    = uint32_t(vectorQueueInfo.size());
	deviceInfo.pQueueCreateInfos       = vectorQueueInfo.data();
	deviceInfo.enabledLayerCount       = 0;
	deviceInfo.ppEnabledLayerNames     = NULL;											// Device layers are deprecated
	deviceInfo.enabledExtensionCount   = (uint32_t)extensions.size();
	deviceInfo.ppEnabledExtensionNames = extensions.size() ? extensions.data() : NULL;
	deviceInfo.pEnabledFeatures        = &setEnabledFeatures;

	result = vkCreateDevice(physicalDevice, &deviceInfo, NULL, &m_logicalDevice);
	assert(result == VK_SUCCESS);
}

/////////////////////////////////////////////////////////////////////////////////////////////

void LogicalDevice::destroyDevice()
{
	vkDestroyDevice(m_logicalDevice, NULL);
	m_logicalDevice = VK_NULL_HANDLE;
}

/////////////////////////////////////////////////////////////////////////////////////////////

void LogicalDevice::requestGraphicsQueue(uint32_t graphicsQueueIndex)
{
	vkGetDeviceQueue(m_logicalDevice, graphicsQueueIndex, 0, &m_logicalDeviceGraphicsQueue);
}

/////////////////////////////////////////////////////////////////////////////////////////////

void LogicalDevice::requestComputeQueue(uint32_t computeQueueIndex)
{
	vkGetDeviceQueue(m_logicalDevice, computeQueueIndex, 0, &m_logicalDeviceComputeQueue);
}

/////////////////////////////////////////////////////////////////////////////////////////////
