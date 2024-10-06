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

#ifndef _MATERIALSCENEVOXELIZATION_H_
#define _MATERIALSCENEVOXELIZATION_H_

// GLOBAL INCLUDES

// PROJECT INCLUDES
#include "../../include/material/materialmanager.h"
#include "../../include/material/material.h"
#include "../../include/shader/shadermanager.h"
#include "../../include/shader/shader.h"
#include "../../include/texture/texturemanager.h"
#include "../../include/texture/texture.h"
#include "../../include/renderpass/renderpassmanager.h"
#include "../../include/renderpass/renderpass.h"
#include "../../include/core/coremanager.h"
#include "../../include/buffer/buffermanager.h"
#include "../../include/buffer/buffer.h"

// CLASS FORWARDING

// NAMESPACE

// DEFINES

/////////////////////////////////////////////////////////////////////////////////////////////

class MaterialSceneVoxelization: public Material
{
	friend class MaterialManager;

protected:
	/** Parameter constructor
	* @param [in] shader's name
	* @return nothing */
	MaterialSceneVoxelization(string &&name) : Material(move(name), move(string("MaterialSceneVoxelization")))
		, m_voxelizationSize(vec4(0.0f))
		, m_reflectance(nullptr)
		, m_normal(nullptr)
	{
		m_vectorClearValue.resize(1);
		m_vectorClearValue[0].color.float32[0]     = 0.0f;
		m_vectorClearValue[0].color.float32[1]     = 0.0f;
		m_vectorClearValue[0].color.float32[2]     = 0.0f;
		m_vectorClearValue[0].color.float32[3]     = 0.0f;
		m_vectorClearValue[0].depthStencil.depth   = 0.0f;
		m_vectorClearValue[0].depthStencil.stencil = 0;
	}

public:
	/** Loads the shader to be used. This could be substituted
	* by reading from a file, where the shader details would be present
	* @return true if the shader was loaded successfully, and false otherwise */
	bool loadShader()
	{
		size_t sizeTemp;

		void* vertShaderCode     = InputOutput::readFile("../data/vulkanshaders/voxelization.vert", &sizeTemp);
		void* geometryShaderCode = InputOutput::readFile("../data/vulkanshaders/voxelization.geom", &sizeTemp);
		void* fragShaderCode     = InputOutput::readFile("../data/vulkanshaders/voxelization.frag", &sizeTemp);

		m_shaderResourceName = "scenevoxelization" + to_string(m_materialInstanceIndex);
		m_shader = shaderM->buildShaderVGF(move(string(m_shaderResourceName)), (const char*)vertShaderCode, (const char*)geometryShaderCode, (const char*)fragShaderCode, m_materialSurfaceType);

		return true;
	}

	/** Exposes the variables that will be modified in the corresponding material data uniform buffer used in the scene,
	* allowing to work from variables from C++ without needing to update the values manually (error-prone)
	* @return nothing */
	void exposeResources()
	{
		exposeStructField(ResourceInternalType::RIT_FLOAT_MAT4, (void*)(&m_projection),       move(string("myMaterialData")), move(string("projection")));
		exposeStructField(ResourceInternalType::RIT_FLOAT_MAT4, (void*)(&m_viewX),            move(string("myMaterialData")), move(string("viewX")));
		exposeStructField(ResourceInternalType::RIT_FLOAT_MAT4, (void*)(&m_viewY),            move(string("myMaterialData")), move(string("viewY")));
		exposeStructField(ResourceInternalType::RIT_FLOAT_MAT4, (void*)(&m_viewZ),            move(string("myMaterialData")), move(string("viewZ")));
		exposeStructField(ResourceInternalType::RIT_FLOAT_VEC4, (void*)(&m_voxelizationSize), move(string("myMaterialData")), move(string("voxelizationSize")));

		assignImageToSampler(move(string("voxelizationReflectance")), move(string("voxelizationReflectance")), VK_DESCRIPTOR_TYPE_STORAGE_IMAGE);
		assignImageToSampler(move(string("voxelization3DDebug")),     move(string("voxelization3DDebug")),     VK_DESCRIPTOR_TYPE_STORAGE_IMAGE);

		assignShaderStorageBuffer(move(string("voxelOccupiedBuffer")),   move(string("voxelOccupiedBuffer")),   VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);
		assignShaderStorageBuffer(move(string("voxelFirstIndexBuffer")), move(string("voxelFirstIndexBuffer")), VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);

		assignTextureToSampler(move(string("reflectanceMap")), move(string(m_reflectanceTextureName)), VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
	}

	/** Load here all the resources needed for this material (textures and the like)
	* @return nothing */
	void loadResources()
	{
		m_reflectance = textureM->getElement(move(string(m_reflectanceTextureName)));
		m_normal      = textureM->getElement(move(string(m_normalTextureName)));
	}

	void setupPipelineData()
	{
		m_pipeline.refPipelineData().setRenderPass(renderPassM->getElement(move(string("voxelizationrenderpass")))->getRenderPass());

		m_pipeline.refPipelineData().setNullDepthStencilState();

		VkPipelineMultisampleStateCreateInfo multiSampleStateInfo = m_pipeline.refPipelineData().getMultiSampleStateInfo();
		multiSampleStateInfo.rasterizationSamples = VK_SAMPLE_COUNT_8_BIT;
		multiSampleStateInfo.minSampleShading     = 1.0f;
		m_pipeline.refPipelineData().setMultiSampleStateInfo(multiSampleStateInfo);

		VkPipelineColorBlendStateCreateInfo colorBlendStateInfo = m_pipeline.refPipelineData().getColorBlendStateInfo();
		colorBlendStateInfo.blendConstants[0] = 0.0f;
		colorBlendStateInfo.blendConstants[1] = 0.0f;
		colorBlendStateInfo.blendConstants[2] = 0.0f;
		colorBlendStateInfo.blendConstants[3] = 0.0f;
		colorBlendStateInfo.logicOp = VK_LOGIC_OP_COPY;
		m_pipeline.refPipelineData().setColorBlendStateInfo(colorBlendStateInfo);

		vector<VkPipelineColorBlendAttachmentState> arrayColorBlendAttachmentStateInfo = m_pipeline.refPipelineData().getArrayColorBlendAttachmentStateInfo();
		arrayColorBlendAttachmentStateInfo[0].srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
		arrayColorBlendAttachmentStateInfo[0].dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
		arrayColorBlendAttachmentStateInfo[0].srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
		arrayColorBlendAttachmentStateInfo[0].dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
		m_pipeline.refPipelineData().updateArrayColorBlendAttachmentStateInfo(arrayColorBlendAttachmentStateInfo);

		VkPipelineRasterizationStateCreateInfo rasterStateInfo = m_pipeline.refPipelineData().getRasterStateInfo();
		rasterStateInfo.depthClampEnable = VK_FALSE;
		rasterStateInfo.cullMode         = VK_CULL_MODE_NONE;
		rasterStateInfo.frontFace        = VK_FRONT_FACE_COUNTER_CLOCKWISE;
		m_pipeline.refPipelineData().setRasterStateInfo(rasterStateInfo);
	}

	SET(mat4, m_projection, Projection)
	SET(mat4, m_viewX, ViewX)
	SET(mat4, m_viewY, ViewY)
	SET(mat4, m_viewZ, ViewZ)
	SET(vec4, m_voxelizationSize, VoxelizationSize)
	GET_SET(string, m_reflectanceTextureName, ReflectanceTextureName)
	GET_SET(string, m_normalTextureName, NormalTextureName)

protected:
	mat4     m_projection;             //!< Orthographic projection matrix used for voxelization
	mat4     m_viewX;                  //!< x axis view matrix used for voxelization
	mat4     m_viewY;                  //!< y axis view matrix used for voxelization
	mat4     m_viewZ;                  //!< z axis view matrix used for voxelization
	vec4     m_voxelizationSize;       //!< 3D voxel texture size
	string   m_reflectanceTextureName; //!< Name of the reflectance texture resource to assign to m_reflectance
	string   m_normalTextureName;      //!< Name of the reflectance texture resource to assign to m_normal
	Texture* m_reflectance;            //!< Reflectance texture
	Texture* m_normal;                 //!< Normal texture
};

/////////////////////////////////////////////////////////////////////////////////////////////

#endif _MATERIALSCENEVOXELIZATION_H_
