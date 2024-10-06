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
#include "../../include/rastertechnique/staticscenevoxelizationtechnique.h"
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

// NAMESPACE
using namespace attributedefines;

// DEFINES

// STATIC MEMBER INITIALIZATION

/////////////////////////////////////////////////////////////////////////////////////////////

StaticSceneVoxelizationTechnique::StaticSceneVoxelizationTechnique(string &&name, string&& className) : SceneVoxelizationTechnique(move(name), move(className))
	, m_voxelizationReflectanceTexture(nullptr)
	, m_staticVoxelIndexTexture(nullptr)
	, m_voxelOccupiedBuffer(nullptr)
	, m_voxelFirstIndexBuffer(nullptr)
	, m_clearVoxelIndexTextureRenderPass(nullptr)
	, m_clearVoxelIndexTextureFramebuffer(nullptr)
{
	m_recordPolicy              = CommandRecordPolicy::CRP_SINGLE_TIME;
	m_isStaticSceneVoxelization = true;
}

/////////////////////////////////////////////////////////////////////////////////////////////

StaticSceneVoxelizationTechnique::~StaticSceneVoxelizationTechnique()
{

}

/////////////////////////////////////////////////////////////////////////////////////////////

void StaticSceneVoxelizationTechnique::init()
{
	m_voxelizationReflectanceTexture = textureM->buildTexture(
		move(string("voxelizationReflectance")),
		VK_FORMAT_R32_UINT,
		VkExtent3D({ uint32_t(m_voxelizedSceneWidth), uint32_t(m_voxelizedSceneWidth), uint32_t(m_voxelizedSceneHeight) }),
		VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT,
		VK_IMAGE_ASPECT_COLOR_BIT,
		VK_IMAGE_ASPECT_COLOR_BIT,
		VK_IMAGE_LAYOUT_UNDEFINED,
		VK_IMAGE_LAYOUT_GENERAL,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		VK_SAMPLE_COUNT_1_BIT,
		VK_IMAGE_TILING_OPTIMAL,
		VK_IMAGE_VIEW_TYPE_3D,
		0);

	m_staticVoxelIndexTexture = textureM->buildTexture(
		move(string("staticVoxelIndexTexture")),
		VK_FORMAT_R32_UINT,
		VkExtent3D({ uint32_t(m_voxelizedSceneWidth), uint32_t(m_voxelizedSceneWidth), uint32_t(m_voxelizedSceneHeight) }),
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
		// so it can be used as attachemnt in a framebuffer to be cleared
		VK_IMAGE_CREATE_2D_ARRAY_COMPATIBLE_BIT);

	textureM->buildTexture(
		move(string("voxelization3DDebug")),
		VK_FORMAT_R8G8B8A8_UNORM,
		VkExtent3D({ uint32_t(m_voxelizedSceneWidth), uint32_t(m_voxelizedSceneWidth), uint32_t(m_voxelizedSceneHeight) }),
		VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT,
		VK_IMAGE_ASPECT_COLOR_BIT,
		VK_IMAGE_ASPECT_COLOR_BIT,
		VK_IMAGE_LAYOUT_UNDEFINED,
		VK_IMAGE_LAYOUT_GENERAL,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		VK_SAMPLE_COUNT_1_BIT,
		VK_IMAGE_TILING_OPTIMAL,
		VK_IMAGE_VIEW_TYPE_3D,
		0);

	// Generate texture to render to, render pass and framebuffer, which will be shared with the dynamic scene voxelization technique
	SceneVoxelizationTechnique::init();

	vector<uint> vectorData;
	vectorData.resize(m_voxelizedSceneWidth * m_voxelizedSceneHeight * m_voxelizedSceneDepth);
	memset(vectorData.data(), maxValue, vectorData.size() * size_t(sizeof(uint)));

	vector<uint> vectorVoxelOccupied;
	vectorVoxelOccupied.resize(m_voxelizedSceneWidth * m_voxelizedSceneHeight * m_voxelizedSceneDepth / 8);
	memset(vectorVoxelOccupied.data(), 0, vectorVoxelOccupied.size() * size_t(sizeof(uint)));
	// Build shader storage buffer to know if a particular voxel is occupied or is not
	// A single bit is used to know if the hashed value of the 3D position of a voxel is empty or occupied
	m_voxelOccupiedBuffer = bufferM->buildBuffer(
		move(string("voxelOccupiedBuffer")),
		vectorVoxelOccupied.data(),
		vectorVoxelOccupied.size(),
		VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

	// Build shader storage buffer to know first index of each generated fragment
	m_voxelFirstIndexBuffer = bufferM->buildBuffer(
		move(string("voxelFirstIndexBuffer")),
		vectorData.data(),
		vectorData.size() * size_t(sizeof(uint)),
		VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
		VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

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
	attributeUM->newElement<AttributeData<VkPipelineBindPoint*>*>(new AttributeData<VkPipelineBindPoint*>(string(g_renderPassAttachmentPipelineBindPoint), move(pipelineBindPoint)));
	attributeUM->newElement<AttributeData<vector<VkFormat>*>*>(new AttributeData<vector<VkFormat>*>(string(g_renderPassAttachmentFormat), move(vectorAttachmentFormat)));
	attributeUM->newElement<AttributeData<vector<VkSampleCountFlagBits>*>*>(new AttributeData<vector<VkSampleCountFlagBits>*>(string(g_renderPassAttachmentSamplesPerPixel), move(vectorAttachmentSamplesPerPixel)));
	attributeUM->newElement<AttributeData<vector<VkImageLayout>*>*>(new AttributeData<vector<VkImageLayout>*>(string(g_renderPassAttachmentFinalLayout), move(vectorAttachmentFinalLayout)));
	attributeUM->newElement<AttributeData<vector<VkAttachmentReference>*>*>(new AttributeData<vector<VkAttachmentReference>*>(string(g_renderPassAttachmentColorReference), move(vectorColorReference)));

	m_clearVoxelIndexTextureRenderPass = renderPassM->buildRenderPass(move(string("clearvoxelindextexturerenderpass")), attributeUM);

	vector<string> arrayAttachment{ m_staticVoxelIndexTexture->getName() };
	m_clearVoxelIndexTextureFramebuffer = framebufferM->buildFramebuffer(move(string("clearvoxelindextextureframebuffer")), (uint32_t)(m_voxelizedSceneWidth), (uint32_t)(m_voxelizedSceneHeight), move(string(m_clearVoxelIndexTextureRenderPass->getName())), move(arrayAttachment));
}

/////////////////////////////////////////////////////////////////////////////////////////////

void StaticSceneVoxelizationTechnique::prepare(float dt)
{

}

/////////////////////////////////////////////////////////////////////////////////////////////

VkCommandBuffer* StaticSceneVoxelizationTechnique::record(int currentImage, uint& commandBufferID, CommandBufferType& commandBufferType)
{
	commandBufferType = CommandBufferType::CBT_GRAPHICS_QUEUE;

	VkCommandBuffer* commandBuffer;
	commandBuffer = addRecordedCommandBuffer(commandBufferID);
	addCommandBufferQueueType(commandBufferID, commandBufferType);
	coreM->allocCommandBuffer(&coreM->getLogicalDevice(), coreM->getGraphicsCommandPool(), commandBuffer);
	coreM->setObjectName(uint64_t(*commandBuffer), VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT, getName());

	coreM->beginCommandBuffer(*commandBuffer);

	VkImageSubresourceRange subresourceRange = {};
	subresourceRange.aspectMask   = VK_IMAGE_ASPECT_COLOR_BIT;
	subresourceRange.baseMipLevel = 0;
	subresourceRange.levelCount   = 1;
	subresourceRange.layerCount   = 1;

	textureM->setImageLayout(*commandBuffer, m_staticVoxelIndexTexture, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, subresourceRange, coreM->getGraphicsQueueIndex());

	VkClearValue clearValueStaticVoxelIndex;
	clearValueStaticVoxelIndex.color.uint32[0] = 4294967295; // Initialise with this value to distinguish voxel index 0
	clearValueStaticVoxelIndex.color.uint32[1] = 0;
	clearValueStaticVoxelIndex.color.uint32[2] = 0;
	clearValueStaticVoxelIndex.color.uint32[3] = 0;

	vector<VkClearValue> vectorClearValue = { clearValueStaticVoxelIndex };

	VkRenderPassBeginInfo renderPassBegin = VulkanStructInitializer::renderPassBeginInfo(
		*m_clearVoxelIndexTextureRenderPass->refRenderPass(),
		 m_clearVoxelIndexTextureFramebuffer->getFramebuffer(),
		 VkRect2D({ 0, 0, uint32_t(m_voxelizedSceneWidth), uint32_t(m_voxelizedSceneHeight) }),
		 vectorClearValue);

	vkCmdBeginRenderPass(*commandBuffer, &renderPassBegin, VK_SUBPASS_CONTENTS_INLINE);

	vkCmdEndRenderPass(*commandBuffer);

	textureM->setImageLayout(*commandBuffer, m_staticVoxelIndexTexture, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_GENERAL, subresourceRange, coreM->getGraphicsQueueIndex());

#ifdef USE_TIMESTAMP
	vkCmdResetQueryPool(*commandBuffer, coreM->getGraphicsQueueQueryPool(), m_queryIndex0, 1);
	vkCmdResetQueryPool(*commandBuffer, coreM->getGraphicsQueueQueryPool(), m_queryIndex1, 1);
	vkCmdWriteTimestamp(*commandBuffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, coreM->getGraphicsQueueQueryPool(), m_queryIndex0);
#endif

	recordRasterizationCommands(commandBuffer);

#ifdef USE_TIMESTAMP
	vkCmdWriteTimestamp(*commandBuffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, coreM->getGraphicsQueueQueryPool(), m_queryIndex1);
#endif

	coreM->endCommandBuffer(*commandBuffer);

	// NOTE: Clear command buffer if re-recorded
	m_vectorCommand.push_back(commandBuffer);

	return commandBuffer;
}

/////////////////////////////////////////////////////////////////////////////////////////////

void StaticSceneVoxelizationTechnique::postCommandSubmit()
{
	m_needsToRecord  = false;
	m_executeCommand = false;
	m_active         = false;

	m_staticVoxelizationComplete.emit();
}

/////////////////////////////////////////////////////////////////////////////////////////////
