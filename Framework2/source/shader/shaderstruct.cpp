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
#include "../../include/shader/shaderstruct.h"

// NAMESPACE

// DEFINES

// STATIC MEMBER INITIALIZATION

/////////////////////////////////////////////////////////////////////////////////////////////

ShaderStruct::ShaderStruct():
	  m_uniformBufferType(ResourceInternalType::RIT_SIZE)
	, m_bindingIndex(-1)
	, m_setIndex(-1)
{

}

/////////////////////////////////////////////////////////////////////////////////////////////

ShaderStruct::ShaderStruct(string               &&name,
						   string               &&className,
		                   ResourceInternalType   uniformBufferType,
              	           VkShaderStageFlagBits  shaderStage,
		                   int                    bindingIndex,
		                   int                    setIndex,
		                   string               &&variableName,
		                   string               &&structName) : GenericResource(move(name), move(className), GenericResourceType::GRT_SHADERSTRUCT)
	, m_uniformBufferType(uniformBufferType)
	, m_shaderStage(shaderStage)
	, m_bindingIndex(bindingIndex)
	, m_setIndex(setIndex)
	, m_variableName(move(variableName))
	, m_structName(move(structName))
{

}

/////////////////////////////////////////////////////////////////////////////////////////////

ShaderStruct::~ShaderStruct()
{

}

/////////////////////////////////////////////////////////////////////////////////////////////
