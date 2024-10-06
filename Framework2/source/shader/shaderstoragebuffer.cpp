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

// GLOBAL INCLUDES

// PROJECT INCLUDES
#include "../../include/shader/shaderstoragebuffer.h"
#include "../../include/buffer/buffer.h"
#include "../../include/texture/texture.h"
#include "../../include/util/loopmacrodefines.h"

// NAMESPACE

// DEFINES

// STATIC MEMBER INITIALIZATION

/////////////////////////////////////////////////////////////////////////////////////////////

ShaderStorageBuffer::ShaderStorageBuffer(): ShaderStruct()
	//, m_buffer(nullptr)
{

}

/////////////////////////////////////////////////////////////////////////////////////////////

ShaderStorageBuffer::ShaderStorageBuffer(string               &&name,
		                                 VkShaderStageFlagBits  shaderStage,
		                                 int                    bindingIndex,
		                                 int                    setIndex,
		                                 string               &&variableName,
		                                 string               &&structName) : 
	ShaderStruct(move(name),
				 move(string("ShaderStorageBuffer")),
		         ResourceInternalType::RIT_STRUCT,
		         shaderStage,
		         bindingIndex,
		         setIndex,
		         move(variableName),
		         move(structName))
{

}

/////////////////////////////////////////////////////////////////////////////////////////////

ShaderStorageBuffer::~ShaderStorageBuffer()
{

}

/////////////////////////////////////////////////////////////////////////////////////////////

void ShaderStorageBuffer::buildDescriptorBufferInfoVector()
{
	const uint maxIndex = m_vectorBufferPtr.size();
	m_vectorDescriptorBufferInfo.resize(maxIndex);

	forI(maxIndex)
	{
		m_vectorDescriptorBufferInfo[i] = m_vectorBufferPtr[i]->refDescriptorBufferInfo();
	}
}

/////////////////////////////////////////////////////////////////////////////////////////////
