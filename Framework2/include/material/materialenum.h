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

#ifndef _MATERIALENUM_H_
#define _MATERIALENUM_H_

// GLOBAL INCLUDES

// PROJECT INCLUDES

// CLASS FORWARDING

// NAMESPACE

// DEFINES

namespace materialenum
{
	/////////////////////////////////////////////////////////////////////////////////////////////

	/** Enum to know which of the dynamic uniform buffers built by the raster manager are used by each material */
	enum MaterialBufferResource
	{
		MBR_NONE     = 1 << 0, //!< No element
		MBR_MODEL    = 1 << 1, //!< Scene per-model data
		MBR_CAMERA   = 1 << 2, //!< Scene camera information
		MBR_MATERIAL = 1 << 4  //!< Scene material data
	};

	/////////////////////////////////////////////////////////////////////////////////////////////

	inline MaterialBufferResource operator|(MaterialBufferResource a, MaterialBufferResource b)
	{
		return static_cast<MaterialBufferResource>(static_cast<unsigned int>(a) | static_cast<unsigned int>(b));
	}

	/////////////////////////////////////////////////////////////////////////////////////////////

	/** Enum to tag the shader stage */
	enum class ShaderStageFlag
	{
		SSF_NONE          = 0x00000001,
		SSF_VERTEX        = 0x00000002,
		SSF_GEOMETRY      = 0x00000004,
		SSF_FRAGMENT      = 0x00000008,
		SSF_COMPUTE       = 0x00000010,
		SSF_RAYGENERATION = 0x00000020,
		SSF_RAYHIT        = 0x00000040,
		SSF_RAYMISS       = 0x00000080,
	};

	/////////////////////////////////////////////////////////////////////////////////////////////

	inline ShaderStageFlag operator|(ShaderStageFlag a, ShaderStageFlag b)
	{
		return static_cast<ShaderStageFlag>(static_cast<uint>(a) | static_cast<uint>(b));
	}

	/////////////////////////////////////////////////////////////////////////////////////////////

	inline bool operator&(ShaderStageFlag a, ShaderStageFlag b)
	{
		return (static_cast<uint>(a) & static_cast<uint>(b));
	}

	/////////////////////////////////////////////////////////////////////////////////////////////

	/** GLSL version for the "#version xyz" shader preprocessor to be added at the beggining of the shader */
	enum class GLSLShaderVersion
	{
		GLSLSV_3_0 = 0,
		GLSLSV_3_1 = 1,
		GLSLSV_3_2 = 2,
		GLSLSV_3_3 = 3,
		GLSLSV_4_0 = 4,
		GLSLSV_4_1 = 5,
		GLSLSV_4_2 = 6,
		GLSLSV_4_3 = 7,
		GLSLSV_4_4 = 8,
		GLSLSV_4_5 = 9,
		GLSLSV_4_6 = 10,
	};

	/////////////////////////////////////////////////////////////////////////////////////////////

	/** Enum to know which kind of surface is expected to be rendered with a material */
	enum MaterialSurfaceType
	{
		MST_OPAQUE = 0, //!< Opaque geometry
		MST_ALPHATESTED = 1, //!< Opaque, alpha tested geometry
		MST_ALPHABLENDED = 2, //!< Alpha blender geometry
		MST_SIZE = 3  //!< Enum max value
	};

	/////////////////////////////////////////////////////////////////////////////////////////////
}

#endif _RESOURCEENUM_H_
