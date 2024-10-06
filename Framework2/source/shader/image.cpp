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
#include "../../include/shader/image.h"
#include "../../include/texture/texture.h"
#include "../../include/texture/texturemanager.h"

// NAMESPACE

// DEFINES

// STATIC MEMBER INITIALIZATION

/////////////////////////////////////////////////////////////////////////////////////////////

Image::Image(const int &id, string &&name, const ResourceInternalType &imageType, VkShaderStageFlagBits shaderStage) : GenericResource(move(name), move(string("Image")), GenericResourceType::GRT_IMAGE),
	m_id(id),
	m_unit(-1),
	m_imageType(imageType),
	m_shaderStage(shaderStage),
	m_hasAssignedTexture(false),
	m_hasAssignedGPUBuffer(false),
	m_texture(nullptr),
	m_gpuBuffer(nullptr),
	m_textureLevel(-1),
	m_layered(false),
	m_textureLayer(-1),
	m_accessType(ImageAccessType::IT_READ_ONLY),
	m_storingFormat(VkFormat::VK_FORMAT_MAX_ENUM),
	m_textureToSampleName(""),
	m_gpuBufferToSampleName("")
{

}

/////////////////////////////////////////////////////////////////////////////////////////////

bool Image::setTextureToImage(Texture *texture, const int unit, const int &textureLevel, const bool &layered, const int &textureLayer, const ImageAccessType &imageAccessType)
{
	if(texture == nullptr)
	{
		return false;
	}

	resetNonConstructorParameterValues();

	m_unit				  = unit;
	m_hasAssignedTexture  = true;
	m_texture             = texture;
	m_textureLevel        = textureLevel;
	m_layered             = layered;
	m_textureLayer        = textureLayer;
	m_accessType          = imageAccessType;
	m_textureToSampleName = m_texture->getName();

	return true;
}

/////////////////////////////////////////////////////////////////////////////////////////////

bool Image::setTextureToImage(string &&name, const int unit, const int &textureLevel, const bool &layered, const int &textureLayer, const ImageAccessType &imageAccessType)
{
	resetNonConstructorParameterValues();

	m_texture			  = nullptr;
	m_hasAssignedTexture  = setTextureToImage(textureM->getElement(move(name)), unit, textureLevel, layered, textureLayer, imageAccessType);
	m_textureToSampleName = move(name);

	return m_hasAssignedTexture;
}

/////////////////////////////////////////////////////////////////////////////////////////////

bool Image::setGPUBufferToImage(GPUBuffer *gpuBuffer, const int unit, const ImageAccessType &imageAccessType)
{
	return true;
}

/////////////////////////////////////////////////////////////////////////////////////////////

bool Image::setGPUBufferToImage(string &&name, const int unit, const ImageAccessType &imageAccessType)
{
	return m_hasAssignedTexture;
}

/////////////////////////////////////////////////////////////////////////////////////////////

Image::~Image()
{

}

/////////////////////////////////////////////////////////////////////////////////////////////

void Image::resetNonConstructorParameterValues()
{
	m_unit                  = -1;
	m_hasAssignedTexture    = false;
	m_hasAssignedGPUBuffer  = false;
	m_texture               = nullptr;
	m_gpuBuffer             = nullptr;
	m_textureLevel          = -1;
	m_layered               = false;
	m_textureLayer          = -1;
	m_accessType            = ImageAccessType::IT_READ_ONLY;
	m_storingFormat         = VkFormat::VK_FORMAT_MAX_ENUM;
	m_textureToSampleName   = "";
	m_gpuBufferToSampleName = "";
}

/////////////////////////////////////////////////////////////////////////////////////////////
