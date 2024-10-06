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

#ifndef _MATERIALSCENELIGHTINGDEFERRED_H_
#define _MATERIALSCENELIGHTINGDEFERRED_H_

// GLOBAL INCLUDES

// PROJECT INCLUDES
#include "../../include/util/genericresource.h"
#include "../../include/material/material.h"
#include "../../include/shader/shadermanager.h"
#include "../../include/shader/shader.h"
#include "../../include/core/coremanager.h"

// CLASS FORWARDING

// NAMESPACE

// DEFINES

/////////////////////////////////////////////////////////////////////////////////////////////

class MaterialSceneLightingDeferred: public Material
{
	friend class MaterialManager;

protected:
	/** Parameter constructor
	* @param name [in] shader's name
	* @return nothing */
	MaterialSceneLightingDeferred(string &&name) : Material(move(name), move(string("MaterialSceneLightingDeferred")))
		, m_irradianceMultiplier(float(gpuPipelineM->getRasterFlagValue(move(string("IRRADIANCE_MULTIPLIER")))))
		, m_directIrradianceMultiplier(float(gpuPipelineM->getRasterFlagValue(move(string("DIRECT_IRRADIANCE_MULTIPLIER")))))
		, m_lightPosition(vec4(0.0f))
		, m_lightForward(vec4(0.0f))
		, m_sceneMinEmitterRadiance(vec4(0.0f))
		, m_sceneExtentVoxelSize(vec4(0.0f))
		, m_brightness(0.36f)
		, m_contrast(1.025f)
		, m_saturation(0.82f)

		, m_intensity(1.0f)
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
		void* fragShaderCode = InputOutput::readFile("../data/vulkanshaders/scenelightingdeferred.frag", &sizeFrag);

		m_shaderResourceName = "scenelightingdeferred";
		m_shader = shaderM->buildShaderVF(move(string("scenelightingdeferred")), (const char*)vertShaderCode, (const char*)fragShaderCode, m_materialSurfaceType);

		return true;
	}

	void setupPipelineData()
	{
		// TODO: Refactor with antialiasing
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
		exposeStructField(ResourceInternalType::RIT_FLOAT_VEC4, (void*)(&m_lightPosition),              move(string("myMaterialData")), move(string("lightPosition")));
		exposeStructField(ResourceInternalType::RIT_FLOAT_VEC4, (void*)(&m_lightForward),               move(string("myMaterialData")), move(string("lightForward")));
		exposeStructField(ResourceInternalType::RIT_FLOAT_VEC4, (void*)(&m_sceneMinEmitterRadiance),    move(string("myMaterialData")), move(string("sceneMinEmitterRadiance")));
		exposeStructField(ResourceInternalType::RIT_FLOAT_VEC4, (void*)(&m_sceneExtentVoxelSize),       move(string("myMaterialData")), move(string("sceneExtentVoxelSize")));
		exposeStructField(ResourceInternalType::RIT_FLOAT,      (void*)(&m_irradianceMultiplier),       move(string("myMaterialData")), move(string("irradianceMultiplier")));
		exposeStructField(ResourceInternalType::RIT_FLOAT,      (void*)(&m_directIrradianceMultiplier), move(string("myMaterialData")), move(string("directIrradianceMultiplier")));
		exposeStructField(ResourceInternalType::RIT_FLOAT,      (void*)(&m_brightness),                 move(string("myMaterialData")), move(string("brightness")));
		exposeStructField(ResourceInternalType::RIT_FLOAT,      (void*)(&m_contrast),                   move(string("myMaterialData")), move(string("contrast")));
		exposeStructField(ResourceInternalType::RIT_FLOAT,      (void*)(&m_saturation),                 move(string("myMaterialData")), move(string("saturation")));
		exposeStructField(ResourceInternalType::RIT_FLOAT,      (void*)(&m_intensity),                  move(string("myMaterialData")), move(string("intensity")));

		assignTextureToSampler(move(string("GBufferNormal")),                move(string("GBufferNormal")),                VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
		assignTextureToSampler(move(string("GBufferReflectance")),           move(string("GBufferReflectance")),           VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
		assignTextureToSampler(move(string("GBufferPosition")),              move(string("GBufferPosition")),              VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
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

		assignTextureToSampler(move(string("staticVoxelIndexTexture")), move(string("staticVoxelIndexTexture")), VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);

		assignShaderStorageBuffer(move(string("sceneLightingDeferredDebugBuffer")),   move(string("sceneLightingDeferredDebugBuffer")),   VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);
		assignShaderStorageBuffer(move(string("neighbourLitVoxelInformationBuffer")), move(string("neighbourLitVoxelInformationBuffer")), VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);
		assignShaderStorageBuffer(move(string("litTestVoxelPerByteBuffer")),          move(string("litTestVoxelPerByteBuffer")),          VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);
		assignShaderStorageBuffer(move(string("occupiedStaticVoxelTileBuffer")),      move(string("occupiedStaticVoxelTileBuffer")),      VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);		
	}

	SET(vec4, m_lightPosition, LightPosition)
	SET(vec4, m_lightForward, LightForward)
	SET(vec4, m_sceneMinEmitterRadiance, SceneMinEmitterRadiance)
	SET(vec4, m_sceneExtentVoxelSize, SceneExtentVoxelSize)
	GET_SET(float, m_irradianceMultiplier, IrradianceMultiplier)
	GET_SET(float, m_directIrradianceMultiplier, DirectIrradianceMultiplier)
	GETCOPY_SET(float, m_brightness, Brightness)
	GETCOPY_SET(float, m_contrast, Contrast)
	GETCOPY_SET(float, m_saturation, Saturation)
	GETCOPY_SET(float, m_intensity, Intensity)

protected:
	vec4  m_lightPosition;              //!< Light position in world coordinates
	vec4  m_lightForward;               //!< Light direction in world coordinates
	vec4  m_sceneMinEmitterRadiance;    //!< Scene min AABB value (x,y and z fields) and emitter radiance (w field)
	vec4  m_sceneExtentVoxelSize;       //!< Scene extent (x,y and z fields) and voxelization resolution (w field)
	float m_irradianceMultiplier;       //!< Equivalent to IRRADIANCE_MULTIPLIER
	float m_directIrradianceMultiplier; //!< Equivalent to DIRECT_IRRADIANCE_MULTIPLIER
	float m_brightness;                 //!< Brightness value to be applied in the shaders used in this technique
	float m_contrast;                   //!< Contrast value to be applied in the shaders used in this technique
	float m_saturation;                 //!< Saturation value to be applied in the shaders used in this technique
	float m_intensity;                  //!< Intensity value to be added to the final color
};

/////////////////////////////////////////////////////////////////////////////////////////////

#endif _MATERIALSCENELIGHTINGDEFERRED_H_
