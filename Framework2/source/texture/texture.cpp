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

#include "../../include/texture/texture.h"
#include "../../include/core/coremanager.h"

// GLOBAL INCLUDES

// PROJECT INCLUDES
#include "../../include/texture/texture.h"

// NAMESPACE

// DEFINES

// STATIC MEMBER INITIALIZATION

/////////////////////////////////////////////////////////////////////////////////////////////

// TODO initialize properly
Texture::Texture(string &&name) : GenericResource(move(name), move(string("Texture")), GenericResourceType::GRT_TEXTURE)
	, m_image(VK_NULL_HANDLE)
	, m_imageLayout(VkImageLayout::VK_IMAGE_LAYOUT_UNDEFINED)
	, m_mem(VK_NULL_HANDLE)
	, m_memorySize(0)
	, m_view(VK_NULL_HANDLE)
	, m_mipMapLevels(0)
	, m_layerCount(0)
	, m_width(0)
	, m_height(0)
	, m_depth(0)
	, m_generateMipmap(false)
	, m_imageUsageFlags(VkImageUsageFlagBits::VK_IMAGE_USAGE_FLAG_BITS_MAX_ENUM)
	, m_format(VK_FORMAT_UNDEFINED)
	, m_imageViewType(VkImageViewType::VK_IMAGE_VIEW_TYPE_2D)
	, m_flags(0)
	, m_view3DAttachment(VK_NULL_HANDLE)
{

}

/////////////////////////////////////////////////////////////////////////////////////////////

Texture::~Texture()
{
	// TODO: put in a destroy method
	if (m_mem != nullptr)
	{
		vkFreeMemory(coreM->getLogicalDevice(), m_mem, nullptr);
	}

	if (!m_isSwapChainTex)
	{
		vkDestroyImage(coreM->getLogicalDevice(), m_image, nullptr);
	}

	if (!m_isSwapChainTex)
	{
		int a = 0;
	}

	vkDestroyImageView(coreM->getLogicalDevice(), m_view, nullptr);
}

/////////////////////////////////////////////////////////////////////////////////////////////

bool Texture::getContentCopy(vectorUint8& vectorData)
{
	void* mappedMemory;
	VkResult result = vkMapMemory(coreM->getLogicalDevice(), m_mem, 0, uint(m_memorySize), 0, &mappedMemory);
	assert(result == VK_SUCCESS);
	vectorData.resize(m_memorySize);
	memcpy((void*)vectorData.data(), mappedMemory, m_memorySize);
	vkUnmapMemory(coreM->getLogicalDevice(), m_mem);
	return (result == VK_SUCCESS);
}

/////////////////////////////////////////////////////////////////////////////////////////////

bool Texture::isDepthTexture(VkFormat format)
{
	switch (format)
	{
		case VK_FORMAT_D16_UNORM:
		case VK_FORMAT_D32_SFLOAT:
		case VK_FORMAT_D16_UNORM_S8_UINT:
		case VK_FORMAT_D24_UNORM_S8_UINT:
		case VK_FORMAT_D32_SFLOAT_S8_UINT:
		{
			return true;
		}
	}

	return false;
}

/////////////////////////////////////////////////////////////////////////////////////////////

bool Texture::isDepthStencilTexture(VkFormat format)
{
	switch (format)
	{
		case VK_FORMAT_D16_UNORM_S8_UINT:
		case VK_FORMAT_D24_UNORM_S8_UINT:
		case VK_FORMAT_D32_SFLOAT_S8_UINT:
		{
			return true;
		}
	}

	return false;
}

/////////////////////////////////////////////////////////////////////////////////////////////
