/*
Copyright 2021 Alejandro Cosin

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
#include "../../include/rastertechnique/sceneraytracingtechnique.h"
#include "../../include/scene/scene.h"
#include "../../include/core/coremanager.h"
#include "../../include/accelerationstructure/accelerationstructure.h"
#include "../../include/accelerationstructure/accelerationstructuremanager.h"
#include "../../include/material/materialmanager.h"
#include "../../include/material/materialsceneraytrace.h"
#include "../../include/parameter/attributedefines.h"
#include "../../include/parameter/attributedata.h"
#include "../../include/node/node.h"
#include "../../include/texture/texturemanager.h"
#include "../../include/texture/texture.h"

// NAMESPACE
using namespace attributedefines;

// DEFINES
// TODO: Unify definition, it is replicated in several places
struct SceneDescription
{
	int objId;
	uint flags;
	mat4 transform;
	mat4 transformIT;
};

// STATIC MEMBER INITIALIZATION

/////////////////////////////////////////////////////////////////////////////////////////////

SceneRayTracingTechnique::SceneRayTracingTechnique(string &&name, string&& className) :
	  RasterTechnique(move(name), move(className))
	, m_materialSceneRayTrace(nullptr)
	, m_staticAccelerationStructure(nullptr)
	, m_rayTracingOffscreen(nullptr)
	, m_sceneDescriptorBuffer(nullptr)
	, m_shaderBindingTableBuffer(nullptr)
	, m_shaderBindingTableBuilt(false)
	, m_shaderGroupBaseAlignment(0)
	, m_shaderGroupHandleSize(0)
	, m_shaderGroupSizeAligned(0)
	, m_shaderGroupStride(0)
{
	m_active = true;
}

/////////////////////////////////////////////////////////////////////////////////////////////

SceneRayTracingTechnique::~SceneRayTracingTechnique()
{

}

/////////////////////////////////////////////////////////////////////////////////////////////

void SceneRayTracingTechnique::init()
{
	m_rayTracingOffscreen = textureM->buildTexture(move(string("raytracingoffscreen")),
												   VK_FORMAT_R16G16B16A16_SFLOAT,
												   { uint32_t(coreM->getWidth()), uint32_t(coreM->getHeight()), 1 },
												   VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
												   VK_IMAGE_ASPECT_COLOR_BIT,
												   VK_IMAGE_ASPECT_COLOR_BIT,
												   VK_IMAGE_LAYOUT_UNDEFINED,
												   VK_IMAGE_LAYOUT_GENERAL,
												   VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
												   VK_SAMPLE_COUNT_1_BIT,
												   VK_IMAGE_TILING_OPTIMAL,
												   VK_IMAGE_VIEW_TYPE_2D,
												   0);

	vectorNodePtr vectorNode = sceneM->getByMeshType(eMeshType::E_MT_RENDER_MODEL);

	vector<SceneDescription> vectorSceneDescription(vectorNode.size());

	forI(vectorNode.size())
	{
		vectorSceneDescription[i].objId       = i;
		vectorSceneDescription[i].flags       = (vectorNode[i]->getNodeType() == eNodeType::E_NT_STATIC_ELEMENT) ? 0 : 1;
		vectorSceneDescription[i].transform   = vectorNode[i]->getModelMat();
		vectorSceneDescription[i].transformIT = glm::inverse(vectorNode[i]->getModelMat());
	}

	m_sceneDescriptorBuffer = bufferM->buildBuffer(move(string("sceneDescriptionBufferCamera")),
		(void*)vectorSceneDescription.data(),
		sizeof(SceneDescription) * vectorSceneDescription.size(),
		VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

	m_staticAccelerationStructure = accelerationStructureM->buildAccelerationStructure(move(string("raytracedaccelerationstructurecamera")), move(vectorNodePtr(vectorNode)));

	MultiTypeUnorderedMap* attribute = new MultiTypeUnorderedMap();
	attribute->newElement<AttributeData<vector<Node*>>*>(new AttributeData<vector<Node*>>(string(g_rayTraceVectorSceneNode), move(vectorNode)));
	m_materialSceneRayTrace = static_cast<MaterialSceneRayTrace*>(materialM->buildMaterial(move(string("MaterialSceneRayTrace")), move(string("MaterialSceneRayTrace")), attribute));

	const VkPhysicalDeviceRayTracingPipelinePropertiesKHR& physicalDeviceRayTracingPipelineProperties = coreM->getkPhysicalDeviceRayTracingPipelineProperties();

	m_shaderGroupBaseAlignment = physicalDeviceRayTracingPipelineProperties.shaderGroupBaseAlignment;
	m_shaderGroupHandleSize    = physicalDeviceRayTracingPipelineProperties.shaderGroupHandleSize;
	m_shaderGroupSizeAligned   = uint32_t((m_shaderGroupHandleSize + (m_shaderGroupBaseAlignment - 1)) & ~uint32_t(m_shaderGroupBaseAlignment - 1));
	m_shaderGroupStride        = m_shaderGroupSizeAligned;
}

/////////////////////////////////////////////////////////////////////////////////////////////

void SceneRayTracingTechnique::prepare(float dt)
{
	if (!m_shaderBindingTableBuilt)
	{
		buildShaderBindingTable();
		m_shaderBindingTableBuilt = true;
	}
}

/////////////////////////////////////////////////////////////////////////////////////////////

VkCommandBuffer* SceneRayTracingTechnique::record(int currentImage, uint& commandBufferID, CommandBufferType& commandBufferType)
{
	commandBufferType = CommandBufferType::CBT_GRAPHICS_QUEUE;

	VkCommandBuffer* commandBuffer;
	commandBuffer = addRecordedCommandBuffer(commandBufferID);
	addCommandBufferQueueType(commandBufferID, commandBufferType);
	coreM->allocCommandBuffer(&coreM->getLogicalDevice(), coreM->getGraphicsCommandPool(), commandBuffer);
	coreM->setObjectName(uint64_t(*commandBuffer), VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT, getName());

	coreM->beginCommandBuffer(*commandBuffer);

#ifdef USE_TIMESTAMP
	vkCmdResetQueryPool(*commandBuffer, coreM->getGraphicsQueueQueryPool(), m_queryIndex0, 1);
	vkCmdResetQueryPool(*commandBuffer, coreM->getGraphicsQueueQueryPool(), m_queryIndex1, 1);
	vkCmdWriteTimestamp(*commandBuffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, coreM->getGraphicsQueueQueryPool(), m_queryIndex0);
#endif
	
	vkCmdBindPipeline(*commandBuffer, VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR, m_materialSceneRayTrace->getPipeline()->getPipeline());

	uint offsetData = 0;
	vkCmdBindDescriptorSets(*commandBuffer, VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR, m_materialSceneRayTrace->getPipelineLayout(), 0, 1, &m_materialSceneRayTrace->refDescriptorSet(), 1, &offsetData);

	VkDeviceAddress shaderBindingTableDeviceAddress = m_shaderBindingTableBuffer->getBufferDeviceAddress();

	vector<VkStridedDeviceAddressRegionKHR> vectorStridedDeviceAddressRegion(4);
	vectorStridedDeviceAddressRegion[0] = VkStridedDeviceAddressRegionKHR{ shaderBindingTableDeviceAddress + 0u * m_shaderGroupSizeAligned, m_shaderGroupStride, m_shaderGroupSizeAligned * 1 };
	vectorStridedDeviceAddressRegion[1] = VkStridedDeviceAddressRegionKHR{ shaderBindingTableDeviceAddress + 1u * m_shaderGroupSizeAligned, m_shaderGroupStride, m_shaderGroupSizeAligned * 1 };
	vectorStridedDeviceAddressRegion[2] = VkStridedDeviceAddressRegionKHR{ shaderBindingTableDeviceAddress + 2u * m_shaderGroupSizeAligned, m_shaderGroupStride, m_shaderGroupSizeAligned * 1 };
	vectorStridedDeviceAddressRegion[3] = {0, 0, 0};

	vkfpM->vkCmdTraceRaysKHR(*commandBuffer, &vectorStridedDeviceAddressRegion[0], &vectorStridedDeviceAddressRegion[1], &vectorStridedDeviceAddressRegion[2], &vectorStridedDeviceAddressRegion[3], coreM->getWidth(), coreM->getHeight(), 1);
	
#ifdef USE_TIMESTAMP
	vkCmdWriteTimestamp(*commandBuffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, coreM->getGraphicsQueueQueryPool(), m_queryIndex1);
#endif

	coreM->endCommandBuffer(*commandBuffer);

	// NOTE: Clear command buffer if re-recorded
	m_vectorCommand.push_back(commandBuffer);

	return commandBuffer;
}

/////////////////////////////////////////////////////////////////////////////////////////////

void SceneRayTracingTechnique::postCommandSubmit()
{
	m_executeCommand = false;
	m_needsToRecord  = (m_vectorCommand.size() != m_usedCommandBufferNumber);
}

/////////////////////////////////////////////////////////////////////////////////////////////

void SceneRayTracingTechnique::buildShaderBindingTable()
{
	vector<VkRayTracingShaderGroupCreateInfoKHR>& vectorRayTracingShaderGroupCreateInfo = m_materialSceneRayTrace->refShader()->refVectorRayTracingShaderGroupCreateInfo();

	uint numberShaderGroup      = static_cast<uint>(vectorRayTracingShaderGroupCreateInfo.size());
	uint shaderBindingTableSize = numberShaderGroup * m_shaderGroupSizeAligned;

	// Retrieve shader handles
	vector<uint8_t> vectorShaderHandleStorage(shaderBindingTableSize);
	VkResult result = vkfpM->vkGetRayTracingShaderGroupHandlesKHR(coreM->getLogicalDevice(), m_materialSceneRayTrace->getPipeline()->getPipeline(), 0, numberShaderGroup, shaderBindingTableSize, vectorShaderHandleStorage.data());

	assert(result == VK_SUCCESS);

	vector<uint8_t> vectorShaderBindingTableBuffer(shaderBindingTableSize);
	uint8_t* pVectorShaderBindingTableBuffer = static_cast<uint8_t*>(vectorShaderBindingTableBuffer.data());

	forI(numberShaderGroup)
	{
		memcpy(pVectorShaderBindingTableBuffer, vectorShaderHandleStorage.data() + i * m_shaderGroupHandleSize, m_shaderGroupHandleSize);
		pVectorShaderBindingTableBuffer += m_shaderGroupSizeAligned;
	}

	m_shaderBindingTableBuffer = bufferM->buildBuffer(move(string("shaderBindingTable")),
		(void*)vectorShaderBindingTableBuffer.data(),
		sizeof(uint8_t) * vectorShaderBindingTableBuffer.size(),
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_SHADER_BINDING_TABLE_BIT_KHR,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
}

/////////////////////////////////////////////////////////////////////////////////////////////
