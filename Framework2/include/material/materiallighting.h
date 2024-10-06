/*
Copyright 2018 Alejandro Cosin

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

#ifndef _MATERIALLIGHTING_H_
#define _MATERIALLIGHTING_H_

// GLOBAL INCLUDES

// PROJECT INCLUDES
#include "../../include/util/genericresource.h"
#include "../../include/material/material.h"
#include "../../include/shader/shadermanager.h"
#include "../../include/material/materialmanager.h"
#include "../../include/texture/texturemanager.h"
#include "../../include/texture/texture.h"
#include "../../include/shader/shader.h"
#include "../../include/core/coremanager.h"
#include "../../include/renderpass/renderpass.h"
#include "../../include/renderpass/renderpassmanager.h"

// CLASS FORWARDING

// NAMESPACE

// DEFINES

/////////////////////////////////////////////////////////////////////////////////////////////

class MaterialLighting: public Material
{
	friend class MaterialManager;

protected:
	/** Parameter constructor
	* @param [in] shader's name
	* @return nothing */
	MaterialLighting(string &&name) : Material(move(name), move(string("MaterialLighting")))
		, m_reflectance(nullptr)
		, m_normal(nullptr)
		, m_spotlight(nullptr)
		, m_voxelOccupiedBuffer(nullptr)
		, m_voxelFirstIndexEmitterCompactedBuffer(nullptr)
		, m_indirectionIndexBuffer(nullptr)
		, m_indirectionRankBuffer(nullptr)
		, m_voxelHashedPositionCompactedBuffer(nullptr)
		, m_emitterBuffer(nullptr)
		, m_fragmentIrradianceBuffer(nullptr)
		, m_debugBuffer(nullptr)
		, m_voxelFirstIndexCompactedBuffer(nullptr)
		, m_nextFragmentIndexBuffer(nullptr)
		, m_fragmentDataBuffer(nullptr)
		, m_irradianceFieldGridDensity(0)
		, m_irradianceMultiplier(float(gpuPipelineM->getRasterFlagValue(move(string("IRRADIANCE_MULTIPLIER")))))
		, m_directIrradianceMultiplier(float(gpuPipelineM->getRasterFlagValue(move(string("DIRECT_IRRADIANCE_MULTIPLIER")))))
	{
		m_vectorClearValue[0].color.float32[0] = 0.0f;
		m_vectorClearValue[0].color.float32[1] = 0.0f;
		m_vectorClearValue[0].color.float32[2] = 0.0f;
		m_vectorClearValue[0].color.float32[3] = 0.0f;
	}

public:
	/** Loads the shader to be used. This could be substituted
	* by reading from a file, where the shader details would be present
	* @return true if the shader was loaded successfully, and false otherwise */
	bool loadShader()
	{
		size_t sizeVert;
		size_t sizeFrag;

		void* vertShaderCode = InputOutput::readFile("../data/vulkanshaders/lighting.vert", &sizeVert);
		void* fragShaderCode = InputOutput::readFile("../data/vulkanshaders/lighting.frag", &sizeFrag);

		m_shaderResourceName = "Lighting" + to_string(m_materialInstanceIndex);
		m_shader = shaderM->buildShaderVF(move(string(m_shaderResourceName)), (const char*)vertShaderCode, (const char*)fragShaderCode, m_materialSurfaceType);

		return true;
	}

	/** Load here all the resources needed for this material (textures and the like)
	* @return nothing */
	void loadResources()
	{
		m_reflectance = textureM->getElement(move(string(m_reflectanceTextureName)));
		m_normal      = textureM->getElement(move(string(m_normalTextureName)));
		m_spotlight   = textureM->getElement(move(string(m_spotlightTextureName)));
	}

	void setupPipelineData()
	{
		m_pipeline.refPipelineData().setRenderPass(renderPassM->getElement(move(string("scenelightingrenderpass")))->getRenderPass());

		// The VkVertexInputBinding viIpBind, stores the rate at which the information will be
		// injected for vertex input.
		m_viIpBind[0].binding   = 0;
		m_viIpBind[0].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
		m_viIpBind[0].stride    = 24; // TODO: Replace by constant, here and in the original source in gpupipeline.cpp

		m_viIpBind[1].binding   = 1;
		m_viIpBind[1].inputRate = VK_VERTEX_INPUT_RATE_INSTANCE;
		m_viIpBind[1].stride    = 16; // TODO: Replace by constant, here and in the original source in gpupipeline.cpp

		// The VkVertexInputAttribute - Description) structure, store 
		// the information that helps in interpreting the data.
		// the information that helps in interpreting the data.
		m_viIpAttrb[0].binding  = 0;
		m_viIpAttrb[0].location = 0;
		m_viIpAttrb[0].format   = VK_FORMAT_R32G32B32_SFLOAT;
		m_viIpAttrb[0].offset   = 0;

		m_viIpAttrb[1].binding  = 0;
		m_viIpAttrb[1].location = 1;
		m_viIpAttrb[1].format   = VK_FORMAT_R32G32B32_UINT;
		m_viIpAttrb[1].offset   = 12; // After, 4 components - RGB  each of 4 bytes

		// Per instance vertex atttribute description
		m_viIpAttrb[2].binding  = 1;
		m_viIpAttrb[2].location = 2;
		m_viIpAttrb[2].format   = VK_FORMAT_R32G32B32_SFLOAT;
		m_viIpAttrb[2].offset   = 0;

		m_viIpAttrb[3].binding  = 1;
		m_viIpAttrb[3].location = 3;
		m_viIpAttrb[3].format   = VK_FORMAT_R32_SFLOAT;
		m_viIpAttrb[3].offset   = 12;

		VkPipelineVertexInputStateCreateInfo vertexInputStateInfo;

		vertexInputStateInfo.sType                           = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
		vertexInputStateInfo.pNext                           = nullptr;
		vertexInputStateInfo.flags                           = 0;
		vertexInputStateInfo.vertexBindingDescriptionCount   = 2;
		vertexInputStateInfo.pVertexBindingDescriptions      = &m_viIpBind[0];
		vertexInputStateInfo.vertexAttributeDescriptionCount = 4;
		vertexInputStateInfo.pVertexAttributeDescriptions    = &m_viIpAttrb[0];

		m_pipeline.refPipelineData().setVertexInputStateInfo(vertexInputStateInfo);
	}

	/** Exposes the variables that will be modified in the corresponding material data uniform buffer used in the scene,
	* allowing to work from variables from C++ without needing to update the values manually (error-prone)
	* @return nothing */
	void exposeResources()
	{
		exposeStructField(ResourceInternalType::RIT_FLOAT_VEC4,   (void*)(&m_voxelSize),                   move(string("myMaterialData")), move(string("voxelSize")));
		exposeStructField(ResourceInternalType::RIT_FLOAT_VEC4,   (void*)(&m_sceneMin),                    move(string("myMaterialData")), move(string("sceneMin")));
		exposeStructField(ResourceInternalType::RIT_FLOAT_VEC4,   (void*)(&m_sceneExtent),                 move(string("myMaterialData")), move(string("sceneExtent")));
		exposeStructField(ResourceInternalType::RIT_FLOAT_VEC4,   (void*)(&m_lightPosition),               move(string("myMaterialData")), move(string("lightPosition")));
		exposeStructField(ResourceInternalType::RIT_FLOAT_MAT4,   (void*)(&m_shadowViewProjection),        move(string("myMaterialData")), move(string("shadowViewProjection")));
		exposeStructField(ResourceInternalType::RIT_FLOAT_VEC4,   (void*)(&m_offsetAndSize),               move(string("myMaterialData")), move(string("offsetAndSize")));
		exposeStructField(ResourceInternalType::RIT_FLOAT_VEC4,   (void*)(&m_zFar),                        move(string("myMaterialData")), move(string("zFar")));
		exposeStructField(ResourceInternalType::RIT_FLOAT_VEC4,   (void*)(&m_lightForwardEmitterRadiance), move(string("myMaterialData")), move(string("lightForwardEmitterRadiance")));
		exposeStructField(ResourceInternalType::RIT_UNSIGNED_INT, (void*)(&m_irradianceFieldGridDensity),  move(string("myMaterialData")), move(string("irradianceFieldGridDensity")));
		exposeStructField(ResourceInternalType::RIT_FLOAT,        (void*)(&m_irradianceMultiplier),        move(string("myMaterialData")), move(string("irradianceMultiplier")));
		exposeStructField(ResourceInternalType::RIT_FLOAT,        (void*)(&m_directIrradianceMultiplier),  move(string("myMaterialData")), move(string("directIrradianceMultiplier")));
		
		assignTextureToSampler(move(string("reflectance")),                  move(string(m_reflectanceTextureName)),       VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
		assignTextureToSampler(move(string("normal")),                       move(string(m_normalTextureName)),            VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
		assignTextureToSampler(move(string("irradiance3DStaticNegativeX")),  move(string("irradiance3DStaticNegativeX")),  VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
		assignTextureToSampler(move(string("irradiance3DStaticPositiveX")),  move(string("irradiance3DStaticPositiveX")),  VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
		assignTextureToSampler(move(string("irradiance3DStaticNegativeY")),  move(string("irradiance3DStaticNegativeY")),  VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
		assignTextureToSampler(move(string("irradiance3DStaticPositiveY")),  move(string("irradiance3DStaticPositiveY")),  VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
		assignTextureToSampler(move(string("irradiance3DStaticNegativeZ")),  move(string("irradiance3DStaticNegativeZ")),  VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
		assignTextureToSampler(move(string("irradiance3DStaticPositiveZ")),  move(string("irradiance3DStaticPositiveZ")),  VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
		assignTextureToSampler(move(string("irradiance3DDynamicNegativeX")), move(string("irradiance3DDynamicNegativeX")), VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
		assignTextureToSampler(move(string("irradiance3DDynamicPositiveX")), move(string("irradiance3DDynamicPositiveX")), VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
		assignTextureToSampler(move(string("irradiance3DDynamicNegativeY")), move(string("irradiance3DDynamicNegativeY")), VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
		assignTextureToSampler(move(string("irradiance3DDynamicPositiveY")), move(string("irradiance3DDynamicPositiveY")), VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
		assignTextureToSampler(move(string("irradiance3DDynamicNegativeZ")), move(string("irradiance3DDynamicNegativeZ")), VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
		assignTextureToSampler(move(string("irradiance3DDynamicPositiveZ")), move(string("irradiance3DDynamicPositiveZ")), VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);

		assignShaderStorageBuffer(move(string("voxelOccupiedBuffer")),               move(string("voxelOccupiedBuffer")),               VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);
		assignShaderStorageBuffer(move(string("debugBuffer")),                       move(string("debugBuffer")),                       VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);
		assignShaderStorageBuffer(move(string("litTestVoxelPerByteBuffer")),         move(string("litTestVoxelPerByteBuffer")),         VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);
		assignShaderStorageBuffer(move(string("litTestDynamicVoxelPerByteBuffer")),  move(string("litTestDynamicVoxelPerByteBuffer")),  VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);
		assignShaderStorageBuffer(move(string("voxelOccupiedDynamicBuffer")),        move(string("voxelOccupiedDynamicBuffer")),        VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);
		assignShaderStorageBuffer(move(string("irradiancePaddingTagTilesBuffer")),   move(string("irradiancePaddingTagTilesBuffer")),   VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);
		assignShaderStorageBuffer(move(string("irradianceFilteringTagTilesBuffer")), move(string("irradianceFilteringTagTilesBuffer")), VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);

		assignAccelerationStructure(move(string("raytracedaccelerationstructure")),        move(string("raytracedaccelerationstructure")),        VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR);
	}

	GET_SET(string, m_reflectanceTextureName, ReflectanceTextureName)
	GET_SET(string, m_normalTextureName, NormalTextureName)
	GET_SET(string, m_spotlightTextureName, SpotlightTextureName)
	SET(vec4, m_voxelSize, VoxelSize)
	SET(vec4, m_sceneMin, SceneMin)
	SET(vec4, m_sceneExtent, SceneExtent)
	SET(vec4, m_lightPosition, LightPosition)
	SET(mat4, m_shadowViewProjection, ShadowViewProjection)
	SET(vec4, m_offsetAndSize, OffsetAndSize)
	SET(vec4, m_zFar, ZFar)
	SET(uint, m_irradianceFieldGridDensity, IrradianceFieldGridDensity)
	SET(vec4, m_lightForwardEmitterRadiance, LightForwardEmitterRadiance)
	GETCOPY_SET(float, m_irradianceMultiplier, IrradianceMultiplier)
	GETCOPY_SET(float, m_directIrradianceMultiplier, DirectIrradianceMultiplier)

protected:
	string                            m_reflectanceTextureName;                //!< Name of the reflectance texture resource to assign to m_reflectance
	string                            m_normalTextureName;                     //!< Name of the reflectance texture resource to assign to m_normal
	string                            m_spotlightTextureName;                  //!< Spotlight mask texture name
	Texture*                          m_reflectance;                           //!< Reflectance texture
	Texture*                          m_normal;                                //!< Normal texture
	Texture*                          m_spotlight;                             //!< Spotlight texture
	Buffer*                           m_voxelOccupiedBuffer;                   //!< Shader storage buffer to know whether a voxel position is occupied or is not
	Buffer*                           m_voxelFirstIndexEmitterCompactedBuffer; //!< Buffer with the geometry of all emitters in the scene (which are supposed to be planar). For now, only one emitter is used.
	Buffer*                           m_indirectionIndexBuffer;                //!< Shader storage buffer for accelerating the search of elements in m_voxelFirstIndexCompactedBuffer and m_voxelHashedPositionCompactedBuffer. Contains the index to the first element in m_voxelFirstIndexCompactedBuffer and m_voxelHashedPositionCompactedBuffer for fixed 3D voxelization values, like (0,0,0), (0,0,128), (0,1,0), (0,1,128), (0,2,0), (0,2,128), ...
	Buffer*                           m_indirectionRankBuffer;                 //!< Shader storage buffer for accelerating the search of elements in m_voxelFirstIndexCompactedBuffer and m_voxelHashedPositionCompactedBuffer. Contains the amount of elements to the right in m_voxelFirstIndexCompactedBuffer for the index in m_voxelFirstIndexCompactedBuffer given at the same position at m_indirectionIndexBuffer
	Buffer*                           m_voxelHashedPositionCompactedBuffer;    //!< Shader storage buffer with the hashed position of the 3D volume voxelization coordinates the fragment data in the same index at the m_voxelFirstIndexCompactedBuffer buffer occupied initially in the non-compacted SSBO
	Buffer*                           m_emitterBuffer;                         //!< Buffer with the geometry of all emitters in the scene (which are supposed to be planar). For now, only one emitter is used.
	vec4                              m_voxelSize;                             //!< 3D voxel texture size
	vec4                              m_sceneMin;                              //!< Minimum value of the scene's aabb
	vec4                              m_sceneExtent;                           //!< Extent of the scene
	vec4                              m_lightPosition;                         //!< Light world position
	mat4                              m_shadowViewProjection;                  //!< Shadow mapping view projection matrix
	Buffer*                           m_fragmentIrradianceBuffer;              //!< Shader storage buffer with the per-fragment irradiance for all scene emitters
	Buffer*                           m_debugBuffer;
	Buffer*                           m_voxelFirstIndexCompactedBuffer;        //!< Shader storage buffer with the parallel prefix sum algorithm result
	Buffer*                           m_nextFragmentIndexBuffer;               //!< Shader storage buffer with the next fragment index in the case several fragments fall in the same 3D voxelization volume
	Buffer*                           m_fragmentDataBuffer;                    //!< Shader storage buffer with the per-fragment data generated during the voxelization
	vec4                              m_offsetAndSize;                         //!< Texture offset for rasterizing the omnidirectional distance maps (x, y coordinates), size of the patch (z coordinate) and size of the texture (w coordinate)
	vec4                              m_zFar;                                  //!< Shadow distance map values are divided by z far camera plane value (x coordinate) to set values approximately in the [0,1] interval, although values can be bigger than 1
	uint                              m_irradianceFieldGridDensity;            //!< Density of the irradiance field's grid (value n means one irradiance field each n units in the voxelization volume in all dimensions)
	vec4                              m_lightForwardEmitterRadiance;           //!< Light forward direction in xyz components, emitter radiance in w component
	float                             m_irradianceMultiplier;                  //!< Equivalent to IRRADIANCE_MULTIPLIER
	float                             m_directIrradianceMultiplier;            //!< Equivalent to DIRECT_IRRADIANCE_MULTIPLIER
	VkVertexInputBindingDescription   m_viIpBind[2];                           //!< Stores the vertex input rate
	VkVertexInputAttributeDescription m_viIpAttrb[4];                          //!< Store metadata helpful in data interpretation
};

/////////////////////////////////////////////////////////////////////////////////////////////

#endif _MATERIALLIGHTING_H_
