/*
Copyright 2021 Alejandro Cosin

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

#pragma once

#ifndef _FUNCTIONPOINTER_H_
#define _FUNCTIONPOINTER_H_

#pragma once

// GLOBAL INCLUDES

// PROJECT INCLUDES
#include "../../include/headers.h"
#include "../../include/util/singleton.h"

// CLASS FORWARDING

// NAMESPACE

// DEFINES
#define vkfpM s_pVFP->instance()

/////////////////////////////////////////////////////////////////////////////////////////////

class VulkanFunctionPointer : public Singleton<VulkanFunctionPointer>
{
public:
	PFN_vkGetPhysicalDeviceFeatures2KHR         vkGetPhysicalDeviceFeatures2KHR;
	PFN_vkGetBufferDeviceAddressKHR             vkGetBufferDeviceAddress;
	PFN_vkGetAccelerationStructureBuildSizesKHR vkGetAccelerationStructureBuildSizesKHR;
	PFN_vkCreateAccelerationStructureKHR        vkCreateAccelerationStructureKHR;
	PFN_vkCmdBuildAccelerationStructuresKHR     vkCmdBuildAccelerationStructuresKHR;
	PFN_vkCreateRayTracingPipelinesKHR          vkCreateRayTracingPipelinesKHR;
	PFN_vkGetRayTracingShaderGroupHandlesKHR    vkGetRayTracingShaderGroupHandlesKHR;
	PFN_vkCmdTraceRaysKHR                       vkCmdTraceRaysKHR;
	PFN_vkCmdDebugMarkerBeginEXT                vkCmdDebugMarkerBegin;
	PFN_vkCmdDebugMarkerEndEXT                  vkCmdDebugMarkerEnd;
	PFN_vkCmdDebugMarkerInsertEXT               vkCmdDebugMarkerInsert;
	PFN_vkDebugMarkerSetObjectNameEXT           vkDebugMarkerSetObjectName;
	PFN_vkDebugMarkerSetObjectTagEXT            vkDebugMarkerSetObjectTag;
};

/////////////////////////////////////////////////////////////////////////////////////////////

static VulkanFunctionPointer* s_pVFP;

/////////////////////////////////////////////////////////////////////////////////////////////

#endif _FUNCTIONPOINTER_H_
