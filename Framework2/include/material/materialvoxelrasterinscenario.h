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

#ifndef _MATERIALVOXELRASTERINSCENARIO_H_
#define _MATERIALVOXELRASTERINSCENARIO_H_

// GLOBAL INCLUDES

// PROJECT INCLUDES
#include "../../include/material/materialmanager.h"
#include "../../include/material/material.h"
#include "../../include/scene/scene.h"
#include "../../include/shader/shadermanager.h"
#include "../../include/shader/shader.h"
#include "../../include/renderpass/renderpassmanager.h"
#include "../../include/renderpass/renderpass.h"
#include "../../include/rastertechnique/staticscenevoxelizationtechnique.h"

// CLASS FORWARDING

// NAMESPACE

// DEFINES

/////////////////////////////////////////////////////////////////////////////////////////////

class MaterialVoxelRasterInScenario: public Material
{
	friend class MaterialManager;

protected:
	/** Parameter constructor
	* @param [in] shader's name
	* @return nothing */
	MaterialVoxelRasterInScenario(string &&name) : Material(move(name), move(string("MaterialVoxelRasterInScenario")))
		, m_numOcupiedVoxel(0)
		, m_padding0(0)
		, m_padding1(0)
		, m_padding2(0)
		, m_pushConstantVoxelFromStaticCompacted(0)
	{
		m_resourcesUsed = (MaterialBufferResource::MBR_CAMERA | MaterialBufferResource::MBR_MATERIAL);

		m_vectorClearValue.resize(2);
		m_vectorClearValue[0].color        = { { 0.0f, 0.0f, 0.0f, 0.0f } };
		m_vectorClearValue[1].depthStencil = { 1.0f, 0 };

		StaticSceneVoxelizationTechnique* technique = static_cast<StaticSceneVoxelizationTechnique*>(gpuPipelineM->getRasterTechniqueByName(move(string("StaticSceneVoxelizationTechnique"))));
		m_voxelizationSize = vec4(float(technique->getVoxelizedSceneWidth()), technique->getVoxelizedSceneHeight(), technique->getVoxelizedSceneDepth(), 0.0f);

		BBox3D& sceneAABB = sceneM->refBox();

		vec3 m;
		vec3 M;
		sceneAABB.getCenteredBoxMinMax(m, M);
		m_sceneMin           = vec4(m.x, m.y, m.z, 0.0f);
		vec3 sceneExtent3D   = sceneAABB.getMax() - sceneAABB.getMin();
		m_sceneExtent        = vec4(sceneExtent3D.x, sceneExtent3D.y, sceneExtent3D.z, 1.0f);
		float sceneExtentMax = max(sceneExtent3D.x, max(sceneExtent3D.y, sceneExtent3D.z));
		m_sceneExtent        = vec4(sceneExtentMax, sceneExtentMax, sceneExtentMax, 1.0f);
	}

public:
	/** Loads the shader to be used. This could be substituted
	* by reading from a file, where the shader details would be present
	* @return true if the shader was loaded successfully, and false otherwise */
	bool loadShader()
	{
		size_t sizeTemp;

		void* vertShaderCode     = InputOutput::readFile("../data/vulkanshaders/voxelrenderinscenario.vert", &sizeTemp);
		void* geometryShaderCode = InputOutput::readFile("../data/vulkanshaders/voxelrenderinscenario.geom", &sizeTemp);
		void* fragShaderCode     = InputOutput::readFile("../data/vulkanshaders/voxelrenderinscenario.frag", &sizeTemp);

		m_shaderResourceName = "voxelrenderinscenario";
		m_shader             = shaderM->buildShaderVGF(move(string(m_shaderResourceName)), (const char*)vertShaderCode, (const char*)geometryShaderCode, (const char*)fragShaderCode, m_materialSurfaceType);

		return true;
	}

	/** Exposes the variables that will be modified in the corresponding material data uniform buffer used in the scene,
	* allowing to work from variables from C++ without needing to update the values manually (error-prone)
	* @return nothing */
	void exposeResources()
	{
		assignShaderStorageBuffer(move(string("voxelrasterinscenariodebugbuffer")),   move(string("voxelrasterinscenariodebugbuffer")),   VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);
		assignShaderStorageBuffer(move(string("voxelHashedPositionCompactedBuffer")), move(string("voxelHashedPositionCompactedBuffer")), VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);
		assignShaderStorageBuffer(move(string("IndirectionIndexBuffer")),             move(string("IndirectionIndexBuffer")),             VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);
		assignShaderStorageBuffer(move(string("IndirectionRankBuffer")),              move(string("IndirectionRankBuffer")),              VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);
		assignShaderStorageBuffer(move(string("litTestVoxelPerByteBuffer")),          move(string("litTestVoxelPerByteBuffer")),          VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);
		assignShaderStorageBuffer(move(string("litTestDynamicVoxelPerByteBuffer")),   move(string("litTestDynamicVoxelPerByteBuffer")),   VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);

		assignImageToSampler(move(string("voxelizationReflectance")),               move(string("voxelizationReflectance")),               VK_DESCRIPTOR_TYPE_STORAGE_IMAGE);
		assignImageToSampler(move(string("dynamicVoxelizationReflectanceTexture")), move(string("dynamicVoxelizationReflectanceTexture")), VK_DESCRIPTOR_TYPE_STORAGE_IMAGE);

		exposeStructField(ResourceInternalType::RIT_FLOAT_MAT4,   (void*)(&m_viewProjection),   move(string("myMaterialData")), move(string("viewProjection")));
		exposeStructField(ResourceInternalType::RIT_FLOAT_VEC4,   (void*)(&m_voxelizationSize), move(string("myMaterialData")), move(string("voxelizationSize")));
		exposeStructField(ResourceInternalType::RIT_FLOAT_VEC4,   (void*)(&m_sceneMin),         move(string("myMaterialData")), move(string("sceneMin")));
		exposeStructField(ResourceInternalType::RIT_FLOAT_VEC4,   (void*)(&m_sceneExtent),      move(string("myMaterialData")), move(string("sceneExtent")));
		exposeStructField(ResourceInternalType::RIT_UNSIGNED_INT, (void*)(&m_numOcupiedVoxel),  move(string("myMaterialData")), move(string("numOcupiedVoxel")));
		exposeStructField(ResourceInternalType::RIT_UNSIGNED_INT, (void*)(&m_padding0),         move(string("myMaterialData")), move(string("padding0")));
		exposeStructField(ResourceInternalType::RIT_UNSIGNED_INT, (void*)(&m_padding1),         move(string("myMaterialData")), move(string("padding1")));
		exposeStructField(ResourceInternalType::RIT_UNSIGNED_INT, (void*)(&m_padding2),         move(string("myMaterialData")), move(string("padding2")));

		pushConstantExposeStructField(ResourceInternalType::RIT_INT, (void*)(&m_pushConstantVoxelFromStaticCompacted), move(string("myPushConstant")), move(string("pushConstantVoxelFromStaticCompacted")));
	}

	void setupPipelineData()
	{
		m_pipeline.refPipelineData().setRenderPass(renderPassM->getElement(move(string("scenelightingrenderpass")))->getRenderPass());

		// The VkVertexInputBinding m_vertexInputBindingDescription, stores the rate at which the information will be
		// injected for vertex input.
		m_vertexInputBindingDescription.binding   = 0;
		m_vertexInputBindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
		m_vertexInputBindingDescription.stride    = 4;

		// The VkVertexInputAttribute - Description) structure, store the information that helps in interpreting the data.
		m_vertexInputAttributeDescription[0].binding  = 0;
		m_vertexInputAttributeDescription[0].location = 0;
		m_vertexInputAttributeDescription[0].format   = VK_FORMAT_R32_UINT;
		m_vertexInputAttributeDescription[0].offset   = 0;

		VkPipelineVertexInputStateCreateInfo inputState = m_pipeline.refPipelineData().getVertexInputStateInfo();
		inputState.vertexBindingDescriptionCount   = 1;
		inputState.pVertexBindingDescriptions      = &m_vertexInputBindingDescription;
		inputState.vertexAttributeDescriptionCount = 1;
		inputState.pVertexAttributeDescriptions    = m_vertexInputAttributeDescription;

		m_pipeline.refPipelineData().updateVertexInputStateInfo(inputState);

		VkPipelineInputAssemblyStateCreateInfo inputAssemblyStateCreateInfo = m_pipeline.refPipelineData().getInputAssemblyInfo();
		inputAssemblyStateCreateInfo.topology = VK_PRIMITIVE_TOPOLOGY_POINT_LIST;

		m_pipeline.refPipelineData().updateinputAssemblyStateCreateInfo(inputAssemblyStateCreateInfo);

		vector<VkPipelineColorBlendAttachmentState> arrayColorBlendAttachmentStateInfo(1);
		forI(1)
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

		m_pipeline.refPipelineData().setRenderPass(renderPassM->getElement(move(string("voxelrasterinscenariorenderpass")))->getRenderPass());
	}

	GET_SET(mat4, m_viewProjection, ViewProjection)
	GET_SET(vec4, m_voxelizationSize, VoxelizationSize)
	GET_SET(vec4, m_sceneMin, SceneMin)
	GET_SET(vec4, m_sceneExtent, SceneExtent)
	SET(uint, m_numOcupiedVoxel, NumOcupiedVoxel)
	SET(int, m_pushConstantVoxelFromStaticCompacted, PushConstantVoxelFromStaticCompacted)

protected:
	mat4                              m_viewProjection;                       //!< Matrix to transform each cube's center coordinates in the geometry shader
	vec4                              m_voxelizationSize;                     //!< Voxelizatoin size
	vec4                              m_sceneMin;                             //!< Scene minimum value
	vec4                              m_sceneExtent;                          //!< Extent of the scene. Also, w value is used to distinguish between rasterization of reflectance or normal information
	uint                              m_numOcupiedVoxel;                      //!< Number of occupied voxel
	uint                              m_padding0;                             //!< Padding value to achieve 4 32-bit data in the material in the shader
	uint                              m_padding1;                             //!< Padding value to achieve 4 32-bit data in the material in the shader
	uint                              m_padding2;                             //!< Padding value to achieve 4 32-bit data in the material in the shader
	VkVertexInputBindingDescription   m_vertexInputBindingDescription;        //!< This material uses a different vertex input format
	VkVertexInputAttributeDescription m_vertexInputAttributeDescription[1];   //!< This material uses a different vertex input format
	int                               m_pushConstantVoxelFromStaticCompacted; //!< Push constant used to know whether the voxel information needs to be taken from the voxelHashedPositionCompactedBuffer buffer or the voxelOccupiedDynamicBuffer buffer
};

/////////////////////////////////////////////////////////////////////////////////////////////

#endif _MATERIALVOXELRASTERINSCENARIO_H_
