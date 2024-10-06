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
#include "../../include/rastertechnique/clear3dtexturestechnique.h"
#include "../../include/core/coremanager.h"
#include "../../include/texture/texture.h"
#include "../../include/texture/texturemanager.h"
#include "../../include/parameter/attributedefines.h"
#include "../../include/parameter/attributedata.h"
#include "../../include/renderpass/renderpass.h"
#include "../../include/renderpass/renderpassmanager.h"
#include "../../include/framebuffer/framebuffer.h"
#include "../../include/framebuffer/framebuffermanager.h"
#include "../../include/util/vulkanstructinitializer.h"
#include "../../include/rastertechnique/dynamicvoxelvisibilityraytracingtechnique.h"

// NAMESPACE
using namespace attributedefines;

// DEFINES

// STATIC MEMBER INITIALIZATION

/////////////////////////////////////////////////////////////////////////////////////////////

Clear3DTexturesTechnique::Clear3DTexturesTechnique(string &&name, string&& className) : RasterTechnique(move(name), move(className))
	, m_clear3DTexturesRenderPass(nullptr)
	, m_clear3DTexturesFramebufferStatic(nullptr)
	, m_dynamicVoxelVisibilityRayTracingTechnique(nullptr)
{
	m_active                      = false;
	m_executeCommand              = false;
	m_sceneVoxelizationResolution = gpuPipelineM->getRasterFlagValue(move(string("SCENE_VOXELIZATION_RESOLUTION")));

	m_vectorTextureStatic.resize(6);
	m_vectorTextureDynamic.resize(6);
}

/////////////////////////////////////////////////////////////////////////////////////////////

void Clear3DTexturesTechnique::init()
{
	// render pass to clear the 3D texture for the dynamic scene voxelization technique
	VkPipelineBindPoint* pipelineBindPoint                         = new VkPipelineBindPoint(VK_PIPELINE_BIND_POINT_GRAPHICS);
	vector<VkFormat>* vectorAttachmentFormat                       = new vector<VkFormat>;
	vector<VkSampleCountFlagBits>* vectorAttachmentSamplesPerPixel = new vector<VkSampleCountFlagBits>;
	vector<VkImageLayout>* vectorAttachmentFinalLayout             = new vector<VkImageLayout>;
	vector<VkAttachmentReference>* vectorColorReference            = new vector<VkAttachmentReference>;

	forI(m_vectorTextureStatic.size())
	{
		vectorAttachmentFormat->push_back(VK_FORMAT_B10G11R11_UFLOAT_PACK32);
		vectorAttachmentSamplesPerPixel->push_back(VK_SAMPLE_COUNT_1_BIT);
		vectorAttachmentFinalLayout->push_back(VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
		vectorColorReference->push_back({ i, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL });
	}

	MultiTypeUnorderedMap* attributeUM = new MultiTypeUnorderedMap();
	attributeUM->newElement<AttributeData<VkPipelineBindPoint*>*>(new AttributeData<VkPipelineBindPoint*>(string(g_renderPassAttachmentPipelineBindPoint), move(pipelineBindPoint)));
	attributeUM->newElement<AttributeData<vector<VkFormat>*>*>(new AttributeData<vector<VkFormat>*>(string(g_renderPassAttachmentFormat), move(vectorAttachmentFormat)));
	attributeUM->newElement<AttributeData<vector<VkSampleCountFlagBits>*>*>(new AttributeData<vector<VkSampleCountFlagBits>*>(string(g_renderPassAttachmentSamplesPerPixel), move(vectorAttachmentSamplesPerPixel)));
	attributeUM->newElement<AttributeData<vector<VkImageLayout>*>*>(new AttributeData<vector<VkImageLayout>*>(string(g_renderPassAttachmentFinalLayout), move(vectorAttachmentFinalLayout)));
	attributeUM->newElement<AttributeData<vector<VkAttachmentReference>*>*>(new AttributeData<vector<VkAttachmentReference>*>(string(g_renderPassAttachmentColorReference), move(vectorColorReference)));

	m_clear3DTexturesRenderPass = renderPassM->buildRenderPass(move(string("clear3dtexturesrenderPass")), attributeUM);

	vector<string> arrayAttachment;
	forI(m_vectorTextureStatic.size())
	{
		arrayAttachment.push_back(m_vectorTextureStatic[i]->getName());
	}
	m_clear3DTexturesFramebufferStatic = framebufferM->buildFramebuffer(move(string("clear3dtexturesframebufferstatic")), (uint32_t)(m_sceneVoxelizationResolution), (uint32_t)(m_sceneVoxelizationResolution), move(string(m_clear3DTexturesRenderPass->getName())), move(arrayAttachment));

	arrayAttachment.clear();
	forI(m_vectorTextureDynamic.size())
	{
		arrayAttachment.push_back(m_vectorTextureDynamic[i]->getName());
	}
	m_clear3DTexturesFramebufferDynamic = framebufferM->buildFramebuffer(move(string("clear3dtexturesframebufferdynamic")), (uint32_t)(m_sceneVoxelizationResolution), (uint32_t)(m_sceneVoxelizationResolution), move(string(m_clear3DTexturesRenderPass->getName())), move(arrayAttachment));

	m_dynamicVoxelVisibilityRayTracingTechnique = static_cast<DynamicVoxelVisibilityRayTracingTechnique*>(gpuPipelineM->getRasterTechniqueByName(move(string("DynamicVoxelVisibilityRayTracingTechnique"))));
	m_dynamicVoxelVisibilityRayTracingTechnique->refSignalDynamicVoxelVisibilityRayTracing().connect<Clear3DTexturesTechnique, &Clear3DTexturesTechnique::slotDynamicVoxelVisibilityRayTracingCompleted>(this);
}

/////////////////////////////////////////////////////////////////////////////////////////////

VkCommandBuffer* Clear3DTexturesTechnique::record(int currentImage, uint& commandBufferID, CommandBufferType& commandBufferType)
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

	VkImageSubresourceRange subresourceRange = {};
	subresourceRange.aspectMask   = VK_IMAGE_ASPECT_COLOR_BIT;
	subresourceRange.baseMipLevel = 0;
	subresourceRange.levelCount   = 1;
	subresourceRange.layerCount   = 1;

	VkClearValue clearValue;
	clearValue.color.float32[0] = 0.0f;
	clearValue.color.float32[1] = 0.0f;
	clearValue.color.float32[2] = 0.0f;
	clearValue.color.float32[3] = 0.0f;

	vector<VkClearValue> vectorClearValue = { clearValue, clearValue, clearValue, clearValue, clearValue, clearValue };

	VkRenderPassBeginInfo renderPassBegin = VulkanStructInitializer::renderPassBeginInfo(
		*m_clear3DTexturesRenderPass->refRenderPass(),
		 m_clear3DTexturesFramebufferDynamic->getFramebuffer(),
		 VkRect2D({ 0, 0, uint32_t(m_sceneVoxelizationResolution), uint32_t(m_sceneVoxelizationResolution) }),
		 vectorClearValue);

#ifdef USE_TIMESTAMP
	vkCmdWriteTimestamp(*commandBuffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, coreM->getGraphicsQueueQueryPool(), m_queryIndex1);
#endif

	coreM->endCommandBuffer(*commandBuffer);

	// NOTE: Clear command buffer if re-recorded
	m_vectorCommand.push_back(commandBuffer);

	return commandBuffer;
}

/////////////////////////////////////////////////////////////////////////////////////////////

void Clear3DTexturesTechnique::postCommandSubmit()
{
	m_signalClear3DTexturesCompletion.emit();

	m_executeCommand = false;
	m_active         = false;
	m_needsToRecord = (m_vectorCommand.size() != m_usedCommandBufferNumber);
}

/////////////////////////////////////////////////////////////////////////////////////////////

void Clear3DTexturesTechnique::slotDynamicVoxelVisibilityRayTracingCompleted()
{
	m_active = true;
}

/////////////////////////////////////////////////////////////////////////////////////////////
