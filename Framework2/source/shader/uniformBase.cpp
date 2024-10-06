/*
Copyright 2014 Alejandro Cosin

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
#include "../../include/shader/uniformBase.h"

// NAMESPACE

// DEFINES

// STATIC MEMBER INITIALIZATION

/////////////////////////////////////////////////////////////////////////////////////////////

UniformBase::UniformBase(const ResourceInternalType type, VkShaderStageFlagBits shaderStage, string&& name, string&& className, string&& structName, string&& structType, ShaderStruct* shaderStructOwner) : GenericResource(move(string(name)), move(className), GenericResourceType::GRT_UNIFORMBASE)
	, m_type(type)
	, m_shaderStage(shaderStage)
	, m_shaderStructOwner(shaderStructOwner)
	, m_name(move(name))
	, m_structName(move(structName))
	, m_structType(move(structType))
{

}

/////////////////////////////////////////////////////////////////////////////////////////////
