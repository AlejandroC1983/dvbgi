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
#include "../../include/shader/sampler.h"
#include "../../include/texture/texture.h"
#include "../../include/texture/texturemanager.h"
#include "../../include/core/coremanager.h"

// NAMESPACE

// DEFINES

// STATIC MEMBER INITIALIZATION

/////////////////////////////////////////////////////////////////////////////////////////////

Sampler::Sampler(string &&name, const ResourceInternalType &samplerType, VkShaderStageFlagBits shaderStage, int bindingIndex, int setIndex, VkFilter filter) : GenericResource(move(name), move(string("Sampler")), GenericResourceType::GRT_SAMPLER)
	, m_samplerHandle(VK_NULL_HANDLE)
	, m_samplerType(samplerType)
	, m_shaderStage(shaderStage)
	, m_hasAssignedTexture(false)
	, m_bindingIndex(bindingIndex)
	, m_setIndex(setIndex)
	, m_isImageSampler(false)
	, m_samplerFormat(ResourceInternalType::RIT_SIZE)
	, m_descriptorType(VK_DESCRIPTOR_TYPE_MAX_ENUM)
	, m_mipmapMode(filter == VkFilter::VK_FILTER_LINEAR ? VK_SAMPLER_MIPMAP_MODE_LINEAR : VK_SAMPLER_MIPMAP_MODE_NEAREST)
	, m_minMagFilter(filter)
{

}

/////////////////////////////////////////////////////////////////////////////////////////////

Sampler::Sampler(string &&name, const ResourceInternalType &samplerType, VkShaderStageFlagBits shaderStage, int bindingIndex, int setIndex, ResourceInternalType samplerFormat, VkFilter filter) : GenericResource(move(name), move(string("Sampler")), GenericResourceType::GRT_SAMPLER)
	, m_samplerHandle(VK_NULL_HANDLE)
	, m_samplerType(samplerType)
	, m_shaderStage(shaderStage)
	, m_hasAssignedTexture(false)
	, m_bindingIndex(bindingIndex)
	, m_setIndex(setIndex)
	, m_isImageSampler(true)
	, m_samplerFormat(samplerFormat)
	, m_descriptorType(VK_DESCRIPTOR_TYPE_MAX_ENUM)
	, m_mipmapMode(filter == VkFilter::VK_FILTER_LINEAR ? VK_SAMPLER_MIPMAP_MODE_LINEAR : VK_SAMPLER_MIPMAP_MODE_NEAREST)
	, m_minMagFilter(filter)
{

}

/////////////////////////////////////////////////////////////////////////////////////////////

bool Sampler::setTextureToSample(Texture *texture)
{
	if (texture == nullptr)
	{
		return false;
	}

	m_vectorTexture.resize(1);
	m_vectorTextureToSampleName.resize(1);

	m_hasAssignedTexture           = true;
	m_vectorTexture[0]             = texture;
	m_vectorTextureToSampleName[0] = texture->getName();

	return true;
}

/////////////////////////////////////////////////////////////////////////////////////////////

bool Sampler::setTextureToSample(string &&name)
{
	m_hasAssignedTexture = setTextureToSample(textureM->getElement(move(name)));

	return m_hasAssignedTexture;
}

/////////////////////////////////////////////////////////////////////////////////////////////

void Sampler::buildDescriptorImageInfoVector()
{
	const uint maxIndex = m_vectorTexture.size();
	m_vectorDescriptorImageInfo.resize(maxIndex);

	VkDescriptorImageInfo temp;

	forI(maxIndex)
	{
		m_vectorDescriptorImageInfo[i].sampler     = m_samplerHandle;
		m_vectorDescriptorImageInfo[i].imageView   = m_vectorTexture[i]->getView();
		m_vectorDescriptorImageInfo[i].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	}
}

/////////////////////////////////////////////////////////////////////////////////////////////

Sampler::~Sampler()
{
	if (m_samplerHandle != VK_NULL_HANDLE)
	{
		vkDestroySampler(coreM->getLogicalDevice(), m_samplerHandle, NULL);
	}
}

/////////////////////////////////////////////////////////////////////////////////////////////
