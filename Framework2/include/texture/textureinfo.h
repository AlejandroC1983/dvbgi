/*
Copyright 2017 Alejandro Cosin

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

#ifndef _TEXTUREINFO_H_
#define _TEXTUREINFO_H_

// GLOBAL INCLUDES
#include "../../external/tinyddsloader/tinyddsloader.hpp"

// PROJECT INCLUDES
#include "../../include/util/singleton.h"
#include "../../include/util/managertemplate.h"
#include "../headers.h"

// CLASS FORWARDING
class IrradianceTexture;

// NAMESPACE

// DEFINES

/////////////////////////////////////////////////////////////////////////////////////////////

/** Simple helper to abstract texture mip map level information from the library / resource used to load it */
struct TextureMipMapInfo
{
	/** Default constructor
	* @return nothing */
	TextureMipMapInfo()
	{

	}

	TextureMipMapInfo(uint32_t width, uint32_t height, uint32_t depth, uint32_t size):
		  m_width(width)
		, m_height(height)
		, m_depth(depth)
		, m_size(size)
	{

	}

	uint32_t m_width;  //!< Texture width
	uint32_t m_height; //!< Texture height
	uint32_t m_depth;  //!< Texture depth
	uint32_t m_size;   //!< Texture mip map level size
};

/////////////////////////////////////////////////////////////////////////////////////////////

/** Simple helper to abstract texture information from the library / resource used to load it */
class TextureInfo
{
public:
	/** Default constructor
	* @return nothing */
	TextureInfo();

	/** Parameter constructor
	* @param texture [in] gli texture to get information from
	* @return nothing */
	TextureInfo(gli::texture2D* texture);

	/** Parameter constructor
	* @param texture [in] dds texture to get information from
	* @return nothing */
	TextureInfo(tinyddsloader::DDSFile* texture);

	/** Parameter constructor
	* @param irradianceTexture [in] .irrt texture to get information from
	* @return nothing */
	TextureInfo(IrradianceTexture* irradianceTexture);

	REF_PTR(void, m_data, Data)
	GETCOPY(uint32_t, m_size, Size)
	GET(vector<TextureMipMapInfo>, m_vectorMipMap, VectorMipMap)

protected:
	void*                     m_data;         //!< Texture data
	uint32_t                  m_size;         //!< Texture whole size (all mipmap data together)
	vector<TextureMipMapInfo> m_vectorMipMap; //!< Vector with the number of mip map levels 
};

/////////////////////////////////////////////////////////////////////////////////////////////

#endif _TEXTUREINFO_H_
