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
#include "../../external/tinyddsloader/tinyddsloader.hpp"

// PROJECT INCLUDES
#include "../../include/texture/texturemanager.h"
#include "../../include/texture/texture.h"
#include "../../include/buffer/buffermanager.h"
#include "../../include/buffer/buffer.h"
#include "../../include/core/coremanager.h"
#include "../../include/core/physicaldevice.h"
#include "../../include/core/logicaldevice.h"
#include "../../include/parameter/attributedefines.h"
#include "../../include/texture/irradiancetexture.h"
#include "../../include/util/logutil.h"

// NAMESPACE
using namespace attributedefines;

// DEFINES

// STATIC MEMBER INITIALIZATION
VkCommandBuffer TextureManager::commandBufferTexture;

/////////////////////////////////////////////////////////////////////////////////////////////

TextureManager::TextureManager()
{
	m_managerName = g_textureManager;
}

/////////////////////////////////////////////////////////////////////////////////////////////

TextureManager::~TextureManager()
{
	destroyResources();
}

/////////////////////////////////////////////////////////////////////////////////////////////

void TextureManager::destroyResources()
{
	forIT(m_mapElement)
	{
		delete it->second;
		it->second = nullptr;
	}
}

/////////////////////////////////////////////////////////////////////////////////////////////

Texture* TextureManager::build2DTextureFromFile(
	string&&          instanceName,
	string&&          filename,
	VkImageUsageFlags imageUsageFlags,
	VkFormat          format)
{
	if (existsElement(move(string(instanceName))))
	{
		return getElement(move(instanceName));
	}

	Texture* texture = new Texture(move(string(instanceName)));

	gli::texture2D*    imageGLI2D        = nullptr;
	TextureInfo*       textureInfo       = nullptr;
	IrradianceTexture* irradianceTexture = nullptr;

	tinyddsloader::DDSFile ddsFile; // TODO: Refactor considering different implementations for RGBA8 and BC compressed formats

	if (isIrradianceTexture(move(string(filename))))
	{
		irradianceTexture = new IrradianceTexture(move(string(filename)));
		textureInfo       = new TextureInfo(irradianceTexture);
	}
	else if(format == VkFormat::VK_FORMAT_R8G8B8A8_UNORM)
	{
		imageGLI2D  = new gli::texture2D(gli::texture2D(gli::load(filename)));
		assert(!imageGLI2D->empty());
		textureInfo = new TextureInfo(imageGLI2D);
	}
	else if (format == VkFormat::VK_FORMAT_BC7_SRGB_BLOCK)
	{
		auto result = ddsFile.Load(filename.c_str());
		assert(result == tinyddsloader::Result::Success);
		textureInfo = new TextureInfo(&ddsFile);
	}
	else if (format == VkFormat::VK_FORMAT_BC5_UNORM_BLOCK)
	{
		auto result = ddsFile.Load(filename.c_str());
		assert(result == tinyddsloader::Result::Success);
		textureInfo = new TextureInfo(&ddsFile);
	}

	const vector<TextureMipMapInfo>& vectorMipMap = textureInfo->getVectorMipMap();
	assert(vectorMipMap.size() != 0);

	// Get the image dimensions
	texture->m_width           = vectorMipMap[0].m_width;
	texture->m_height          = vectorMipMap[0].m_height;
	texture->m_depth           = vectorMipMap[0].m_depth;
	texture->m_mipMapLevels    = uint32_t(vectorMipMap.size()); // Get number of mip-map levels
	texture->m_generateMipmap  = false;
	texture->m_path            = move(filename);
	texture->m_format          = format;
	texture->m_imageUsageFlags = imageUsageFlags;
	texture->m_imageViewType   = VkImageViewType::VK_IMAGE_VIEW_TYPE_2D;
	texture->m_flags           = 0;

	// Create image info with optimal tiling support (VK_IMAGE_TILING_OPTIMAL)
	VkExtent3D imageExtent = { texture->m_width, texture->m_height, texture->m_depth };
	texture->m_image       = buildImage(format,
										imageExtent,
										texture->m_mipMapLevels,
										imageUsageFlags | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
										VK_SAMPLE_COUNT_1_BIT,
										VK_IMAGE_TILING_OPTIMAL,
										texture->m_imageViewType,
										texture->m_flags);

	coreM->setObjectName(uint64_t(texture->m_image), VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_EXT, texture->getName());

	texture->m_mem = buildImageMemory(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, texture->m_image, texture->m_memorySize);


	vector<VkExtent3D> vectorMipMapExtent;
	vector<uint> vectorMipMapSize;

	for (uint32_t i = 0; i < texture->m_mipMapLevels; ++i)
	{
		VkExtent3D mipExtent = { vectorMipMap[i].m_width, vectorMipMap[i].m_height, vectorMipMap[i].m_depth };
		vectorMipMapExtent.push_back(mipExtent);
		vectorMipMapSize.push_back(uint(vectorMipMap[i].m_size));
	}

	texture->m_imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	fillImageMemoryMipmaps(texture,
						   textureInfo->refData(),
						   uint(textureInfo->getSize()),
						   vectorMipMapExtent,
						   vectorMipMapSize);

	VkImageFormatProperties imageProperties{};
	VkComponentMapping components;

	if (format == VkFormat::VK_FORMAT_BC7_SRGB_BLOCK)
	{
		VkResult result = vkGetPhysicalDeviceImageFormatProperties(coreM->getPhysicalDevice(), VK_FORMAT_BC7_SRGB_BLOCK, VK_IMAGE_TYPE_2D, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_SAMPLED_BIT, 0, &imageProperties);
		components      = { VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_A };
	}
	else if (format == VkFormat::VK_FORMAT_BC5_UNORM_BLOCK)
	{
		VkResult result = vkGetPhysicalDeviceImageFormatProperties(coreM->getPhysicalDevice(), VK_FORMAT_BC5_UNORM_BLOCK, VK_IMAGE_TYPE_2D, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_SAMPLED_BIT, 0, &imageProperties);
		components      = { VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G };
	}

	texture->m_view = buildImageView(VK_IMAGE_ASPECT_COLOR_BIT, texture->m_image, components, texture->getMipMapLevels(), format, VK_IMAGE_VIEW_TYPE_2D);

	coreM->setObjectName(uint64_t(texture->m_image), VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_VIEW_EXT, texture->getName());

	TextureManager::addElement(move(string(instanceName)), texture);
	texture->m_name = move(instanceName);

	if (imageGLI2D != nullptr)
	{
		delete imageGLI2D;
	}

	if (textureInfo != nullptr)
	{
		delete textureInfo;
	}

	if (irradianceTexture != nullptr)
	{
		delete irradianceTexture;
	}

	// PENDING: custom components
	
	return texture;
}

/////////////////////////////////////////////////////////////////////////////////////////////

Texture* TextureManager::buildTextureFromData(
	string&&                  instanceName,
	VkImageUsageFlags         imageUsageFlags,
	VkFormat                  format,
	void*                     imageData,
	uint                      dataSize,
	const vector<VkExtent3D>& vectorMipMapExtent,
	const vector<uint>&       vectorMipMapSize,
	VkSampleCountFlagBits     samples,
	VkImageTiling             tiling,
	VkImageViewType           imageViewType)
{
	if (existsElement(move(string(instanceName))))
	{
		return getElement(move(instanceName));
	}

	Texture* texture = new Texture(move(string(instanceName)));
	
	// Get the image dimensions, vectorMipMapExtent[0] is assumed to contain mip map 0
	texture->m_width           = uint32_t(vectorMipMapExtent[0].width);
	texture->m_height          = uint32_t(vectorMipMapExtent[0].height);
	texture->m_depth           = uint32_t(vectorMipMapExtent[0].depth);
	texture->m_mipMapLevels    = uint32_t(vectorMipMapExtent.size());
	texture->m_generateMipmap  = false;
	texture->m_format          = format;
	texture->m_imageUsageFlags = imageUsageFlags;
	texture->m_imageViewType   = imageViewType;
	texture->m_flags           = 0;

	// Create image info with optimal tiling support (VK_IMAGE_TILING_OPTIMAL)
	texture->m_image = buildImage(format,
								  { texture->m_width, texture->m_height, texture->m_depth },
								  texture->m_mipMapLevels,
								  imageUsageFlags | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
								  samples,
								  tiling,
								  texture->m_imageViewType,
								  texture->m_flags);

	coreM->setObjectName(uint64_t(texture->m_image), VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_EXT, texture->getName());

	texture->m_mem         = buildImageMemory(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, texture->m_image, texture->m_memorySize);
	texture->m_imageLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;

	fillImageMemoryMipmaps(texture,
						   imageData,
						   dataSize,
						   vectorMipMapExtent,
						   vectorMipMapSize);
	
	VkComponentMapping components = { VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_A };

	texture->m_view = buildImageView(VK_IMAGE_ASPECT_COLOR_BIT, texture->m_image, components, 1, format, texture->m_imageViewType);

	coreM->setObjectName(uint64_t(texture->m_image), VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_VIEW_EXT, texture->getName());

	TextureManager::addElement(move(string(instanceName)), texture);
	texture->m_name = move(instanceName);

	return texture;
}

/////////////////////////////////////////////////////////////////////////////////////////////

Texture* TextureManager::buildTexture(
	string&&              instanceName,
	VkFormat              format,
	VkExtent3D            extent,
	VkImageUsageFlags     usage,
	VkImageAspectFlags    aspectMask,
	VkImageAspectFlags    layoutAspectMask,
	VkImageLayout         oldImageLayout,
	VkImageLayout         newImageLayout,
	VkMemoryPropertyFlags properties,
	VkSampleCountFlagBits samples,
	VkImageTiling         tiling,
	VkImageViewType       imageViewType,
	VkImageCreateFlags    flags)
{
	if (existsElement(move(string(instanceName))))
	{
		return getElement(move(instanceName));
	}

	Texture* texture = new Texture(move(string(instanceName)));

	texture->m_width           = extent.width;
	texture->m_height          = extent.height;
	texture->m_depth           = extent.depth;
	texture->m_mipMapLevels    = 1;
	texture->m_generateMipmap  = false;
	texture->m_imageUsageFlags = usage;
	texture->m_imageViewType   = imageViewType;
	texture->m_flags           = flags;

	texture->m_image           = buildImage(format, extent, 1, usage, samples, tiling, texture->m_imageViewType, texture->m_flags);
	texture->m_mem             = buildImageMemory(properties, texture->m_image, texture->m_memorySize);

	coreM->setObjectName(uint64_t(texture->m_image), VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_EXT, texture->getName());

	bool imageAs2DArrayForFramebufferClearing = (usage & VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT) && (imageViewType == VK_IMAGE_VIEW_TYPE_3D);

	if (imageAs2DArrayForFramebufferClearing)
	{
		if ((extent.width != extent.height) || (extent.width != extent.depth))
		{
			cout << "ERROR: TRYING TO BUILD A 3D TEXTURE IN TextureManager::buildTexture WITH DIMENSIONS NOT THE SAME FOR WIDTH / HEIGHT / DEPTH" << endl;
		}
		else
		{
			texture->m_view3DAttachment = buildImageView(aspectMask, texture->m_image, { VK_COMPONENT_SWIZZLE_IDENTITY }, 1, format, texture->m_imageViewType, extent.width, imageAs2DArrayForFramebufferClearing);

			coreM->setObjectName(uint64_t(texture->m_view3DAttachment), VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_VIEW_EXT, texture->getName());
		}
	}

	// Use command buffer to create the depth image. This includes -
	// Command buffer allocation, recording with begin/end scope and submission.
	coreM->allocCommandBuffer(&coreM->getLogicalDevice(), coreM->getGraphicsCommandPool(), &commandBufferTexture);
	coreM->beginCommandBuffer(commandBufferTexture);
	{
		VkImageSubresourceRange subresourceRange = {};
		subresourceRange.aspectMask   = layoutAspectMask;
		subresourceRange.baseMipLevel = 0;
		subresourceRange.levelCount   = 1;
		subresourceRange.layerCount   = (texture->m_imageViewType == VK_IMAGE_VIEW_TYPE_CUBE) ? 6 : 1;

		// Set the image layout (to optimal)
		setImageLayout(commandBufferTexture, texture, aspectMask, oldImageLayout, newImageLayout, subresourceRange, coreM->getGraphicsQueueIndex());
	}
	coreM->endCommandBuffer(commandBufferTexture);
	coreM->submitCommandBuffer(coreM->getLogicalDeviceGraphicsQueue(), &commandBufferTexture);

	// TODO: put in separate static method
	texture->m_view = buildImageView(aspectMask, texture->m_image, { VK_COMPONENT_SWIZZLE_IDENTITY }, 1, format, texture->m_imageViewType, 1, false);

	coreM->setObjectName(uint64_t(texture->m_image), VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_VIEW_EXT, texture->getName());

	TextureManager::addElement(move(string(instanceName)), texture);
	texture->m_name        = move(instanceName);
	texture->m_format      = format;
	texture->m_imageLayout = newImageLayout;

	return texture;
}

/////////////////////////////////////////////////////////////////////////////////////////////

void TextureManager::destroyCommandBuffer()
{
	VkCommandBuffer cmdBufs[] = { commandBufferTexture };
	vkFreeCommandBuffers(coreM->getLogicalDevice(), coreM->getGraphicsCommandPool(), sizeof(cmdBufs) / sizeof(VkCommandBuffer), cmdBufs);
}

/////////////////////////////////////////////////////////////////////////////////////////////

Texture* TextureManager::buildTextureFromExistingResources(
	string&&        instanceName,
	VkImage         image,
	VkImageView     view,
	uint32_t        width,
	uint32_t        height,
	VkFormat        format,
	VkImageViewType imageViewType)
{
	if (existsElement(move(string(instanceName))))
	{
		return getElement(move(instanceName));
	}

	Texture* texture = new Texture(move(string(instanceName)));

	texture->m_image           = image;
	texture->m_imageLayout     = VK_IMAGE_LAYOUT_MAX_ENUM;
	texture->m_mem             = VK_NULL_HANDLE;
	texture->m_view            = view;
	texture->m_mipMapLevels    = 0;
	texture->m_layerCount      = 1;
	texture->m_width           = width;
	texture->m_height          = height;
	texture->m_depth           = 1;
	texture->m_generateMipmap  = false;
	texture->m_imageUsageFlags = VK_IMAGE_USAGE_FLAG_BITS_MAX_ENUM;
	texture->m_format          = format;
	texture->m_imageViewType   = imageViewType;
	texture->m_flags           = 0;

	TextureManager::addElement(move(string(instanceName)), texture);
	texture->m_name = move(instanceName);

	return texture;
}

/////////////////////////////////////////////////////////////////////////////////////////////

VkImageView TextureManager::buildImageView(VkImageAspectFlags aspectMask, VkImage image, VkComponentMapping components, uint32_t levelCount, VkFormat format, VkImageViewType viewType)
{
	return buildImageView(aspectMask, image, components, levelCount, format, viewType, 1, false);
}

/////////////////////////////////////////////////////////////////////////////////////////////

VkImageView TextureManager::buildImageView(VkImageAspectFlags aspectMask, VkImage image, VkComponentMapping components, uint32_t levelCount, VkFormat format, VkImageViewType viewType, int layerCount, bool imageAs2DArrayForFramebufferClearing)
{
	VkImageViewCreateInfo imgViewInfo = {};
	imgViewInfo.sType                           = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	imgViewInfo.pNext                           = NULL;
	imgViewInfo.image                           = image;
	imgViewInfo.format                          = format;
	imgViewInfo.components                      = components; //{ VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_A }
	imgViewInfo.subresourceRange.aspectMask     = aspectMask;
	imgViewInfo.subresourceRange.baseMipLevel   = 0;
	imgViewInfo.subresourceRange.levelCount     = levelCount;
	imgViewInfo.subresourceRange.baseArrayLayer = 0;
	imgViewInfo.subresourceRange.layerCount     = (viewType == VK_IMAGE_VIEW_TYPE_CUBE) ? 6 : layerCount;
	imgViewInfo.viewType                        = ((viewType == VK_IMAGE_VIEW_TYPE_3D) && imageAs2DArrayForFramebufferClearing) ? VK_IMAGE_VIEW_TYPE_2D_ARRAY : viewType;
	imgViewInfo.flags                           = 0;

	VkImageView imageView;
	VkResult result = vkCreateImageView(coreM->getLogicalDevice(), &imgViewInfo, nullptr, &imageView);
	assert(result == VK_SUCCESS);

	return imageView;
}

/////////////////////////////////////////////////////////////////////////////////////////////

VkSampler TextureManager::buildSampler(float minLod, float maxLod, VkSamplerMipmapMode mipmapMode, VkFilter minMagFilter)
{
	// Create sampler
	VkSamplerCreateInfo samplerCI = {};
	samplerCI.sType                   = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	samplerCI.pNext                   = NULL;
	samplerCI.magFilter               = minMagFilter;
	samplerCI.minFilter               = minMagFilter;
	samplerCI.mipmapMode              = mipmapMode;
	samplerCI.addressModeU            = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplerCI.addressModeV            = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplerCI.addressModeW            = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplerCI.mipLodBias              = 0.0f;
	samplerCI.anisotropyEnable        = VK_FALSE;
	samplerCI.maxAnisotropy           = 1; // could not be neccessary
	samplerCI.compareEnable           = VK_FALSE; // Implemented because of the voxelization step
	samplerCI.compareOp               = VK_COMPARE_OP_ALWAYS;  // Implemented because of the voxelization step, old value was VK_COMPARE_OP_NEVER
	samplerCI.minLod                  = minLod;
	samplerCI.maxLod                  = maxLod;
	samplerCI.borderColor             = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
	samplerCI.unnormalizedCoordinates = VK_FALSE;

	VkSampler sampler;

	VkResult error = vkCreateSampler(coreM->getLogicalDevice(), &samplerCI, nullptr, &sampler);
	assert(!error);

	return sampler;
}

/////////////////////////////////////////////////////////////////////////////////////////////

void TextureManager::setImageLayout(VkCommandBuffer commandBuffer, Texture* texture, VkImageAspectFlags aspectMask, VkImageLayout oldImageLayout, VkImageLayout newImageLayout, const VkImageSubresourceRange& subresourceRange, uint queueIndex)
{
	// Dependency on cmd
	assert(commandBuffer != VK_NULL_HANDLE);

	// The deviceObj->queue must be initialized
	assert(coreM->getLogicalDeviceGraphicsQueue() != VK_NULL_HANDLE);

	VkImageMemoryBarrier imgMemoryBarrier = {};
	imgMemoryBarrier.sType                = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	imgMemoryBarrier.pNext                = NULL;
	imgMemoryBarrier.srcAccessMask        = 0;
	imgMemoryBarrier.dstAccessMask        = 0;
	imgMemoryBarrier.oldLayout            = oldImageLayout;
	imgMemoryBarrier.newLayout            = newImageLayout;
	imgMemoryBarrier.srcQueueFamilyIndex  = queueIndex;
	imgMemoryBarrier.dstQueueFamilyIndex  = queueIndex;
	imgMemoryBarrier.image                = texture->getImage();
	imgMemoryBarrier.subresourceRange     = subresourceRange;

	if (oldImageLayout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL)
	{
		imgMemoryBarrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	}

	// Source layouts (old)
	switch (oldImageLayout)
	{
		case VK_IMAGE_LAYOUT_UNDEFINED:
		{
			imgMemoryBarrier.srcAccessMask = 0;
			break;
		}
		case VK_IMAGE_LAYOUT_PREINITIALIZED:
		{
			imgMemoryBarrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT;
			break;
		}
		case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
		{
			imgMemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			break;
		}
		case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
		{
			imgMemoryBarrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
			break;
		}
		case VK_IMAGE_LAYOUT_GENERAL:
		{
			imgMemoryBarrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;
			break;
		}
	}

	switch (newImageLayout)
	{
		case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
		case VK_IMAGE_LAYOUT_PRESENT_SRC_KHR:
		{
			// Ensure that anything that was copying from this image has completed
			// An image in this layout can only be used as the destination operand of the commands

			imgMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			break;
		}
		case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
		{
			// Ensure any Copy or CPU writes to image are flushed
			// An image in this layout can only be used as a read-only shader resource

			imgMemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			imgMemoryBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
			break;
		}
		case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
		{
			// An image in this layout can only be used as a framebuffer color attachment

			imgMemoryBarrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
			break;
		}
		case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
		{
			// An image in this layout can only be used as a framebuffer depth/stencil attachment

			imgMemoryBarrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
			break;
		}
		case VK_IMAGE_LAYOUT_GENERAL:
		{
			imgMemoryBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;
			break;
		}
	}

	// TODO: avoid using VK_PIPELINE_STAGE_ALL_COMMANDS_BIT and particularize to the proper type of barrier
	VkPipelineStageFlags srcStages = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
	VkPipelineStageFlags destStages = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;

	vkCmdPipelineBarrier(commandBuffer, srcStages, destStages, 0, 0, NULL, 0, NULL, 1, &imgMemoryBarrier);

	texture->m_imageLayout = newImageLayout;
}

/////////////////////////////////////////////////////////////////////////////////////////////

void TextureManager::changeTextureLayout(vectorTexturePtr vectorTextureLayoutChange, vectorImageLayout vectorTextureLayoutChangeOriginalLayout, vectorImageLayout vectorTextureLayoutChangeFinalLayout, bool useComputeQueue)
{
	if (vectorTextureLayoutChange.size() != vectorTextureLayoutChangeOriginalLayout.size())
	{
		cout << "ERROR: WRONG VECTOR SIZE IN TextureManager::changeTextureLayout" << endl;
		return;
	}

	VkCommandBuffer commandBuffer;
	coreM->allocCommandBuffer(&coreM->getLogicalDevice(), useComputeQueue ? coreM->getComputeCommandPool() : coreM->getGraphicsCommandPool(), &commandBuffer);
	coreM->beginCommandBuffer(commandBuffer);

	VkImageSubresourceRange subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };

	forI(vectorTextureLayoutChange.size())
	{
		subresourceRange.levelCount = vectorTextureLayoutChange[i]->getMipMapLevels();
		textureM->setImageLayout(commandBuffer, vectorTextureLayoutChange[i], VK_IMAGE_ASPECT_COLOR_BIT, vectorTextureLayoutChangeOriginalLayout[i], vectorTextureLayoutChangeFinalLayout[i], subresourceRange, useComputeQueue ? coreM->getComputeQueueIndex() : coreM->getGraphicsQueueIndex());
	}

	coreM->endCommandBuffer(commandBuffer);
	coreM->submitCommandBuffer(useComputeQueue ? coreM->getLogicalDeviceComputeQueue() : coreM->getLogicalDeviceGraphicsQueue(), &commandBuffer);
	VkCommandBuffer cmdBufs[] = { commandBuffer };
	vkFreeCommandBuffers(coreM->getLogicalDevice(), useComputeQueue ? coreM->getComputeCommandPool() : coreM->getGraphicsCommandPool(), sizeof(cmdBufs) / sizeof(VkCommandBuffer), cmdBufs);
}

/////////////////////////////////////////////////////////////////////////////////////////////

void TextureManager::printTextureResourceInformation()
{
	vectorString bufferName = {
		"dynamicVoxelizationReflectanceTexture",
		"irradiance3DDynamicNegativeX",
		"irradiance3DDynamicNegativeY",
		"irradiance3DDynamicNegativeZ",
		"irradiance3DDynamicPositiveX",
		"irradiance3DDynamicPositiveY",
		"irradiance3DDynamicPositiveZ",
		"irradiance3DStaticNegativeX",
		"irradiance3DStaticNegativeY",
		"irradiance3DStaticNegativeZ",
		"irradiance3DStaticPositiveX",
		"irradiance3DStaticPositiveY",
		"irradiance3DStaticPositiveZ",
		"staticVoxelIndexTexture",
		"staticVoxelNeighbourInfo",
		"voxelizationReflectance",
	};

	mapStringInt mapData;
	forIT(bufferName)
	{
		auto itResult = m_mapElement.find(*it);
		mapData.insert(pair<string, int>(itResult->first, itResult->second->m_memorySize));
	}

	LogUtil::printInformationTabulated(mapData);
}

/////////////////////////////////////////////////////////////////////////////////////////////

VkImage TextureManager::buildImage(
	VkFormat              format,
	VkExtent3D            extent,
	uint32_t              mipLevels,
	VkImageUsageFlags     usage,
	VkSampleCountFlagBits samples,
	VkImageTiling         tiling,
	VkImageViewType       imageViewType,
	VkImageCreateFlags    flags)
{
	// TODO: put in separate static method

	// NOTE: If flags equals to VK_IMAGE_CREATE_2D_ARRAY_COMPATIBLE_BIT and the image view type imageViewType equals to 
	// VK_IMAGE_VIEW_TYPE_3D then this 3D texture will be used as an attachment and for that reason the amount of layers 
	// (field arrayLayers) has to match the dimensions of the texture which will be interpeted as an array of 2D textures

	VkImageCreateInfo imageInfo = {};
	imageInfo.sType                 = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	imageInfo.pNext                 = NULL;
	imageInfo.imageType             = getImageType(imageViewType);
	imageInfo.format                = format;
	imageInfo.extent                = extent;
	imageInfo.mipLevels             = mipLevels;
	imageInfo.arrayLayers           = (imageViewType == VkImageViewType::VK_IMAGE_VIEW_TYPE_CUBE) ? 6 : 1;
	imageInfo.samples               = samples;
	imageInfo.tiling                = tiling; /*VK_IMAGE_TILING_OPTIMAL*/;
	imageInfo.queueFamilyIndexCount = 0;
	imageInfo.pQueueFamilyIndices   = NULL;
	imageInfo.sharingMode           = VK_SHARING_MODE_EXCLUSIVE;
	imageInfo.usage                 = usage;
	imageInfo.flags                 = flags;
	imageInfo.initialLayout         = VK_IMAGE_LAYOUT_UNDEFINED;

	VkImage image;
	VkResult result = vkCreateImage(coreM->getLogicalDevice(), &imageInfo, nullptr, &image);
	assert(result == VK_SUCCESS);

	return image;
}

/////////////////////////////////////////////////////////////////////////////////////////////

VkDeviceMemory TextureManager::buildImageMemory(VkFlags requirementsMask, VkImage& image, VkDeviceSize& memorySize)
{
	VkResult result;

	VkMemoryRequirements memRqrmnt;
	vkGetImageMemoryRequirements(coreM->getLogicalDevice(), image, &memRqrmnt);

	VkMemoryAllocateInfo memAlloc = {};
	memAlloc.sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	memAlloc.pNext           = NULL;
	memAlloc.memoryTypeIndex = 0;
	memAlloc.allocationSize  = memRqrmnt.size;
	memorySize               = memRqrmnt.size;

	VkDeviceMemory memory;
	// Determine the type of memory required with the help of memory properties
	bool propertiesResult = coreM->memoryTypeFromProperties(memRqrmnt.memoryTypeBits, requirementsMask, coreM->getPhysicalDeviceMemoryProperties().memoryTypes, memAlloc.memoryTypeIndex);

	assert(propertiesResult);

	// Allocate the memory for image objects
	result = vkAllocateMemory(coreM->getLogicalDevice(), &memAlloc, nullptr, &memory);
	assert(result == VK_SUCCESS);

	// Bind the allocated memeory
	result = vkBindImageMemory(coreM->getLogicalDevice(), image, memory, 0);
	assert(result == VK_SUCCESS);

	return memory;
}

/////////////////////////////////////////////////////////////////////////////////////////////

void TextureManager::fillImageMemoryMipmaps(Texture*                  texture,
											void*                     imageData,
											uint                      dataSize,
											const vector<VkExtent3D>& vectorMipMapExtent,
											const vector<uint>&       vectorMipMapSize)
{
	Buffer* buffer = bufferM->buildBuffer(move(string("fillBuffer")), imageData, dataSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

	VkImageSubresourceRange subresourceRange = {};
	subresourceRange.aspectMask   = VK_IMAGE_ASPECT_COLOR_BIT;
	subresourceRange.baseMipLevel = 0;
	subresourceRange.levelCount   = texture->getMipMapLevels();
	subresourceRange.layerCount   = 1;

	// Use a separate command buffer for texture loading, start command buffer recording
	coreM->allocCommandBuffer(&coreM->getLogicalDevice(), coreM->getGraphicsCommandPool(), &commandBufferTexture);
	coreM->beginCommandBuffer(commandBufferTexture);

	// set the image layout to be VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL since it is destination for copying buffer into image using vkCmdCopyBufferToImage -
	setImageLayout(commandBufferTexture, texture, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, subresourceRange, coreM->getGraphicsQueueIndex());

	uint32_t bufferOffset = 0;
	vector<VkBufferImageCopy> bufferImgCopyList; // List contains the buffer image copy for each mipLevel -
													  // Iterater through each mip level and set buffer image copy -
	for (uint32_t i = 0; i < texture->getMipMapLevels(); i++)
	{
		VkBufferImageCopy bufImgCopyItem = {};
		bufImgCopyItem.imageSubresource.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
		bufImgCopyItem.imageSubresource.mipLevel       = i;
		bufImgCopyItem.imageSubresource.layerCount     = 1;
		bufImgCopyItem.imageSubresource.baseArrayLayer = 0;
		bufImgCopyItem.imageExtent                     = vectorMipMapExtent[i];
		bufImgCopyItem.bufferOffset                    = bufferOffset;
										
		bufferImgCopyList.push_back(bufImgCopyItem);
		bufferOffset += uint32_t(vectorMipMapSize[i]); // adjust buffer offset
	}

	// Copy the staging buffer memory data contain the stage raw data(with mip levels) into image object
	vkCmdCopyBufferToImage(commandBufferTexture, buffer->getBuffer(), texture->getImage(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, uint32_t(bufferImgCopyList.size()), bufferImgCopyList.data());

	// Advised to change the image layout to shader read after staged buffer copied into image memory -
	setImageLayout(commandBufferTexture, texture, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, subresourceRange, coreM->getGraphicsQueueIndex());

	coreM->endCommandBuffer(commandBufferTexture); // Submit command buffer containing copy and image layout commands-

															  // Create a fence object to ensure that the command buffer is executed, coping our staged raw data from the buffers to image memory with respective image layout and attributes into consideration -
	VkFence fence;
	VkFenceCreateInfo fenceCI = {};
	fenceCI.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	fenceCI.flags = 0;

	VkResult error = vkCreateFence(coreM->getLogicalDevice(), &fenceCI, nullptr, &fence);
	assert(!error);

	VkSubmitInfo submitInfo = {};
	submitInfo.sType              = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.pNext              = NULL;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers    = &commandBufferTexture;

	coreM->submitCommandBuffer(coreM->getLogicalDeviceGraphicsQueue(), &commandBufferTexture, &submitInfo, fence);

	error = vkWaitForFences(coreM->getLogicalDevice(), 1, &fence, VK_TRUE, 10000000000);
	assert(!error);

	// Destroy resources used for this operation
	vkDestroyFence(coreM->getLogicalDevice(), fence, nullptr);
	bufferM->removeElement(move(string("fillBuffer")));
}

/////////////////////////////////////////////////////////////////////////////////////////////

bool TextureManager::isIrradianceTexture(string&& path)
{
	if (path.size() < 5)
	{
		return false;
	}

	string extension = path.substr(path.size() - 5, 5);

	if (strcmp(extension.c_str(), string(".irrt").c_str()))
	{
		return false;
	}

	return true;
}

/////////////////////////////////////////////////////////////////////////////////////////////

VkImageType TextureManager::getImageType(VkImageViewType imageViewType)
{
	switch (imageViewType)
	{
		case VkImageViewType::VK_IMAGE_VIEW_TYPE_1D:
		{
			return VkImageType::VK_IMAGE_TYPE_1D;
		}
		case VkImageViewType::VK_IMAGE_VIEW_TYPE_2D:
		{
			return VkImageType::VK_IMAGE_TYPE_2D;
		}
		case VkImageViewType::VK_IMAGE_VIEW_TYPE_3D:
		{
			return VkImageType::VK_IMAGE_TYPE_3D;
		}
		case VkImageViewType::VK_IMAGE_VIEW_TYPE_CUBE:
		{
			return VkImageType::VK_IMAGE_TYPE_2D;
		}
		default:
		{
			cout << "ERROR: no image view type case available" << endl;
			assert(1);
		}
	}

	return VkImageType::VK_IMAGE_TYPE_2D;
}

/////////////////////////////////////////////////////////////////////////////////////////////
