/*
Copyright 2023 Alejandro Cosin

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

#ifndef _MATERIALGBUFFER_H_
#define _MATERIALGBUFFER_H_

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
class Shader;

// NAMESPACE

// DEFINES

/////////////////////////////////////////////////////////////////////////////////////////////

class MaterialGBuffer: public Material
{
	friend class MaterialManager;

protected:
	/** Parameter constructor
	* @param [in] shader's name
	* @return nothing */
	MaterialGBuffer(string &&name) : Material(move(name), move(string("MaterialGBuffer")))
		, m_reflectance(nullptr)
		, m_normal(nullptr)
	{
		m_vectorClearValue.resize(4);

		m_vectorClearValue[0].color.uint32[0] = 0;
		m_vectorClearValue[0].color.uint32[1] = 0;
		m_vectorClearValue[0].color.uint32[2] = 0;
		m_vectorClearValue[0].color.uint32[3] = 0;

		m_vectorClearValue[1].color.uint32[0] = 0;
		m_vectorClearValue[1].color.uint32[1] = 0;
		m_vectorClearValue[1].color.uint32[2] = 0;
		m_vectorClearValue[1].color.uint32[3] = 0;

		m_vectorClearValue[2].color.float32[0] = 0.0f;
		m_vectorClearValue[2].color.float32[1] = 0.0f;
		m_vectorClearValue[2].color.float32[2] = 0.0f;
		m_vectorClearValue[2].color.float32[3] = 0.0f;

		m_vectorClearValue[3].depthStencil.depth   = 0.0f;
		m_vectorClearValue[3].depthStencil.stencil = 0;
	}

public:
	/** Loads the shader to be used. This could be substituted
	* by reading from a file, where the shader details would be present
	* @return true if the shader was loaded successfully, and false otherwise */
	bool loadShader()
	{
		size_t sizeVert;
		size_t sizeFrag;

		void* vertShaderCode = InputOutput::readFile("../data/vulkanshaders/gbuffer.vert", &sizeVert);
		void* fragShaderCode = InputOutput::readFile("../data/vulkanshaders/gbuffer.frag", &sizeFrag);

		m_shaderResourceName = "gbuffer" + to_string(m_materialInstanceIndex);
		m_shader = shaderM->buildShaderVF(move(string(m_shaderResourceName)), (const char*)vertShaderCode, (const char*)fragShaderCode, m_materialSurfaceType);

		return true;
	}

	/** Load here all the resources needed for this material (textures and the like)
	* @return nothing */
	void loadResources()
	{
		m_reflectance = textureM->getElement(move(string(m_reflectanceTextureName))); //textureM->getElement(move(string("reflectanceTexture0")));
		m_normal      = textureM->getElement(move(string(m_normalTextureName)));
	}

	void setupPipelineData()
	{
		m_pipeline.refPipelineData().setRenderPass(renderPassM->getElement(move(string("gbufferrenderpass")))->getRenderPass());

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

		vector<VkPipelineColorBlendAttachmentState> arrayColorBlendAttachmentStateInfo(3);
		forI(3)
		{
			arrayColorBlendAttachmentStateInfo[i].blendEnable         = 0;
			arrayColorBlendAttachmentStateInfo[i].srcColorBlendFactor = VK_BLEND_FACTOR_ZERO;
			arrayColorBlendAttachmentStateInfo[i].dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
			arrayColorBlendAttachmentStateInfo[i].colorBlendOp        = VK_BLEND_OP_ADD;
			arrayColorBlendAttachmentStateInfo[i].srcAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
			arrayColorBlendAttachmentStateInfo[i].dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
			arrayColorBlendAttachmentStateInfo[i].alphaBlendOp        = VK_BLEND_OP_ADD;
			arrayColorBlendAttachmentStateInfo[i].colorWriteMask      = VK_PIPELINE_CREATE_DISABLE_OPTIMIZATION_BIT | VK_PIPELINE_CREATE_ALLOW_DERIVATIVES_BIT | VK_PIPELINE_CREATE_DERIVATIVE_BIT | VK_PIPELINE_CREATE_VIEW_INDEX_FROM_DEVICE_INDEX_BIT;
		}

		m_pipeline.refPipelineData().updateArrayColorBlendAttachmentStateInfo(arrayColorBlendAttachmentStateInfo);
		m_pipeline.refPipelineData().updateColorBlendStateBlendConstants(vec4(1.0f));
	}

	/** Exposes the variables that will be modified in the corresponding material data uniform buffer used in the scene,
	* allowing to work from variables from C++ without needing to update the values manually (error-prone)
	* @return nothing */
	void exposeResources()
	{
		exposeStructField(ResourceInternalType::RIT_FLOAT_VEC4, (void*)(&m_sceneMin),                    move(string("myMaterialData")), move(string("sceneMin")));
		exposeStructField(ResourceInternalType::RIT_FLOAT_VEC4, (void*)(&m_sceneExtentVoxelizationSize), move(string("myMaterialData")), move(string("sceneExtentVoxelizationSize")));

		assignTextureToSampler(move(string("reflectance")), move(string(m_reflectanceTextureName)), VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
		assignTextureToSampler(move(string("normal")),      move(string(m_normalTextureName)),      VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);

		assignShaderStorageBuffer(move(string("voxelOccupiedBuffer")), move(string("voxelOccupiedBuffer")), VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);
	}

	GET_SET(string, m_reflectanceTextureName, ReflectanceTextureName)
	GET_SET(string, m_normalTextureName, NormalTextureName)
	SET(vec4, m_sceneMin, SceneMin)
	SET(vec4, m_sceneExtentVoxelizationSize, SceneExtentVoxelizationSize)

protected:
	string                            m_reflectanceTextureName;      //!< Name of the reflectance texture resource to assign to m_reflectance
	string                            m_normalTextureName;           //!< Name of the reflectance texture resource to assign to m_normal
	Texture*                          m_reflectance;                 //!< Reflectance texture
	Texture*                          m_normal;                      //!< Normal texture
	vec4                              m_sceneMin;                    //!< Scene min
	vec4                              m_sceneExtentVoxelizationSize; //!< Scene extent and voxel size in w field
	VkVertexInputBindingDescription   m_viIpBind[2];                 //!< Stores the vertex input rate
	VkVertexInputAttributeDescription m_viIpAttrb[4];                //!< Store metadata helpful in data interpretation
};

/////////////////////////////////////////////////////////////////////////////////////////////

#endif _MATERIALGBUFFER_H_
