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

#ifndef _PUSH_CONSTANT_H_
#define _PUSH_CONSTANT_H_

// GLOBAL INCLUDES

// PROJECT INCLUDES
#include "../headers.h"
#include "../../include/util/genericresource.h"
#include "../../include/uniformbuffer/cpubuffer.h"

// CLASS FORWARDING

// NAMESPACE

// DEFINES

/////////////////////////////////////////////////////////////////////////////////////////////

class PushConstant
{
public:
	/** Default constructor
	* @return nothing */
	PushConstant();

	/** Destructor
	* @return nothing */
	virtual ~PushConstant();

	REF(vectorUniformBasePtr, m_vecUniformBase, VecUniformBase)
	REF(CPUBuffer, m_CPUBuffer, CPUBuffer)
	GETCOPY_SET(VkShaderStageFlags, m_shaderStages, ShaderStages)

public:
	vectorUniformBasePtr m_vecUniformBase; //!< Vector with the uniform variables present in the push constant struct
	CPUBuffer            m_CPUBuffer;      //!< CPU bufer mapping the information of the GPU buffer
	VkShaderStageFlags   m_shaderStages;   //!< All shader stages in which the push constant is used (assuming a single push constant per whole shader, with no differencies between sgader stages)
};

/////////////////////////////////////////////////////////////////////////////////////////////

#endif _PUSH_CONSTANT_H_
