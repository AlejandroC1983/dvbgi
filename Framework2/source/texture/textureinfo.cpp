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
#include "../../include/texture/textureinfo.h"
#include "../../include/texture/irradiancetexture.h"

// NAMESPACE

// DEFINES

// STATIC MEMBER INITIALIZATION

/////////////////////////////////////////////////////////////////////////////////////////////

TextureInfo::TextureInfo() :
	m_data(nullptr)
{

}

/////////////////////////////////////////////////////////////////////////////////////////////

TextureInfo::TextureInfo(gli::texture2D* texture)
{
	m_size                        = uint32_t(texture->size());
	m_data                        = texture->data();
	const uint32_t numMipMapLevel = uint32_t(texture->levels());

	forI(numMipMapLevel)
	{
		m_vectorMipMap.push_back(TextureMipMapInfo(uint32_t((*texture)[i].dimensions().x),
			                                       uint32_t((*texture)[i].dimensions().y),
			                                       uint32_t((*texture)[i].dimensions().z),
			                                       uint32_t((*texture)[i].size())));
	}
}

/////////////////////////////////////////////////////////////////////////////////////////////

TextureInfo::TextureInfo(tinyddsloader::DDSFile* texture)
{
	m_size                        = uint32_t(texture->GetArraySize());
	m_data                        = texture->GetImageData(0, 0)->m_mem;
	const uint32_t numMipMapLevel = uint32_t(texture->GetMipCount());

	forI(numMipMapLevel)
	{
		const tinyddsloader::ImageData* imageData = texture->GetImageData(i, 0);
		
		m_vectorMipMap.push_back(TextureMipMapInfo(imageData->m_width, imageData->m_height, imageData->m_depth, imageData->m_memSlicePitch));
		m_size += imageData->m_memSlicePitch;
	}
}

/////////////////////////////////////////////////////////////////////////////////////////////

TextureInfo::TextureInfo(IrradianceTexture* irradianceTexture)
{
	m_size                        = uint32_t(irradianceTexture->getSize());
	m_data                        = (void*)(irradianceTexture->getVectorTextureFullData().data());
	const uint32_t numMipMapLevel = 1;

	for (uint32_t i = 0; i < numMipMapLevel; ++i)
	{
		m_vectorMipMap.push_back(TextureMipMapInfo(uint32_t(irradianceTexture->getWidth()),
			                                       uint32_t(irradianceTexture->getHeight()),
			                                       uint32_t(irradianceTexture->getDepth()),
			                                       uint32_t(irradianceTexture->getSize())));
	}
}

/////////////////////////////////////////////////////////////////////////////////////////////
