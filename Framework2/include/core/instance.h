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

#ifndef _INSTANCE_H_
#define _INSTANCE_H_

// GLOBAL INCLUDES

// PROJECT INCLUDES
#include "../../include/headers.h"
#include "../../include/util/getsetmacros.h"

// CLASS FORWARDING

// NAMESPACE

// DEFINES

/////////////////////////////////////////////////////////////////////////////////////////////

class Instance
{
public:
	/** Default constructor
	* @return nothing */
	Instance();

	/** Retrieves the layer properties available, storing them in the arrayLayerPropertyList given as parameter
	* @param arrayLayerPropertyList [inout] array to store layer information
	* @return nothing */
	void getInstanceLayerProperties(vector<LayerProperties>& arrayLayerPropertyList);

	/** Prints the information present in the arrayLayerPropertyList given as parameter
	* @param arrayLayerPropertyList [in] array with the layer information to print
	* @return nothing */
	void printLayerAndExtensionData(const vector<LayerProperties>& arrayLayerPropertyList);

	/** Inspects the incoming layer names against system supported layers, theses layers are not supported then this function
	* removed it from arrayLayerNames allow
	* @param arrayLayerNames        [in] array with the layer names to know if they're supported
	* @param arrayLayerPropertyList [in] array with the layer properties to know if elements in arrayLayerNames are supported
	* @return nothing */
	void areLayersSupported(vector<const char*>& arrayLayerNames, const vector<LayerProperties>& arrayLayerPropertyList);

	/** Builds an instance with the layers and extensions given as parameter
	* @param arrayLayer      [in] array with the layer names to add to the instance
	* @param arrayExtension  [in] array with the extension names to add to the instance
	* @param applicationName [in] application name
	* @return the built instance */
	void createInstance(vector<const char*>& arrayLayer, vector<const char*>& arrayExtension, const char* applicationName);

	/** Destroys the given instance
	* @param instance [inout] instance to destroy
	* @return nothing */
	void destroyInstance();

	/** Function called when there's debug information once the debug report callback has been built
	* @param msgFlags    [in] flags for the message
	* @param objType     [in] object type
	* @param srcObject   [in] source object
	* @param location    [in] location
	* @param msgCode     [in] message code
	* @param layerPrefix [in] prefix of the layer calling
	* @param msg         [in] message
	* @param userData    [in] user data
	* @return true if the type of debug function was identified and false otherwise */
	static VKAPI_ATTR VkBool32 VKAPI_CALL debugFunction(VkFlags msgFlags,
		VkDebugReportObjectTypeEXT objType,
		uint64_t srcObject,
		size_t location,
		int32_t msgCode,
		const char *layerPrefix,
		const char *msg,
		void *userData);

	/** Builds the debug report callback function
	* @return result of the operation */
	VkResult createDebugReportCallback();

	/** Destroys the report callback
	* @return nothing */
	void destroyDebugReportCallback();

	GET(VkInstance, m_instance, Instance)

protected:
	PFN_vkCreateDebugReportCallbackEXT  m_dbgCreateDebugReportCallback;  //!< Function pointer to build the debugging report callback
	PFN_vkDestroyDebugReportCallbackEXT m_dbgDestroyDebugReportCallback; //!< Function pointer to destroy the debugging report callback
	VkDebugReportCallbackEXT            m_debugReportCallback;           //!< Function pointer to the debug callback function
	VkDebugReportCallbackCreateInfoEXT  m_dbgReportCreateInfo;           //!< Struct used to store flags and data when building the debug report callback
    VkInstance                          m_instance;                      //!< Vulkan instance used
};

/////////////////////////////////////////////////////////////////////////////////////////////

#endif _INSTANCE_H_
