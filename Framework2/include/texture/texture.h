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

#ifndef _TEXTURE_H_
#define _TEXTURE_H_

// GLOBAL INCLUDES

// PROJECT INCLUDES
#include "../headers.h"
#include "../../include/util/genericresource.h"

// CLASS FORWARDING

// NAMESPACE

// DEFINES

/////////////////////////////////////////////////////////////////////////////////////////////

class Texture : public GenericResource
{
	friend class TextureManager;

protected:
	/** Parameter constructor
	* @param [in] texture's name
	* @return nothing */
	Texture(string &&name);

public:
	/** Destructor
	* @return nothing */
	virtual ~Texture();

	/** Returns a vector of byte with the information of the image memory
	* @return true if the copy operation was made successfully, false otherwise */
	bool getContentCopy(vectorUint8& vectorData);

	/** Utility method to knowif a specific texture format is a depth format (with or without stencil information)
	* @return true if the format given as parameter is a depth format */
	static bool isDepthTexture(VkFormat format);

	/** Utility method to knowif a specific texture format is a depth-stencil format
	* @return true if the format given as parameter is a depth-stencil format */
	static bool isDepthStencilTexture(VkFormat format);

	GET(VkImage, m_image, Image)
	GETCOPY(VkImageLayout, m_imageLayout, ImageLayout)
	GET(VkDeviceMemory, m_mem, Mem)
	GET(VkImageView, m_view, View)
	GETCOPY(uint32_t, m_mipMapLevels, MipMapLevels)
	GETCOPY(uint32_t, m_layerCount, LayerCount)
	GETCOPY(uint32_t, m_width, Width)
	GETCOPY(uint32_t, m_height, Height)
	GETCOPY(uint32_t, m_depth, Depth)
	GETCOPY(bool, m_generateMipmap, GenerateMipmap)
	GETCOPY(string, m_path, Path)
	GETCOPY(VkImageUsageFlags, m_imageUsageFlags, ImageUsageFlags)
	GETCOPY(VkFormat, m_format, Format)
	GETCOPY(VkImageViewType, m_imageViewType, ImageViewType)
	GETCOPY(VkImageCreateFlags, m_flags, Flags)
	GETCOPY_SET(bool, m_isSwapChainTex, IsSwapChainTex)
	GET(VkImageView, m_view3DAttachment, View3DAttachment)

protected:
	VkImage            m_image;           //!< Texture image
	VkImageLayout      m_imageLayout;     //!< Enum with the image layout
	VkDeviceMemory     m_mem;             //!< Image memory
	VkDeviceSize       m_memorySize;      //!< Size of the image memory
	VkImageView        m_view;            //!< Image view
	uint32_t           m_mipMapLevels;    //!< Number of image mip-map levels
	uint32_t           m_layerCount;      //!< Number of image layers
	uint32_t           m_width;           //!< Image width
	uint32_t           m_height;          //!< Image height
	uint32_t           m_depth;           //!< Image depth
	bool               m_generateMipmap;  //!< If true, mipmaps will be generated when setting the data of the texture
	string             m_path;            //!< Texture path to the image file this texure has loaded (if any)
	VkImageUsageFlags  m_imageUsageFlags; //!< Enum with the usage flags of this image
	VkFormat           m_format;          //!< Image format
	VkImageViewType    m_imageViewType;   //!< Image view type
	VkImageCreateFlags m_flags;           //!< Image flags
	bool               m_isSwapChainTex;  //!< Flag to know if this texture is a swapchain texture
	VkImageView        m_view3DAttachment; //!< Extra image view in case this texture is a 3D texturee used as image attachment (as 2D array)
};

/////////////////////////////////////////////////////////////////////////////////////////////

#endif _TEXTURE_H_
