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
#include "../../include/rastertechnique/antialiasingtechnique.h"
#include "../../include/material/materialmanager.h"
#include "../../include/material/material.h"
#include "../../include/util/vulkanstructinitializer.h"
#include "../../include/parameter/attributedefines.h"
#include "../../include/parameter/attributedata.h"
#include "../../include/renderpass/renderpassmanager.h"
#include "../../include/renderpass/renderpass.h"
#include "../../include/material/materialantialiasing.h"
#include "../../include/uniformbuffer/uniformbuffer.h"

// NAMESPACE
using namespace attributedefines;

// DEFINES

// STATIC MEMBER INITIALIZATION

/////////////////////////////////////////////////////////////////////////////////////////////

AntialiasingTechnique::AntialiasingTechnique(string &&name, string&& className) : RasterTechnique(move(name), move(className))
	, m_renderPass(nullptr)
	, m_material(nullptr)
{
	m_isLastPipelineTechnique = true;
	m_usedCommandBufferNumber = 3;
}

/////////////////////////////////////////////////////////////////////////////////////////////

AntialiasingTechnique::~AntialiasingTechnique()
{

}

/////////////////////////////////////////////////////////////////////////////////////////////

void AntialiasingTechnique::init()
{
	VkPipelineBindPoint* pipelineBindPoint                         = new VkPipelineBindPoint(VK_PIPELINE_BIND_POINT_GRAPHICS);
	vector<VkFormat>* vectorAttachmentFormat                       = new vector<VkFormat>;
	vector<VkSampleCountFlagBits>* vectorAttachmentSamplesPerPixel = new vector<VkSampleCountFlagBits>;
	vector<VkImageLayout>* vectorAttachmentFinalLayout             = new vector<VkImageLayout>;
	vector<VkAttachmentReference>* vectorColorReference            = new vector<VkAttachmentReference>;
	vectorAttachmentFormat->push_back(VK_FORMAT_R8G8B8A8_UNORM);
	vectorAttachmentSamplesPerPixel->push_back(VK_SAMPLE_COUNT_1_BIT);
	vectorAttachmentFinalLayout->push_back(VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);
	vectorColorReference->push_back({ 0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL });
	MultiTypeUnorderedMap *attributeUM = new MultiTypeUnorderedMap();
	attributeUM->newElement<AttributeData<VkPipelineBindPoint*>*>          (new AttributeData<VkPipelineBindPoint*>          (string(g_renderPassAttachmentPipelineBindPoint), move(pipelineBindPoint)));
	attributeUM->newElement<AttributeData<vector<VkFormat>*>*>             (new AttributeData<vector<VkFormat>*>             (string(g_renderPassAttachmentFormat),            move(vectorAttachmentFormat)));
	attributeUM->newElement<AttributeData<vector<VkSampleCountFlagBits>*>*>(new AttributeData<vector<VkSampleCountFlagBits>*>(string(g_renderPassAttachmentSamplesPerPixel),   move(vectorAttachmentSamplesPerPixel)));
	attributeUM->newElement<AttributeData<vector<VkImageLayout>*>*>        (new AttributeData<vector<VkImageLayout>*>        (string(g_renderPassAttachmentFinalLayout),       move(vectorAttachmentFinalLayout)));
	attributeUM->newElement<AttributeData<vector<VkAttachmentReference>*>*>(new AttributeData<vector<VkAttachmentReference>*>(string(g_renderPassAttachmentColorReference),    move(vectorColorReference)));
	m_renderPass = renderPassM->buildRenderPass(move(string("antialiasingrenderpass")), attributeUM);

	m_material = materialM->buildMaterial(move(string("MaterialAntialiasing")), move(string("MaterialAntialiasing")), nullptr);
	MaterialAntialiasing* castedMaterial = static_cast<MaterialAntialiasing*>(m_material);
	castedMaterial->setRenderTargetSize(vec4(coreM->getWidth(), coreM->getHeight(), 0.0f, 0.0f));

	m_vectorMaterialName.push_back("MaterialAntialiasing");
	m_vectorMaterial.push_back(m_material);
}

/////////////////////////////////////////////////////////////////////////////////////////////

VkCommandBuffer* AntialiasingTechnique::record(int currentImage, uint& commandBufferID, CommandBufferType& commandBufferType)
{
	commandBufferType = CommandBufferType::CBT_GRAPHICS_QUEUE;

	VkCommandBuffer* commandBuffer;
	commandBuffer = addRecordedCommandBuffer(commandBufferID);
	addCommandBufferQueueType(commandBufferID, commandBufferType);
	coreM->allocCommandBuffer(&coreM->getLogicalDevice(), coreM->getGraphicsCommandPool(), commandBuffer);
	coreM->setObjectName(uint64_t(*commandBuffer), VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT, getName());

	coreM->beginCommandBuffer(*commandBuffer);

	Texture* renderTarget = textureM->getElement(move(string("scenelightingcolor")));

	VkImageSubresourceRange subresourceRange = {};
	subresourceRange.aspectMask   = VK_IMAGE_ASPECT_COLOR_BIT;
	subresourceRange.baseMipLevel = 0;
	subresourceRange.levelCount   = 1;
	subresourceRange.layerCount   = 1;

	textureM->setImageLayout(*commandBuffer, renderTarget, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, subresourceRange, coreM->getGraphicsQueueIndex());

#ifdef USE_TIMESTAMP
	vkCmdResetQueryPool(*commandBuffer, coreM->getGraphicsQueueQueryPool(), m_queryIndex0, 1);
	vkCmdResetQueryPool(*commandBuffer, coreM->getGraphicsQueueQueryPool(), m_queryIndex1, 1);
	vkCmdWriteTimestamp(*commandBuffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, coreM->getGraphicsQueueQueryPool(), m_queryIndex0);
#endif

	VkRenderPassBeginInfo renderPassBeginInfo = VulkanStructInitializer::renderPassBeginInfo(
		coreM->refRenderPass(),
		coreM->getArrayFramebuffers()[currentImage]->getFramebuffer(),
		VkRect2D({ 0, 0, uint32_t(coreM->getWidth()), uint32_t(coreM->getHeight()) }),
		m_material->refVectorClearValue());

	vkCmdBeginRenderPass(*commandBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

	gpuPipelineM->initViewports(float(coreM->getWidth()), float(coreM->getHeight()), 0.0f, 0.0f, 0.0f, 1.0f, commandBuffer);
	gpuPipelineM->initScissors(coreM->getWidth(), coreM->getHeight(), 0, 0, commandBuffer);

	uint dynamicAllignment = materialM->getMaterialUBDynamicAllignment();

	uint32_t offsetData = static_cast<uint32_t>(m_material->getMaterialUniformBufferIndex() * dynamicAllignment);
	vkCmdBindDescriptorSets(*commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_material->getPipelineLayout(), 0, 1, &m_material->refDescriptorSet(), 1, &offsetData);
	vkCmdBindPipeline(*commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_material->getPipeline()->getPipeline());
	vkCmdDraw(*commandBuffer, 3, 1, 0, 0);

	vkCmdEndRenderPass(*commandBuffer);

#ifdef USE_TIMESTAMP
	vkCmdWriteTimestamp(*commandBuffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, coreM->getGraphicsQueueQueryPool(), m_queryIndex1);
#endif

	textureM->setImageLayout(*commandBuffer, renderTarget, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, subresourceRange, coreM->getGraphicsQueueIndex());

	coreM->endCommandBuffer(*commandBuffer);

	// NOTE: Clear command buffer if re-recorded
	m_vectorCommand.push_back(commandBuffer);

	return commandBuffer;
}

/////////////////////////////////////////////////////////////////////////////////////////////

void AntialiasingTechnique::postCommandSubmit()
{
	m_executeCommand = false;
	m_needsToRecord  = (m_vectorCommand.size() != m_usedCommandBufferNumber);
}

/////////////////////////////////////////////////////////////////////////////////////////////
