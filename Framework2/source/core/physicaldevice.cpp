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
#include "../../include/core/physicaldevice.h"
#include "../../include/core/coremanager.h"

// NAMESPACE

// DEFINES

// STATIC MEMBER INITIALIZATION

/////////////////////////////////////////////////////////////////////////////////////////////

PhysicalDevice::PhysicalDevice():
	  m_physicalDeviceFeatures({})
	, m_graphicsQueueIndex(UINT32_MAX)
	, m_computeQueueIndex(UINT32_MAX)
	, m_physicalDevice(VK_NULL_HANDLE)
	, m_physicalDeviceProperties({})
	, m_physicalDeviceMemoryProperties({})
{

}

/////////////////////////////////////////////////////////////////////////////////////////////

vector<VkPhysicalDevice> PhysicalDevice::enumeratePhysicalDevices(VkInstance instance)
{
	uint32_t gpuDeviceCount;
	vector<VkPhysicalDevice> arrayGPU;

	VkResult result = vkEnumeratePhysicalDevices(instance, &gpuDeviceCount, NULL);
	assert(result == VK_SUCCESS);

	assert(gpuDeviceCount);

	arrayGPU.resize(gpuDeviceCount);

	result = vkEnumeratePhysicalDevices(instance, &gpuDeviceCount, arrayGPU.data());
	assert(result == VK_SUCCESS);

	return arrayGPU;
}

/////////////////////////////////////////////////////////////////////////////////////////////

VkResult PhysicalDevice::getDeviceExtensionProperties(vector<LayerProperties>& layerPropertyList)
{
	VkResult result = VK_SUCCESS; // Variable to check Vulkan API result status
	uint32_t extensionCount;
	for (LayerProperties& globalLayerProp : layerPropertyList)
	{
		result = vkEnumerateDeviceExtensionProperties(m_physicalDevice, globalLayerProp.properties.layerName, &extensionCount, NULL);
		globalLayerProp.extensions.resize(extensionCount);
		result = vkEnumerateDeviceExtensionProperties(m_physicalDevice, globalLayerProp.properties.layerName, &extensionCount, globalLayerProp.extensions.data());
	}

	return result;
}

/////////////////////////////////////////////////////////////////////////////////////////////

void PhysicalDevice::printDeviceExtensionData(const vector<LayerProperties>& layerPropertyList)
{
	cout << "Device extensions" << endl;
	cout << "===================" << endl;
	for (const LayerProperties& layerProperty : layerPropertyList)
	{
		cout << "\n" << layerProperty.properties.description << "\n\t|\n\t|---[Layer Name]--> " << layerProperty.properties.layerName << "\n";
		if (layerProperty.extensions.size())
		{
			for (auto j : layerProperty.extensions)
			{
				cout << "\t\t|\n\t\t|---[Device Extesion]--> " << j.extensionName << "\n";
			}
		}
		else
		{
			cout << "\t\t|\n\t\t|---[Device Extesion]--> No extension found \n";
		}
	}
}

/////////////////////////////////////////////////////////////////////////////////////////////

void PhysicalDevice::getPhysicalDeviceProperties()
{
	vkGetPhysicalDeviceProperties(m_physicalDevice, &m_physicalDeviceProperties); // Get the physical device or GPU properties
}

/////////////////////////////////////////////////////////////////////////////////////////////

void PhysicalDevice::retrievePhysicalDeviceMemoryProperties()
{
	vkGetPhysicalDeviceMemoryProperties(m_physicalDevice, &m_physicalDeviceMemoryProperties); // Get the memory properties from the physical device or GPU.
}

/////////////////////////////////////////////////////////////////////////////////////////////

void PhysicalDevice::getPhysicalDeviceQueuesAndProperties()
{
	uint32_t queueFamilyCount;
	vkGetPhysicalDeviceQueueFamilyProperties(m_physicalDevice, &queueFamilyCount, NULL); // Query queue families count with pass NULL as second parameter.
	m_queueFamilyProps.resize(queueFamilyCount); // Allocate space to accomodate Queue properties.
	vkGetPhysicalDeviceQueueFamilyProperties(m_physicalDevice, &queueFamilyCount, m_queueFamilyProps.data()); // Get queue family properties
}

/////////////////////////////////////////////////////////////////////////////////////////////

void PhysicalDevice::getGraphicsQueueHandle()
{
	//	1. Get the number of Queues supported by the Physical device
	//	2. Get the properties each Queue type or Queue Family
	//			There could be 4 Queue type or Queue families supported by physical device - 
	//			Graphics Queue	- VK_QUEUE_GRAPHICS_BIT 
	//			Compute Queue	- VK_QUEUE_COMPUTE_BIT
	//			DMA				- VK_QUEUE_TRANSFER_BIT
	//			Sparse memory	- VK_QUEUE_SPARSE_BINDING_BIT
	//	3. Get the index ID for the required Queue family, this ID will act like a handle index to queue.

	bool found = false;
	// 1. Iterate number of Queues supported by the Physical device
	for (unsigned int i = 0; i < m_queueFamilyProps.size(); i++)
	{
		// 2. Get the Graphics Queue type
		//		There could be 4 Queue type or Queue families supported by physical device - 
		//		Graphics Queue		- VK_QUEUE_GRAPHICS_BIT 
		//		Compute Queue		- VK_QUEUE_COMPUTE_BIT
		//		DMA/Transfer Queue	- VK_QUEUE_TRANSFER_BIT
		//		Sparse memory		- VK_QUEUE_SPARSE_BINDING_BIT

		if (m_queueFamilyProps[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)
		{
			// 3. Get the handle/index ID of graphics queue family.
			found = true;
			m_graphicsQueueIndex = i;
			break;
		}
	}

	// Assert if there is no queue found.
	assert(found);
}

/////////////////////////////////////////////////////////////////////////////////////////////

void PhysicalDevice::getComputeQueueHandle()
{
	// Try to find a dedicated compute queue, if not available, then just take the first queue family
	// with compute capabilities
	bool found = false;
	for (unsigned int i = 0; i < m_queueFamilyProps.size(); i++)
	{
		if ((m_queueFamilyProps[i].queueFlags & VK_QUEUE_COMPUTE_BIT) && 
		   ((m_queueFamilyProps[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) == 0))
		{
			found = true;
			m_computeQueueIndex = i;
			break;
		}
	}

	if (!found)
	{
		for (unsigned int i = 0; i < m_queueFamilyProps.size(); i++)
		{
			if (m_queueFamilyProps[i].queueFlags & VK_QUEUE_COMPUTE_BIT)
			{
				found = true;
				m_computeQueueIndex = i;
				break;
			}
		}
	}
	
	assert(found);
}

/////////////////////////////////////////////////////////////////////////////////////////////

void PhysicalDevice::getPhysicalDeviceFeatures()
{
	vkGetPhysicalDeviceFeatures(m_physicalDevice, &m_physicalDeviceFeatures);
}

/////////////////////////////////////////////////////////////////////////////////////////////

bool PhysicalDevice::memoryTypeFromProperties(uint32_t typeBits, VkMemoryPropertyFlags requirementsMask, const VkMemoryType* memoryTypes, uint32_t& typeIndex)
{
	// Search memtypes to find first index with those properties
	for (uint32_t i = 0; i < 32; i++)
	{
		if ((typeBits & 1) == 1)
		{
			// Type is available, does it match user properties?
			if ((memoryTypes[i].propertyFlags & requirementsMask) == requirementsMask)
			{
				typeIndex = i;
				return true;
			}
		}
		typeBits >>= 1;
	}
	// No memory types matched, return failure
	return false;
}

/////////////////////////////////////////////////////////////////////////////////////////////

void PhysicalDevice::obtainPhysicalDeviceRayTracingPipelineProperties()
{
	m_physicalDeviceRayTracingPipelineProperties = {};
	m_physicalDeviceRayTracingPipelineProperties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_PROPERTIES_KHR;

	VkPhysicalDeviceProperties2 physicalDeviceProperties2{};
	physicalDeviceProperties2.sType                    = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
	physicalDeviceProperties2.pNext                    = &m_physicalDeviceRayTracingPipelineProperties;
	vkGetPhysicalDeviceProperties2(m_physicalDevice, &physicalDeviceProperties2);
}

/////////////////////////////////////////////////////////////////////////////////////////////

void PhysicalDevice::obtainPhysicalDeviceSubgroupProperties()
{
	m_physicalDeviceSubgroupProperties = {};
	m_physicalDeviceSubgroupProperties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SUBGROUP_PROPERTIES;

	VkPhysicalDeviceProperties2 physicalDeviceProperties2{};
	physicalDeviceProperties2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
	physicalDeviceProperties2.pNext = &m_physicalDeviceSubgroupProperties;
	vkGetPhysicalDeviceProperties2(m_physicalDevice, &physicalDeviceProperties2);
}

/////////////////////////////////////////////////////////////////////////////////////////////

void PhysicalDevice::obtainPhysicalDeviceConsrvativeRasterizationProperties()
{
	m_physicalDeviceConservativeRasterizationProperties = {};
	m_physicalDeviceConservativeRasterizationProperties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_CONSERVATIVE_RASTERIZATION_PROPERTIES_EXT;

	VkPhysicalDeviceProperties2 physicalDeviceProperties2{};
	physicalDeviceProperties2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
	physicalDeviceProperties2.pNext = &m_physicalDeviceConservativeRasterizationProperties;
	vkGetPhysicalDeviceProperties2(m_physicalDevice, &physicalDeviceProperties2);
}

/////////////////////////////////////////////////////////////////////////////////////////////
