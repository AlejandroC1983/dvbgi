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

#ifndef _MODELENUM_H_
#define _MODELENUM_H_

// GLOBAL INCLUDES
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

// PROJECT INCLUDES
#include "../headers.h"

// CLASS FORWARDING

// NAMESPACE
using namespace commonnamespace;

// DEFINES

namespace modelenum
{

	/////////////////////////////////////////////////////////////////////////////////////////////

	static vec3 vec3AssimpCast(const aiVector3D v)
	{
		return vec3(v.x, v.y, v.z);
	}

	/////////////////////////////////////////////////////////////////////////////////////////////

	static vec2 vec2AssimpCast(const aiVector3D v)
	{
		return vec2(v.x, v.y);
	}

	/////////////////////////////////////////////////////////////////////////////////////////////

	static quat quatAssimpCast(const aiQuaternion q)
	{
		return quat(q.w, q.x, q.y, q.z);
	}

	/////////////////////////////////////////////////////////////////////////////////////////////

	static mat4 mat4AssimpCast(const aiMatrix4x4& m)
	{
		return transpose(make_mat4(&m.a1));
	}

	/////////////////////////////////////////////////////////////////////////////////////////////
}

#endif _MODELENUM_H_