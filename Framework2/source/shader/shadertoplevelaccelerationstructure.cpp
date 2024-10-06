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

// GLOBAL INCLUDES

// PROJECT INCLUDES
#include "../../include/shader/shadertoplevelaccelerationstructure.h"
#include "../../include/accelerationstructure/accelerationstructure.h"

// NAMESPACE

// DEFINES

// STATIC MEMBER INITIALIZATION

/////////////////////////////////////////////////////////////////////////////////////////////

ShaderTopLevelAccelerationStructure::ShaderTopLevelAccelerationStructure(): ShaderStruct()
	, m_accelerationStructure(nullptr)
	, m_descriptorType(VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR)
{

}

/////////////////////////////////////////////////////////////////////////////////////////////

ShaderTopLevelAccelerationStructure::ShaderTopLevelAccelerationStructure(string               &&name,
		                                 VkShaderStageFlagBits  shaderStage,
		                                 int                    bindingIndex,
		                                 int                    setIndex,
		                                 string               &&variableName,
		                                 string               &&structName) : 
	ShaderStruct(move(name),
				 move(string("ShaderTopLevelAccelerationStructure")),
		         ResourceInternalType::RIT_ACCELERATION_STRUCTURE,
		         shaderStage,
		         bindingIndex,
		         setIndex,
		         move(variableName),
		         move(structName))
	, m_accelerationStructure(nullptr)
	, m_descriptorType(VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR)
{

}

/////////////////////////////////////////////////////////////////////////////////////////////

ShaderTopLevelAccelerationStructure::~ShaderTopLevelAccelerationStructure()
{

}

/////////////////////////////////////////////////////////////////////////////////////////////

void ShaderTopLevelAccelerationStructure::buildWriteDescriptorSetAccelerationStructure()
{
	assert(m_accelerationStructure != nullptr);

	m_writeDescriptorSetAccelerationStructure.sType                      = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET_ACCELERATION_STRUCTURE_KHR;
	m_writeDescriptorSetAccelerationStructure.pNext                      = nullptr;
	m_writeDescriptorSetAccelerationStructure.accelerationStructureCount = 1;
	m_writeDescriptorSetAccelerationStructure.pAccelerationStructures    = &m_accelerationStructure->refTlas();
}

/////////////////////////////////////////////////////////////////////////////////////////////
