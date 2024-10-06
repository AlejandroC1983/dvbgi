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

#ifndef _MATERIALANTIALIASING_H_
#define _MATERIALANTIALIASING_H_

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

// CLASS FORWARDING

// NAMESPACE

// DEFINES

/////////////////////////////////////////////////////////////////////////////////////////////

class MaterialAntialiasing: public Material
{
	friend class MaterialManager;

protected:
	/** Parameter constructor
	* @param name [in] shader's name
	* @return nothing */
	MaterialAntialiasing(string &&name) : Material(move(name), move(string("MaterialAntialiasing")))
	{
		m_vectorClearValue.resize(2);
		m_vectorClearValue[0].color.float32[0]     = 0.0f;
		m_vectorClearValue[0].color.float32[1]     = 0.0f;
		m_vectorClearValue[0].color.float32[2]     = 0.0f;
		m_vectorClearValue[0].color.float32[3]     = 0.0f;
		m_vectorClearValue[1].depthStencil.depth   = 1.0f;
		m_vectorClearValue[1].depthStencil.stencil = 0;

		m_resourcesUsed = MaterialBufferResource::MBR_MATERIAL;
	}

public:
	/** Loads the shader to be used. This could be substituted
	* by reading from a file, where the shader details would be present
	* @return true if the shader was loaded successfully, and false otherwise */
	bool loadShader()
	{
		size_t sizeVert;
		size_t sizeFrag;

		void* vertShaderCode = InputOutput::readFile("../data/vulkanshaders/antialiasing.vert", &sizeVert);
		void* fragShaderCode = InputOutput::readFile("../data/vulkanshaders/antialiasing.frag", &sizeFrag);

		m_shader = shaderM->buildShaderVF(move(string("antialiasing")), (const char*)vertShaderCode, (const char*)fragShaderCode, m_materialSurfaceType);

		return true;
	}

	void setupPipelineData()
	{
		VkPipelineRasterizationStateCreateInfo rasterStateInfo = m_pipeline.refPipelineData().getRasterStateInfo();
		rasterStateInfo.depthClampEnable        = 0;
		rasterStateInfo.rasterizerDiscardEnable = 0;
		rasterStateInfo.polygonMode             = VK_POLYGON_MODE_FILL;
		rasterStateInfo.cullMode                = VK_CULL_MODE_NONE;
		rasterStateInfo.frontFace               = VK_FRONT_FACE_COUNTER_CLOCKWISE;
		rasterStateInfo.depthBiasEnable         = 0;
		rasterStateInfo.depthBiasConstantFactor = 0.0f;
		rasterStateInfo.depthBiasClamp          = 0.0f;
		rasterStateInfo.depthBiasSlopeFactor    = 0.0f;
		rasterStateInfo.lineWidth               = 1.0f;
		m_pipeline.refPipelineData().updateRasterStateInfo(rasterStateInfo);

		VkPipelineVertexInputStateCreateInfo vertexInputStateInfo = m_pipeline.refPipelineData().getVertexInputStateInfo();
		vertexInputStateInfo.vertexBindingDescriptionCount        = 0;
		vertexInputStateInfo.pVertexBindingDescriptions           = nullptr;
		vertexInputStateInfo.vertexAttributeDescriptionCount      = 0;
		vertexInputStateInfo.pVertexAttributeDescriptions         = nullptr;
		m_pipeline.refPipelineData().updateVertexInputStateInfo(vertexInputStateInfo);

		vector<VkPipelineColorBlendAttachmentState> arrayColorBlendAttachmentStateInfo = m_pipeline.refPipelineData().getArrayColorBlendAttachmentStateInfo();
		arrayColorBlendAttachmentStateInfo[0].blendEnable         = 0;
		arrayColorBlendAttachmentStateInfo[0].srcColorBlendFactor = VK_BLEND_FACTOR_ZERO;
		arrayColorBlendAttachmentStateInfo[0].dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
		arrayColorBlendAttachmentStateInfo[0].colorBlendOp        = VK_BLEND_OP_ADD;
		arrayColorBlendAttachmentStateInfo[0].srcAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
		arrayColorBlendAttachmentStateInfo[0].dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
		arrayColorBlendAttachmentStateInfo[0].alphaBlendOp        = VK_BLEND_OP_ADD;
		arrayColorBlendAttachmentStateInfo[0].colorWriteMask      = 15;

		VkPipelineColorBlendStateCreateInfo colorBlendStateInfo = m_pipeline.refPipelineData().getColorBlendStateInfo();
		colorBlendStateInfo.blendConstants[0] = 0.0f;
		colorBlendStateInfo.blendConstants[1] = 0.0f;
		colorBlendStateInfo.blendConstants[2] = 0.0f;
		colorBlendStateInfo.blendConstants[3] = 0.0f;

		m_pipeline.refPipelineData().updateColorBlendStateInfo(colorBlendStateInfo, arrayColorBlendAttachmentStateInfo);

		VkPipelineDepthStencilStateCreateInfo depthStencilStateInfo = m_pipeline.refPipelineData().getDepthStencilStateInfo();
		depthStencilStateInfo.front.failOp      = VK_STENCIL_OP_KEEP;
		depthStencilStateInfo.front.passOp      = VK_STENCIL_OP_KEEP;
		depthStencilStateInfo.front.depthFailOp = VK_STENCIL_OP_KEEP;
		depthStencilStateInfo.front.compareOp   = VK_COMPARE_OP_NEVER;
		depthStencilStateInfo.front.compareMask = 0;
		depthStencilStateInfo.front.writeMask   = 0;
		depthStencilStateInfo.front.reference   = 0;
		depthStencilStateInfo.back              = depthStencilStateInfo.front;
		depthStencilStateInfo.back.compareOp    = VK_COMPARE_OP_ALWAYS;
		m_pipeline.refPipelineData().updateDepthStencilStateInfo(depthStencilStateInfo);

		m_pipeline.refPipelineData().setRenderPass(coreM->refRenderPass());
	}

	/** Exposes the variables that will be modified in the corresponding material data uniform buffer used in the scene,
	* allowing to work from variables from C++ without needing to update the values manually (error-prone)
	* @return nothing */
	void exposeResources()
	{
		exposeStructField(ResourceInternalType::RIT_FLOAT_VEC4, (void*)(&m_renderTargetSize), move(string("myMaterialData")), move(string("renderTargetSize")));

		assignTextureToSampler(move(string("scenelightingcolor")), move(string("scenelightingcolor")), VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
	}

	SET(vec4, m_renderTargetSize, RenderTargetSize)

protected:
	vec4 m_renderTargetSize; //!< Resolution of the processed texture
};

/////////////////////////////////////////////////////////////////////////////////////////////

#endif _MATERIALANTIALIASING_H_
