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

#ifndef _PHYSICALDEVICE_H_
#define _PHYSICALDEVICE_H_

// GLOBAL INCLUDES

// PROJECT INCLUDES
#include "../../include/headers.h"
#include "../../include/util/getsetmacros.h"
#include "../../include/commonnamespace.h"

// CLASS FORWARDING

// NAMESPACE
using namespace commonnamespace;

// DEFINES

/////////////////////////////////////////////////////////////////////////////////////////////

class PhysicalDevice
{
public:
	/** Default constructor
	* @return nothing */
	PhysicalDevice();

	/** Builds a vector with the physical devices found for the instance given
	* @param instance [in] instance to look for physical devices
	* @return a vector with the physical devices found for the instance given */
	vector<VkPhysicalDevice> enumeratePhysicalDevices(VkInstance instance);

	/** Builds sets in the layerPropertyList vector parameter the extension data for each leyer data in the vector
	* @param physicalDevice    [in]    physical device to look for device extensions
	* @param layerPropertyList [inout] vector to fill information for each layer information element in the vector
	* @return result of the computation (VK_SUCCESS if everything went fine) */
	VkResult getDeviceExtensionProperties(vector<LayerProperties>& layerPropertyList);

	/** Prints the extension data of the layers given as input in layerPropertyList
	* @param layerPropertyList [in] vector with the layers to print information
	* @return nothing */
	void printDeviceExtensionData(const vector<LayerProperties>& layerPropertyList);

	/** Returns a struct of type VkPhysicalDeviceProperties with the physical device properties
	* retrieved from the device given as parameter
	* @param physicalDevice [in] physical device to retrieve information from
	* @return nothing */
	void getPhysicalDeviceProperties();

	/** Returns a struct of type VkPhysicalDeviceMemoryProperties with the physical device memory properties
	* retrieved from the device given as parameter
	* @param physicalDevice [in] physical device to retrieve information from
	* @return struct of type VkPhysicalDeviceMemoryProperties with the physical device memory properties retrieved
	* from the device given as parameter
	* @return nothing */
	void retrievePhysicalDeviceMemoryProperties();

	/** Returns a vector with the available queues exposed by the physical devices
	* @param physicalDevice [in] physical device to retrieve information from
	* @return nothing */
	void getPhysicalDeviceQueuesAndProperties();

	/** Returns the handle index of the first graphics queue found in queueFamilyProps
	* @param queueFamilyProps [in] array of VkQueueFamilyProperties to retrieve graphics queue properties from
	* @return nothing */
	void getGraphicsQueueHandle();

	/** Returns the handle index of the first compute queue found in queueFamilyProps
	* @param queueFamilyProps [in] array of VkQueueFamilyProperties to retrieve compute queue properties from
	* @return nothing */
	void getComputeQueueHandle();

	/** Gets the selecred physical device features
	* @return nothing */
	void getPhysicalDeviceFeatures();

	/** Returns the handle index of the first graphics queue found in queueFamilyProps
	* @param typeBits          [in]    memory bit types
	* @param requirements_mask [in]    bit mask describing the requirements
	* @param memoryTypes       [in]    pointer to array of memory types
	* @param typeIndex         [inout] retrieved type index
	* @return true if the memory type was found, false otherwise */
	bool memoryTypeFromProperties(uint32_t typeBits, VkMemoryPropertyFlags requirementMask, const VkMemoryType* memoryTypes, uint32_t& typeIndex);

	/** Gets the ray tracing properties for this physical device
	* @return nothing */
	void obtainPhysicalDeviceRayTracingPipelineProperties();

	/** Gets the subgroup properties for this physical device
	* @return nothing */
	void obtainPhysicalDeviceSubgroupProperties();

	/** Gets the Conservative rasterization properties for this physical device
	* @return nothing */
	void obtainPhysicalDeviceConsrvativeRasterizationProperties();

	GET(vector<VkQueueFamilyProperties>, m_queueFamilyProps, QueueFamilyProps)
	GET(VkPhysicalDeviceMemoryProperties, m_physicalDeviceMemoryProperties, PhysicalDeviceMemoryProperties)
	REF(vector<LayerProperties>, m_layerPropertyList, LayerPropertyList)
	GET_SET(VkPhysicalDevice, m_physicalDevice, PhysicalDevice)
	GETCOPY(uint32_t, m_graphicsQueueIndex, GraphicsQueueIndex)
	GETCOPY(uint32_t, m_computeQueueIndex, ComputeQueueIndex)
	GET(VkPhysicalDeviceProperties, m_physicalDeviceProperties, PhysicalDeviceProperties)
	GET(VkPhysicalDeviceRayTracingPipelinePropertiesKHR, m_physicalDeviceRayTracingPipelineProperties, PhysicalDeviceRayTracingPipelineProperties)
	GET(VkPhysicalDeviceSubgroupProperties, m_physicalDeviceSubgroupProperties, PhysicalDeviceSubgroupProperties)
	GET(VkPhysicalDeviceConservativeRasterizationPropertiesEXT, m_physicalDeviceConservativeRasterizationProperties, PhysicalDeviceConservativeRasterizationProperties)

protected:
	VkPhysicalDeviceFeatures                        m_physicalDeviceFeatures;                     //!< Physical device features: this variable shouldn't be here!!
	vector<VkQueueFamilyProperties>                 m_queueFamilyProps;                           //!< Store all queue families exposed by the physical device. attributes
	uint32_t						                m_graphicsQueueIndex;                         //!< Stores graphics queue index
	uint32_t						                m_computeQueueIndex;                          //!< Stores graphics queue index
	VkPhysicalDevice				                m_physicalDevice;		                      //!< Physical device
	VkPhysicalDeviceProperties		                m_physicalDeviceProperties;                   //!< Physical device attributes
	VkPhysicalDeviceMemoryProperties                m_physicalDeviceMemoryProperties;             //!< Physical device memory properties
	vector<LayerProperties>                         m_layerPropertyList;                          //!< List pof properties of layers
	VkPhysicalDeviceRayTracingPipelinePropertiesKHR m_physicalDeviceRayTracingPipelineProperties; //!< Physical device ray tracing properties
	VkPhysicalDeviceSubgroupProperties              m_physicalDeviceSubgroupProperties;           //!< Physical device subgroup properties
	VkPhysicalDeviceConservativeRasterizationPropertiesEXT m_physicalDeviceConservativeRasterizationProperties; //!< Physical device conservative rasterization properties
};

/////////////////////////////////////////////////////////////////////////////////////////////

#endif _PHYSICALDEVICE_H_
