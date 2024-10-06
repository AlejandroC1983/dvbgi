/*
Copyright 2022 Alejandro Cosin

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

#ifndef _COMPONENTENUM_H_
#define _COMPONENTENUM_H_

// GLOBAL INCLUDES

// PROJECT INCLUDES
#include "../headers.h"

// CLASS FORWARDING

// NAMESPACE
using namespace commonnamespace;

// DEFINES

namespace componentenum
{

#define MAXIMUM_BONES_PER_SKELETAL_MESH 64
#define MAXIMUM_BONES_PER_VERTEX 4

	/////////////////////////////////////////////////////////////////////////////////////////////

	struct BoneInformation
	{
		BoneInformation() :
			  m_name("")
			, m_index(-1)
		{

		};

		BoneInformation(string&& name, int index, mat4& offsetMatrix) :
			  m_name(move(name))
			, m_index(index)
			, m_offsetMatrix(offsetMatrix)
		{

		};

		string       m_name;            //!< Bone name
		int          m_index;           //!< Bone index
		vectorString m_parentBone;      //!< Vector with the names of the parent phones for this bone until the root bone
		vectorInt    m_parentBoneIndex; //!< Vector with the index of the parent phones for this bone until the root bone
		mat4         m_offsetMatrix;    //!< Offset matrix of this bone
	};

	/////////////////////////////////////////////////////////////////////////////////////////////
}

#endif _COMPONENTENUM_H_