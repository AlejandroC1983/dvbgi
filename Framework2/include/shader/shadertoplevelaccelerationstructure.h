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

#ifndef _SHADERTOPLEVELACCELERATIONSTRUCUTURE_H_
#define _SHADERTOPLEVELACCELERATIONSTRUCUTURE_H_

// GLOBAL INCLUDES

// PROJECT INCLUDES
#include "../../include/shader/shaderstruct.h"
#include "../../include/shader/resourceenum.h"

// CLASS FORWARDING
class AccelerationStructure;

// NAMESPACE
using namespace commonnamespace;
using namespace resourceenum;

// DEFINES

/////////////////////////////////////////////////////////////////////////////////////////////

class ShaderTopLevelAccelerationStructure : public ShaderStruct
{
public:
	/** Default constructor
	* @return nothing */
	ShaderTopLevelAccelerationStructure();

	/** Parameter constructor
	* @param name              [in] shader storage buffer variable name (hasn't to do with declared variables in the shader, is this C++'s class instance name)
	* @param shaderStage       [in] enumerated with value the shader stage the shader storage buffer being built is used
	* @param bindingIndex      [in] binding index of this shader storage buffer buffer for the shader it's owned by
	* @param setIndex          [in] set index inside the binding index given by bindingIndex of this shader storage buffer buffer for the shader it's owned by
	* @param variableName      [in] name of the variable in the shader
	* @param structName        [in] name of the variable in the struct
	* @return nothing */
	ShaderTopLevelAccelerationStructure(string               &&name,
		                                VkShaderStageFlagBits  shaderStage,
		                                int                    bindingIndex,
		                                int                    setIndex,
		                                string               &&variableName,
		                                string               &&structName);

	/** Default destructor
	* @return nothing */
	virtual ~ShaderTopLevelAccelerationStructure();

	/** Fill the information in m_writeDescriptorSetAccelerationStructure, needed for Material::buildPipeline
	* @return nothing */
	void buildWriteDescriptorSetAccelerationStructure();
	
	GET_PTR_SET_PTR(AccelerationStructure, m_accelerationStructure, AccelerationStructure)
	REF_SET(string, m_accelerationStructureName, AccelerationStructureName)
	GETCOPY_SET(VkDescriptorType, m_descriptorType, DescriptorType)
	REF_RETURN_PTR(VkWriteDescriptorSetAccelerationStructureKHR, m_writeDescriptorSetAccelerationStructure, WriteDescriptorSetAccelerationStructure)
	
protected:
	AccelerationStructure*                       m_accelerationStructure;                   //!< Acceleration structure used in this shader
	string                                       m_accelerationStructureName;               //!< Name of the acceleration structure resource assigned to m_accelerationStructure
	VkDescriptorType                             m_descriptorType;                          //!< Flags for the descriptor set built for this acceleration structure
	VkWriteDescriptorSetAccelerationStructureKHR m_writeDescriptorSetAccelerationStructure; //!< Write descriptor set used in Material::buildPipeline
};

/////////////////////////////////////////////////////////////////////////////////////////////

#endif _SHADERTOPLEVELACCELERATIONSTRUCUTURE_H_
