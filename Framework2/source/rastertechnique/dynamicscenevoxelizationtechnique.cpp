/*
Copyright 2022 Alejandro Cosin

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
#include "../../include/rastertechnique/dynamicscenevoxelizationtechnique.h"
#include "../../include/material/materialmanager.h"
#include "../../include/shader/shader.h"
#include "../../include/scene/scene.h"
#include "../../include/material/materialscenevoxelization.h"
#include "../../include/parameter/attributedefines.h"
#include "../../include/parameter/attributedata.h"
#include "../../include/framebuffer/framebuffer.h"
#include "../../include/framebuffer/framebuffermanager.h"
#include "../../include/material/materialcolortexture.h"
#include "../../include/util/mathutil.h"
#include "../../include/component/basecomponent.h"
#include "../../include/component/rendercomponent.h"
#include "../../include/util/vulkanstructinitializer.h"
#include "../../include/rastertechnique/resetlitvoxeldatatechnique.h"

// NAMESPACE
using namespace attributedefines;

// DEFINES

// STATIC MEMBER INITIALIZATION

/////////////////////////////////////////////////////////////////////////////////////////////

DynamicSceneVoxelizationTechnique::DynamicSceneVoxelizationTechnique(string &&name, string&& className) : SceneVoxelizationTechnique(move(name), move(className))
	, m_dynamicVoxelizationReflectanceTexture(nullptr)
	, m_dynamicTextureClearRenderPass(nullptr)
	, m_dynamicTextureClearFramebuffer(nullptr)
	, m_dynamicVoxelizationDebugBuffer(nullptr)
	, m_sceneVoxelizationResolution(0)
{
	m_recordPolicy                = CommandRecordPolicy::CRP_EVERY_SWAPCHAIN_IMAGE;
	m_isStaticSceneVoxelization   = false;

	m_active                      = false;
	m_needsToRecord               = false;
	m_executeCommand              = false;
	m_sceneVoxelizationResolution = gpuPipelineM->getRasterFlagValue(move(string("SCENE_VOXELIZATION_RESOLUTION")));
}

/////////////////////////////////////////////////////////////////////////////////////////////

DynamicSceneVoxelizationTechnique::~DynamicSceneVoxelizationTechnique()
{

}

/////////////////////////////////////////////////////////////////////////////////////////////

void DynamicSceneVoxelizationTechnique::init()
{
	m_dynamicVoxelizationReflectanceTexture = textureM->buildTexture(
		move(string("dynamicVoxelizationReflectanceTexture")),
		VK_FORMAT_R32_UINT,
		VkExtent3D({ uint32_t(m_voxelizedSceneWidth), uint32_t(m_voxelizedSceneHeight), uint32_t(m_voxelizedSceneDepth) }),
		VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
		VK_IMAGE_ASPECT_COLOR_BIT,
		VK_IMAGE_ASPECT_COLOR_BIT,
		VK_IMAGE_LAYOUT_UNDEFINED,
		VK_IMAGE_LAYOUT_GENERAL,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		VK_SAMPLE_COUNT_1_BIT,
		VK_IMAGE_TILING_OPTIMAL,
		VK_IMAGE_VIEW_TYPE_3D,
		// The option below is needed to build the image compatible with 2D array so the image view can be built as a 2D array with layers = texture dimension,
		// so it can be used as attachemtn in a framebuffer to be cleared
		VK_IMAGE_CREATE_2D_ARRAY_COMPATIBLE_BIT);

	m_dynamicVoxelizationDebugBuffer = bufferM->buildBuffer(
		move(string("dynamicVoxelizationDebugBuffer")),
		nullptr,
		1024,
		VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

	m_renderPass                = renderPassM->getElement(move(string("voxelizationrenderpass")));
	m_framebuffer               = framebufferM->getElement(move(string("voxelizationoutputFB")));
	m_voxelizationOutputTexture = textureM->getElement(move(string("voxelizationoutputimage")));

	// Generate scene voxelization materials for both static and dynamic scene elements
	SceneVoxelizationTechnique::generateVoxelizationMaterials();

	// render pass to clear the 3D texture for the dynamic scene voxelization technique
	VkPipelineBindPoint* pipelineBindPoint                         = new VkPipelineBindPoint(VK_PIPELINE_BIND_POINT_GRAPHICS);
	vector<VkFormat>* vectorAttachmentFormat                       = new vector<VkFormat>;
	vector<VkSampleCountFlagBits>* vectorAttachmentSamplesPerPixel = new vector<VkSampleCountFlagBits>;
	vector<VkImageLayout>* vectorAttachmentFinalLayout             = new vector<VkImageLayout>;
	vector<VkAttachmentReference>* vectorColorReference            = new vector<VkAttachmentReference>;

	vectorAttachmentFormat->push_back(VK_FORMAT_R32_UINT);
	vectorAttachmentSamplesPerPixel->push_back(VK_SAMPLE_COUNT_1_BIT);
	vectorAttachmentFinalLayout->push_back(VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
	vectorColorReference->push_back({ 0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL });

	MultiTypeUnorderedMap* attributeUM = new MultiTypeUnorderedMap();
	attributeUM->newElement<AttributeData<VkPipelineBindPoint*>*>(          new AttributeData<VkPipelineBindPoint*>(          string(g_renderPassAttachmentPipelineBindPoint), move(pipelineBindPoint)));
	attributeUM->newElement<AttributeData<vector<VkFormat>*>*>(             new AttributeData<vector<VkFormat>*>(             string(g_renderPassAttachmentFormat),            move(vectorAttachmentFormat)));
	attributeUM->newElement<AttributeData<vector<VkSampleCountFlagBits>*>*>(new AttributeData<vector<VkSampleCountFlagBits>*>(string(g_renderPassAttachmentSamplesPerPixel),   move(vectorAttachmentSamplesPerPixel)));
	attributeUM->newElement<AttributeData<vector<VkImageLayout>*>*>(        new AttributeData<vector<VkImageLayout>*>(        string(g_renderPassAttachmentFinalLayout),       move(vectorAttachmentFinalLayout)));
	attributeUM->newElement<AttributeData<vector<VkAttachmentReference>*>*>(new AttributeData<vector<VkAttachmentReference>*>(string(g_renderPassAttachmentColorReference),    move(vectorColorReference)));

	m_dynamicTextureClearRenderPass = renderPassM->buildRenderPass(move(string("dynamictextureclearrenderPass")), attributeUM);

	vector<string> arrayAttachment;
	arrayAttachment.push_back(m_dynamicVoxelizationReflectanceTexture->getName());
	m_dynamicTextureClearFramebuffer = framebufferM->buildFramebuffer(move(string("dynamictextureclearframebuffer")), (uint32_t)(m_voxelizedSceneWidth), (uint32_t)(m_voxelizedSceneHeight), move(string(m_dynamicTextureClearRenderPass->getName())), move(arrayAttachment));

	m_resetLitvoxelData = static_cast<ResetLitvoxelData*>(gpuPipelineM->getRasterTechniqueByName(move(string("ResetLitvoxelData"))));
	m_resetLitvoxelData->refSignalResetLitvoxelDataCompletion().connect<DynamicSceneVoxelizationTechnique, &DynamicSceneVoxelizationTechnique::slotResetLitVoxelComplete>(this);
}

/////////////////////////////////////////////////////////////////////////////////////////////

void DynamicSceneVoxelizationTechnique::prepare(float dt)
{

}

/////////////////////////////////////////////////////////////////////////////////////////////

VkCommandBuffer* DynamicSceneVoxelizationTechnique::record(int currentImage, uint& commandBufferID, CommandBufferType& commandBufferType)
{
	commandBufferType = CommandBufferType::CBT_GRAPHICS_QUEUE;

	VkCommandBuffer* commandBuffer;
	commandBuffer = addRecordedCommandBuffer(commandBufferID);
	addCommandBufferQueueType(commandBufferID, commandBufferType);
	coreM->allocCommandBuffer(&coreM->getLogicalDevice(), coreM->getGraphicsCommandPool(), commandBuffer);
	coreM->setObjectName(uint64_t(*commandBuffer), VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT, getName());

	// TODO: Add somehow command buffer measurement for this part
	coreM->beginCommandBuffer(*commandBuffer);

	VkImageSubresourceRange subresourceRange = {};
	subresourceRange.aspectMask   = VK_IMAGE_ASPECT_COLOR_BIT;
	subresourceRange.baseMipLevel = 0;
	subresourceRange.levelCount   = 1;
	subresourceRange.layerCount   = 1;

	textureM->setImageLayout(*commandBuffer, m_dynamicVoxelizationReflectanceTexture, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, subresourceRange, coreM->getGraphicsQueueIndex());

	VkClearValue clearValue;
	clearValue.color.float32[0] = 0.0f;
	clearValue.color.float32[1] = 0.0f;
	clearValue.color.float32[2] = 0.0f;
	clearValue.color.float32[3] = 0.0f;

	VkClearValue clearValueNegativeVoxelFace;
	clearValueNegativeVoxelFace.color.uint32[0] = 4294967295;
	clearValueNegativeVoxelFace.color.uint32[1] = 0;
	clearValueNegativeVoxelFace.color.uint32[2] = 0;
	clearValueNegativeVoxelFace.color.uint32[3] = 0;

	VkClearValue clearValuePositiveVoxelFace;
	clearValuePositiveVoxelFace.color.uint32[0] = 0;
	clearValuePositiveVoxelFace.color.uint32[1] = 0;
	clearValuePositiveVoxelFace.color.uint32[2] = 0;
	clearValuePositiveVoxelFace.color.uint32[3] = 0;

	vector<VkClearValue> vectorClearValue = { clearValue, clearValueNegativeVoxelFace, clearValuePositiveVoxelFace, clearValueNegativeVoxelFace, clearValuePositiveVoxelFace, clearValueNegativeVoxelFace, clearValuePositiveVoxelFace };

	VkRenderPassBeginInfo renderPassBegin = VulkanStructInitializer::renderPassBeginInfo(
		*m_dynamicTextureClearRenderPass->refRenderPass(),
		m_dynamicTextureClearFramebuffer->getFramebuffer(),
		VkRect2D({ 0, 0, uint32_t(m_sceneVoxelizationResolution), uint32_t(m_sceneVoxelizationResolution) }),
		vectorClearValue);

	vkCmdBeginRenderPass(*commandBuffer, &renderPassBegin, VK_SUBPASS_CONTENTS_INLINE);

	vkCmdEndRenderPass(*commandBuffer);

	textureM->setImageLayout(*commandBuffer, m_dynamicVoxelizationReflectanceTexture, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_GENERAL, subresourceRange, coreM->getGraphicsQueueIndex());

#ifdef USE_TIMESTAMP
	vkCmdResetQueryPool(*commandBuffer, coreM->getGraphicsQueueQueryPool(), m_queryIndex0, 1);
	vkCmdResetQueryPool(*commandBuffer, coreM->getGraphicsQueueQueryPool(), m_queryIndex1, 1);
	vkCmdWriteTimestamp(*commandBuffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, coreM->getGraphicsQueueQueryPool(), m_queryIndex0);
#endif

	recordRasterizationCommands(commandBuffer);

#ifdef USE_TIMESTAMP
	vkCmdWriteTimestamp(*commandBuffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, coreM->getGraphicsQueueQueryPool(), m_queryIndex1);
#endif

	VkBufferMemoryBarrier bufferMemoryBarrier;

	Buffer* buffer                          = bufferM->getElement(move(string("voxelOccupiedBuffer")));
	bufferMemoryBarrier.sType               = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
	bufferMemoryBarrier.pNext               = nullptr;
	bufferMemoryBarrier.srcAccessMask       = VK_ACCESS_SHADER_WRITE_BIT;
	bufferMemoryBarrier.dstAccessMask       = VK_ACCESS_SHADER_READ_BIT;
	bufferMemoryBarrier.srcQueueFamilyIndex = coreM->getGraphicsQueueIndex();
	bufferMemoryBarrier.dstQueueFamilyIndex = coreM->getGraphicsQueueIndex();
	bufferMemoryBarrier.offset              = 0;
	bufferMemoryBarrier.buffer              = buffer->getBuffer();
	bufferMemoryBarrier.size                = buffer->getDataSize();
	vkCmdPipelineBarrier(*commandBuffer, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_PIPELINE_STAGE_RAY_TRACING_SHADER_BIT_KHR, 0, 0, NULL, 1, &bufferMemoryBarrier, 0, NULL);

	buffer                                  = bufferM->getElement(move(string("voxelOccupiedDynamicBuffer")));
	bufferMemoryBarrier.buffer              = buffer->getBuffer();
	bufferMemoryBarrier.size                = buffer->getDataSize();
	vkCmdPipelineBarrier(*commandBuffer, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_PIPELINE_STAGE_RAY_TRACING_SHADER_BIT_KHR, 0, 0, NULL, 1, &bufferMemoryBarrier, 0, NULL);

	coreM->endCommandBuffer(*commandBuffer);

	// NOTE: Clear command buffer if re-recorded
	m_vectorCommand.push_back(commandBuffer);

	return commandBuffer;
}

/////////////////////////////////////////////////////////////////////////////////////////////

void DynamicSceneVoxelizationTechnique::postCommandSubmit()
{
	m_executeCommand = false;
	m_active         = false;
	m_needsToRecord  = false;

	// Add barrier for the voxel occupied dynamic buffer?

	m_dynamicVoxelizationComplete.emit();
}

/////////////////////////////////////////////////////////////////////////////////////////////

void DynamicSceneVoxelizationTechnique::slotResetLitVoxelComplete()
{
	m_active        = true;
	m_needsToRecord = true;
}

/////////////////////////////////////////////////////////////////////////////////////////////
