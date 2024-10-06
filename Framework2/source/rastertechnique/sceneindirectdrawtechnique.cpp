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

// GLOBAL INCLUDES

// PROJECT INCLUDES
#include "../../include/rastertechnique/sceneindirectdrawtechnique.h"
#include "../../include/material/materialmanager.h"
#include "../../include/core/coremanager.h"
#include "../../include/scene/scene.h"
#include "../../include/util/loopmacrodefines.h"
#include "../../include/buffer/buffer.h"
#include "../../include/buffer/buffermanager.h"
#include "../../include/core/gpupipeline.h"
#include "../../include/uniformbuffer/uniformbuffer.h"
#include "../../include/util/vulkanstructinitializer.h"
#include "../../include/renderpass/renderpass.h"
#include "../../include/renderpass/renderpassmanager.h"
#include "../../include/framebuffer/framebuffer.h"
#include "../../include/framebuffer/framebuffermanager.h"
#include "../../include/material/materialindirectcolortexture.h"
#include "../../include/component/basecomponent.h"
#include "../../include/component/rendercomponent.h"

// NAMESPACE

// DEFINES

// STATIC MEMBER INITIALIZATION

/////////////////////////////////////////////////////////////////////////////////////////////

SceneIndirectDrawTechnique::SceneIndirectDrawTechnique(string &&name, string&& className) : RasterTechnique(move(name), move(className))
	, m_renderPass(nullptr)
	, m_framebuffer(nullptr)
{
}

/////////////////////////////////////////////////////////////////////////////////////////////

SceneIndirectDrawTechnique::~SceneIndirectDrawTechnique()
{

}

/////////////////////////////////////////////////////////////////////////////////////////////

void SceneIndirectDrawTechnique::init()
{
	m_arrayNode            = sceneM->getByNodeType(eNodeType::E_NT_STATIC_ELEMENT | eNodeType::E_NT_DYNAMIC_ELEMENT);
	m_arrayNodeSkinnedMesh = sceneM->getByNodeType(eNodeType::E_NT_SKINNEDMESH_ELEMENT);

	m_indirectCommandBufferMainCamera = bufferM->getElement(move(string("indirectCommandBufferMainCamera")));

	m_renderPass = renderPassM->getElement(move(string("scenelightingrenderpass")));
	m_framebuffer = framebufferM->getElement(move(string("scenelightingrenderpassFB")));

	m_material = static_cast<MaterialIndirectColorTexture*>(materialM->getElement(move(string("DefaultMaterialInstanced"))));
	m_material->setColor(vec3(1.0f, 1.0f, 1.0f));
	m_vectorMaterialName.push_back("DefaultMaterialInstanced");
	m_vectorMaterial.push_back(m_material);

	inputM->refEventSinglePressSignalSlot().addKeyDownSignal(KeyCode::KEY_CODE_L);
	SignalVoid* signalAdd = inputM->refEventSinglePressSignalSlot().refKeyDownSignalByKey(KeyCode::KEY_CODE_L);
	signalAdd->connect<SceneIndirectDrawTechnique, &SceneIndirectDrawTechnique::slotLKeyPressed>(this);

	inputM->refEventSinglePressSignalSlot().addKeyDownSignal(KeyCode::KEY_CODE_R);
	signalAdd = inputM->refEventSinglePressSignalSlot().refKeyDownSignalByKey(KeyCode::KEY_CODE_R);
	signalAdd->connect<SceneIndirectDrawTechnique, &SceneIndirectDrawTechnique::slotLKeyPressed>(this);

	inputM->refEventSinglePressSignalSlot().addKeyDownSignal(KeyCode::KEY_CODE_N);
	signalAdd = inputM->refEventSinglePressSignalSlot().refKeyDownSignalByKey(KeyCode::KEY_CODE_N);
	signalAdd->connect<SceneIndirectDrawTechnique, &SceneIndirectDrawTechnique::slotLKeyPressed>(this);

	inputM->refEventSinglePressSignalSlot().addKeyDownSignal(KeyCode::KEY_CODE_I);
	signalAdd = inputM->refEventSinglePressSignalSlot().refKeyDownSignalByKey(KeyCode::KEY_CODE_I);
	signalAdd->connect<SceneIndirectDrawTechnique, &SceneIndirectDrawTechnique::slotLKeyPressed>(this);
}

/////////////////////////////////////////////////////////////////////////////////////////////

VkCommandBuffer* SceneIndirectDrawTechnique::record(int currentImage, uint& commandBufferID, CommandBufferType& commandBufferType)
{
	// TODO: Refactor with SceneLightingTechnique
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

	if(m_arrayNodeSkinnedMesh.size() > 0)
	{
		VulkanStructInitializer::insertBufferMemoryBarrier(bufferM->getElement(move(string("vertexBufferSkinnedMesh"))),
														   VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT,
														   VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT,
														   VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
														   VK_PIPELINE_STAGE_VERTEX_SHADER_BIT,
														   commandBuffer);
	}

	VkRenderPassBeginInfo renderPassBegin = VulkanStructInitializer::renderPassBeginInfo(
		*m_renderPass->refRenderPass(),
		m_framebuffer->getFramebuffer(),
		VkRect2D({ 0, 0, coreM->getWidth(), coreM->getHeight() }),
		m_material->refVectorClearValue());

	vkCmdBeginRenderPass(*commandBuffer, &renderPassBegin, VK_SUBPASS_CONTENTS_INLINE); // Start recording the render pass instance

	const VkDeviceSize offsets[1] = { 0 };
	Buffer* instanceDataBuffer = bufferM->getElement(move(string("instanceDataBuffer")));
	vkCmdBindVertexBuffers(*commandBuffer, 0, 1, &bufferM->getElement(move(string("vertexBuffer")))->getBuffer(), offsets);
	vkCmdBindVertexBuffers(*commandBuffer, 1, 1, &instanceDataBuffer->getBuffer(), offsets);
	vkCmdBindIndexBuffer(*commandBuffer, bufferM->getElement(move(string("indexBuffer")))->getBuffer(), 0, VK_INDEX_TYPE_UINT32);

	gpuPipelineM->initViewports((float)coreM->getWidth(), (float)coreM->getHeight(), 0.0f, 0.0f, 0.0f, 1.0f, commandBuffer);
	gpuPipelineM->initScissors(coreM->getWidth(), coreM->getHeight(), 0, 0, commandBuffer);

	drawIndirectNodeVector(*commandBuffer, m_arrayNode);

	if (m_arrayNodeSkinnedMesh.size() > 0)
	{
		vkCmdBindVertexBuffers(*commandBuffer, 0, 1, &bufferM->getElement(move(string("vertexBufferSkinnedMesh")))->getBuffer(), offsets);
		vkCmdBindVertexBuffers(*commandBuffer, 1, 1, &instanceDataBuffer->getBuffer(), offsets);
		vkCmdBindIndexBuffer(*commandBuffer, bufferM->getElement(move(string("indexBufferSkinnedMesh")))->getBuffer(), 0, VK_INDEX_TYPE_UINT32);
		drawIndirectNodeVector(*commandBuffer, m_arrayNodeSkinnedMesh);
	}

	vkCmdEndRenderPass(*commandBuffer);
	
#ifdef USE_TIMESTAMP
	vkCmdWriteTimestamp(*commandBuffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, coreM->getGraphicsQueueQueryPool(), m_queryIndex1);
#endif

	coreM->endCommandBuffer(*commandBuffer);

	m_vectorCommand.push_back(commandBuffer);

	return commandBuffer;
}

/////////////////////////////////////////////////////////////////////////////////////////////

void SceneIndirectDrawTechnique::postCommandSubmit()
{
	m_executeCommand = false;
	m_needsToRecord  = (m_vectorCommand.size() != m_usedCommandBufferNumber);
}

/////////////////////////////////////////////////////////////////////////////////////////////

void SceneIndirectDrawTechnique::slotLKeyPressed()
{
	m_active = !m_active;
}

/////////////////////////////////////////////////////////////////////////////////////////////

void SceneIndirectDrawTechnique::drawIndirectNodeVector(VkCommandBuffer commandBuffer, const vectorNodePtr& vectorNode)
{
	uint dynamicAllignment = materialM->getMaterialUBDynamicAllignment();

	forI(vectorNode.size())
	{
		RenderComponent* renderComponent = vectorNode[i]->refRenderComponent();

		vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, renderComponent->refMaterial()->getPipeline()->getPipeline()); // Bound the command buffer with the graphics pipeline

		uint32_t elementIndex;
		uint32_t offsetData[3];
		uint32_t sceneDataBufferOffset = static_cast<uint32_t>(gpuPipelineM->getSceneUniformData()->getDynamicAllignment());

		elementIndex  = sceneM->getElementIndex(vectorNode[i]);
		offsetData[0] = elementIndex * sceneDataBufferOffset;
		offsetData[1] = 0;
		offsetData[2] = static_cast<uint32_t>(renderComponent->refMaterial()->getMaterialUniformBufferIndex() * dynamicAllignment);

		vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, renderComponent->refMaterial()->getPipelineLayout(), 0, 1, &renderComponent->refMaterial()->refDescriptorSet(), 3, &offsetData[0]);
		vkCmdDrawIndexedIndirect(commandBuffer, m_indirectCommandBufferMainCamera->getBuffer(), sceneM->getElementIndex(vectorNode[i]) * sizeof(VkDrawIndexedIndirectCommand), 1, sizeof(VkDrawIndexedIndirectCommand));
	}
}

/////////////////////////////////////////////////////////////////////////////////////////////
