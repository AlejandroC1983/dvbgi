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

#ifndef _SHADERSTORAGEBUFFER_H_
#define _SHADERSTORAGEBUFFER_H_

// GLOBAL INCLUDES

// PROJECT INCLUDES
#include "../../include/shader/shaderstruct.h"
#include "../../include/shader/resourceenum.h"

// CLASS FORWARDING
class Buffer;

// NAMESPACE
using namespace commonnamespace;
using namespace resourceenum;

// DEFINES

/////////////////////////////////////////////////////////////////////////////////////////////

class ShaderStorageBuffer : public ShaderStruct
{
public:
	/** Default constructor
	* @return nothing */
	ShaderStorageBuffer();

	/** Parameter constructor
	* @param name              [in] shader storage buffer variable name (hasn't to do with declared variables in the shader, is this C++'s class instance name)
	* @param shaderStage       [in] enumerated with value the shader stage the shader storage buffer being built is used
	* @param bindingIndex      [in] binding index of this shader storage buffer buffer for the shader it's owned by
	* @param setIndex          [in] set index inside the binding index given by bindingIndex of this shader storage buffer buffer for the shader it's owned by
	* @param variableName      [in] name of the variable in the shader
	* @param structName        [in] name of the variable in the struct
	* @return nothing */
	ShaderStorageBuffer(string               &&name,
		                VkShaderStageFlagBits  shaderStage,
		                int                    bindingIndex,
		                int                    setIndex,
		                string               &&variableName,
		                string               &&structName);

	/** Default destructor
	* @return nothing */
	virtual ~ShaderStorageBuffer();

	/** Helper function to add to m_vectorDescriptorBufferInfo the descriptor buffer info for each buffer present in m_vectorBufferPtr
	* @return nothing */
	void buildDescriptorBufferInfoVector();
	
	GET_SET(vectorString, m_vectorStructFieldName, VectorStructFieldName)
	GET_SET(vectorString, m_vectorStructFieldType, VectorStructFieldType)
	GET_SET(vector<ResourceInternalType>, m_vectorInternalType, VectorInternalType)
	GETCOPY_SET(VkDescriptorType, m_descriptorType, DescriptorType)
	REF_SET(vectorBufferPtr, m_vectorBufferPtr, VectorBufferPtr)
	REF_SET(vectorString, m_vectorBufferName, VectorBufferName)
	REF(vectorDescriptorBufferInfo, m_vectorDescriptorBufferInfo, VectorDescriptorBufferInfo)
	
protected:
	vectorString                 m_vectorStructFieldName;      //!< String array with the name of each storage buffer field
	vectorString                 m_vectorStructFieldType;      //!< String array with the type of each storage buffer field
	vector<ResourceInternalType> m_vectorInternalType;         //!< Contains the type of each storage buffer field (if in the possible values of ResourceInternalType)
	VkDescriptorType             m_descriptorType;             //!< Flags for the descriptor set built for this shader storage buffer
	VkDescriptorBufferInfo	     m_bufferInfo;                 //!< Sahder storage buffer information struct
	vectorBufferPtr              m_vectorBufferPtr;            //!< Vector with pointer to the buffers used in this shader storage buffer in case the SSBO is using an array of buffers instead only one, in which case it will be in m_buffer
	vectorString                 m_vectorBufferName;           //!< Vector with the names of the buffers in m_vectorBufferPtr used in this shader storage buffer in case the SSBO is using an array of buffers instead only one, in which case it will be in m_buffer
	vectorDescriptorBufferInfo   m_vectorDescriptorBufferInfo; //!< Used in Material::buildPipeline to build a vector with the descriptor buffer info for each buffer present in m_vectorBufferPtr
};

/////////////////////////////////////////////////////////////////////////////////////////////

#endif _SHADERSTORAGEBUFFER_H_
