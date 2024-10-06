/*
Copyright 2016 Alejandro Cosin

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

#ifndef _SAMPLER_H_
#define _SAMPLER_H_

// GLOBAL INCLUDES

// PROJECT INCLUDES
#include "../../include/util/genericresource.h"
#include "../../include/shader/resourceenum.h"

// CLASS FORWARDING
class Texture;

// NAMESPACE
using namespace commonnamespace;
using namespace resourceenum;

// DEFINES

/////////////////////////////////////////////////////////////////////////////////////////////

class Sampler : public GenericResource
{
public:
	/** Parameter constructor for texture samplers
	* @param name         [in] texture sampler name
	* @param samplerType  [in] texture sampler type
	* @param shaderStage  [in] enumerated with value the shader stage the uniform being built is used
	* @param bindingIndex [in] binding index of this uniform buffer for the shader it's owned by
	* @param setIndex     [in] set index inside the binding index given by bindingIndex of this uniform buffer for the shader it's owned by
	* @param filter       [in] fitering mode
	* @return nothing */
	Sampler(string &&name, const ResourceInternalType &samplerType, VkShaderStageFlagBits shaderStage, int bindingIndex, int setIndex, VkFilter filter);

	/** Parameter constructor for image samplers
	* @param name         [in] image sampler name
	* @param samplerType  [in] image sampler type
	* @param shaderStage  [in] enumerated with value the shader stage the uniform being built is used
	* @param bindingIndex [in] binding index of this uniform buffer for the shader it's owned by
	* @param setIndex     [in] set index inside the binding index given by bindingIndex of this uniform buffer for the shader it's owned by
	* @param format       [in] format used in the shader for the image this sampler represents
	* @param filter       [in] fitering mode
	* @return nothing */
	Sampler(string &&name, const ResourceInternalType &samplerType, VkShaderStageFlagBits shaderStage, int bindingIndex, int setIndex, ResourceInternalType samplerFormat, VkFilter filter);

	/** Sets the texture given as parameter as the one to be bound with this sampler, if the texture type and sampler type are compatible
	* @param texture [in] texture to set to this sampler
	* @return true if the texture was successfully set as the one used by this sampler, and false otherwise */
	bool setTextureToSample(Texture *texture);

	/** Sets the texture given as parameter as the one to be bound with this sampler, if the texture type and sampler type are compatible, if
	* the texture manager has a texture with this name available. If it's not the case, the m_hasAssignedTexture flag will be used
	* before rasterization for try to find a texture with name given by m_textureToSampleName and set it to m_texture, setting the
	* m_hasAssignedTexture flag as true
	* @param name [in] name of the texture to use for sampling (will be requested to the texture manager when calling the method, and, if not found, again before rasterization)
	* @return true if the texture was found and assigned to m_texture successfully (sampler and texture may not be compatible), and false otherwise */
	bool setTextureToSample(string &&name);

	/** Helper function to add to m_vectorDescriptorImageInfo the descriptor image info for each image present in m_vectorTexturePtr
	* @return nothing */
	void buildDescriptorImageInfoVector();

	/** Default destructor
	*@return nothing */
	~Sampler();
	
	GETCOPY_SET(VkSampler, m_samplerHandle, SamplerHandle)
	GET(ResourceInternalType, m_samplerType, SamplerType)
	GET(VkShaderStageFlagBits, m_shaderStage, ShaderStage)
	GETCOPY(int, m_bindingIndex, BindingIndex)
	GETCOPY(int, m_setIndex, SetIndex)
	GET(bool, m_hasAssignedTexture, HasAssignedTexture)
	GET(vectorTexturePtr, m_vectorTexture, VectorTexture)
	REF(vectorTexturePtr, m_vectorTexture, VectorTexture)
	GET(vectorString, m_vectorTextureToSampleName, VectorTextureToSampleName)
	SETMOVE(vectorString, m_vectorTextureToSampleName, VectorTextureToSampleName)
	GET(bool, m_isImageSampler, IsImageSampler)
	GETCOPY_SET(VkDescriptorType, m_descriptorType, DescriptorType)
	GETCOPY_SET(VkSamplerMipmapMode, m_mipmapMode, MipmapMode)
	GETCOPY_SET(VkFilter, m_minMagFilter, MinMagFilter)
	REF_SET(vectorTexturePtr, m_vectorTexture, VectorTexturePtr)
	REF(vectorDescriptorImageInfo, m_vectorDescriptorImageInfo, VectorDescriptorImageInfo)

protected:
	VkSampler                 m_samplerHandle;             //!< Handle to the sampler
	ResourceInternalType      m_samplerType;               //!< sampler type
	VkShaderStageFlagBits     m_shaderStage;               //!< enumerated with value the shader stage the uniform being built is used
	bool                      m_hasAssignedTexture;        //!< If false, this sampler still doesn't have a proper value for the m_texture to sample from
	int                       m_bindingIndex;              //!< Binding index for this uniform buffer in the shader its owned by
	int                       m_setIndex;                  //!< Set index inside m_bindingIndex for this uniform buffer in the shader its owned by
	vectorTexturePtr          m_vectorTexture;             //!< Vector with the textures bound to this sampler, always one unless the sampler is of type combined image sampler
	vectorString              m_vectorTextureToSampleName; //!< Vector with the names of the textures to sample (for cases when the texture is built after the initialization call of the shader containing this sampler, will be taken into account before rasterization), always one unless the sampler is of type combined image sampler
	bool                      m_isImageSampler;            //!< True if this is an image sampler, false if it's a texture sampler
	ResourceInternalType      m_samplerFormat;             //!< If m_isImageSampler is true, then this is the image sampler format
	VkDescriptorType          m_descriptorType;            //!< Flags for the descriptor set built for this sampler
	VkSamplerMipmapMode       m_mipmapMode;                //!< Mip map mode for the sampler
	VkFilter                  m_minMagFilter;              //!< Flag for sampler minification / magnification filter
	vectorDescriptorImageInfo m_vectorDescriptorImageInfo; //!< Used in Material::buildPipeline to builld a vector with the descriptor image info for each image present in 
};

/////////////////////////////////////////////////////////////////////////////////////////////

#endif _SAMPLER_H_
