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

#ifndef _IRRADIANCETEXTURE_H_
#define _IRRADIANCETEXTURE_H_

// GLOBAL INCLUDES

// PROJECT INCLUDES
#include "../headers.h"
#include "../../include/util/getsetmacros.h"

// CLASS FORWARDING

// NAMESPACE

// DEFINES

/////////////////////////////////////////////////////////////////////////////////////////////

/** Simple helper to abstract texture information from the library / resource used to load it */
class IrradianceTexture
{
public:
	/** Default constructor
	* @return nothing */
	IrradianceTexture();

	/** Parameter constructor
	* @param path [in] path to the .irrt texture
	* @return nothing */
	IrradianceTexture(string&& path);

	GETCOPY(uint, m_width, Width)
	GETCOPY(uint, m_height, Height)
	GETCOPY(uint, m_depth, Depth)
	GETCOPY(uint, m_size, Size)
	GET(vector<float>, m_vectorDistance, VectorDistance)
	GET(vector<uint>, m_vectorTextureSize, VectorTextureSize)
	GET(vector<float>, m_vectorTextureFullData, VectorTextureFullData)

protected:
	uint                  m_width;                 //!< Texture width
	uint                  m_height;                //!< Texture height
	uint                  m_depth;                 //!< Texture depth (number of irradiance layers computed and placed together in this texture)
	uint                  m_size;                  //!< Size in bytes of m_vectorTextureFullData
	vector<float>         m_vectorDistance;        //!< Irradiance computatoin distance associated to each one of the textures present in m_vectorTextureFullData
	vector<uint>          m_vectorTextureSize;     //!< Size of each one of the textures in the m_vectorTextureFullData vector
	vector<float>         m_vectorTextureFullData; //!< Full texture data (all irradiance textures consecutively)
	vector<vector<float>> m_vectorTextureData;     //!< Texture data (all irradiance textures consecutively)
};

/////////////////////////////////////////////////////////////////////////////////////////////

#endif _IRRADIANCETEXTURE_H_
