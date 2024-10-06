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
#include "../../include/rastertechnique/voxelrasterinscenariotechnique.h"
#include "../../include/rastertechnique/staticscenevoxelizationtechnique.h"
#include "../../include/rastertechnique/bufferprefixsumtechnique.h"
#include "../../include/core/coremanager.h"
#include "../../include/buffer/buffermanager.h"
#include "../../include/buffer/buffer.h"
#include "../../include/material/materialvoxelrasterinscenario.h"
#include "../../include/util/vulkanstructinitializer.h"
#include "../../include/uniformbuffer/uniformbuffer.h"
#include "../../include/camera/cameramanager.h"
#include "../../include/framebuffer/framebuffer.h"
#include "../../include/framebuffer/framebuffermanager.h"
#include "../../include/rastertechnique/dynamicvoxelcopytobuffertechnique.h"
#include "../../include/parameter/attributedefines.h"
#include "../../include/parameter/attributedata.h"

// NAMESPACE
using namespace attributedefines;

// DEFINES

// STATIC MEMBER INITIALIZATION

/////////////////////////////////////////////////////////////////////////////////////////////

VoxelRasterInScenarioTechnique::VoxelRasterInScenarioTechnique(string &&name, string&& className) : RasterTechnique(move(name), move(className))
	, m_material(nullptr)
	, m_numOccupiedVoxel(0)
	, m_voxelrasterinscenariodebugbuffer(nullptr)
	, m_renderPass(nullptr)
	, m_framebuffer(nullptr)
{
	m_active = false;
}

/////////////////////////////////////////////////////////////////////////////////////////////

VoxelRasterInScenarioTechnique::~VoxelRasterInScenarioTechnique()
{

}

/////////////////////////////////////////////////////////////////////////////////////////////

void VoxelRasterInScenarioTechnique::init()
{
	inputM->refEventSinglePressSignalSlot().addKeyDownSignal(KeyCode::KEY_CODE_R);
	SignalVoid* signalAdd = inputM->refEventSinglePressSignalSlot().refKeyDownSignalByKey(KeyCode::KEY_CODE_R);
	signalAdd->connect<VoxelRasterInScenarioTechnique, &VoxelRasterInScenarioTechnique::slotRKeyPressed>(this);

	inputM->refEventSinglePressSignalSlot().addKeyDownSignal(KeyCode::KEY_CODE_N);
	signalAdd = inputM->refEventSinglePressSignalSlot().refKeyDownSignalByKey(KeyCode::KEY_CODE_N);
	signalAdd->connect<VoxelRasterInScenarioTechnique, &VoxelRasterInScenarioTechnique::slotNKeyPressed>(this);

	inputM->refEventSinglePressSignalSlot().addKeyDownSignal(KeyCode::KEY_CODE_V);
	signalAdd = inputM->refEventSinglePressSignalSlot().refKeyDownSignalByKey(KeyCode::KEY_CODE_V);
	signalAdd->connect<VoxelRasterInScenarioTechnique, &VoxelRasterInScenarioTechnique::slotVKeyPressed>(this);

	inputM->refEventSinglePressSignalSlot().addKeyDownSignal(KeyCode::KEY_CODE_Y);
	signalAdd = inputM->refEventSinglePressSignalSlot().refKeyDownSignalByKey(KeyCode::KEY_CODE_Y);
	signalAdd->connect<VoxelRasterInScenarioTechnique, &VoxelRasterInScenarioTechnique::slotYKeyPressed>(this);

	vector<uint> vectorData;
	vectorData.resize(4096);
	memset(vectorData.data(), 0, vectorData.size() * size_t(sizeof(uint)));
	m_voxelrasterinscenariodebugbuffer = bufferM->buildBuffer(
		move(string("voxelrasterinscenariodebugbuffer")),
		vectorData.data(),
		vectorData.size() * sizeof(uint),
		VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

	m_material = static_cast<MaterialVoxelRasterInScenario*>(materialM->buildMaterial(move(string("MaterialVoxelRasterInScenario")), move(string("MaterialVoxelRasterInScenario")), nullptr));

	m_vectorMaterialName.push_back("MaterialVoxelRasterInScenario");
	m_vectorMaterial.push_back(m_material);

	StaticSceneVoxelizationTechnique* technique = static_cast<StaticSceneVoxelizationTechnique*>(gpuPipelineM->getRasterTechniqueByName(move(string("StaticSceneVoxelizationTechnique"))));

	BufferPrefixSumTechnique* techniquePrefixSum = static_cast<BufferPrefixSumTechnique*>(gpuPipelineM->getRasterTechniqueByName(move(string("BufferPrefixSumTechnique"))));
	techniquePrefixSum->refPrefixSumComplete().connect<VoxelRasterInScenarioTechnique, &VoxelRasterInScenarioTechnique::slotPrefixSumCompleted>(this);

	VkPipelineBindPoint* pipelineBindPoint                         = new VkPipelineBindPoint(VK_PIPELINE_BIND_POINT_GRAPHICS);
	vector<VkFormat>* vectorAttachmentFormat                       = new vector<VkFormat>;
	vector<VkSampleCountFlagBits>* vectorAttachmentSamplesPerPixel = new vector<VkSampleCountFlagBits>;
	vector<VkImageLayout>* vectorAttachmentFinalLayout             = new vector<VkImageLayout>;
	vector<VkAttachmentReference>* vectorColorReference            = new vector<VkAttachmentReference>;
	vector<VkAttachmentLoadOp>* vectorAttachmentLoadOp             = new vector<VkAttachmentLoadOp>;
	VkAttachmentReference* depthReference                          = new VkAttachmentReference({ 1, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL });

	vectorAttachmentFormat->push_back(VK_FORMAT_R8G8B8A8_UNORM);
	vectorAttachmentFormat->push_back(VK_FORMAT_D24_UNORM_S8_UINT);

	vectorAttachmentSamplesPerPixel->push_back(VK_SAMPLE_COUNT_1_BIT);
	vectorAttachmentSamplesPerPixel->push_back(VK_SAMPLE_COUNT_1_BIT);

	vectorAttachmentFinalLayout->push_back(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
	vectorAttachmentFinalLayout->push_back(VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);

	vectorColorReference->push_back({ 0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL });
	
	vectorAttachmentLoadOp->push_back(VK_ATTACHMENT_LOAD_OP_LOAD);
	vectorAttachmentLoadOp->push_back(VK_ATTACHMENT_LOAD_OP_LOAD);

	MultiTypeUnorderedMap *attributeUM = new MultiTypeUnorderedMap();
	attributeUM->newElement<AttributeData<VkAttachmentReference*>*>        (new AttributeData<VkAttachmentReference*>(string(g_renderPassAttachmentDepthReference),            move(depthReference)));
	attributeUM->newElement<AttributeData<VkPipelineBindPoint*>*>          (new AttributeData<VkPipelineBindPoint*>          (string(g_renderPassAttachmentPipelineBindPoint), move(pipelineBindPoint)));
	attributeUM->newElement<AttributeData<vector<VkFormat>*>*>             (new AttributeData<vector<VkFormat>*>             (string(g_renderPassAttachmentFormat),            move(vectorAttachmentFormat)));
	attributeUM->newElement<AttributeData<vector<VkSampleCountFlagBits>*>*>(new AttributeData<vector<VkSampleCountFlagBits>*>(string(g_renderPassAttachmentSamplesPerPixel),   move(vectorAttachmentSamplesPerPixel)));
	attributeUM->newElement<AttributeData<vector<VkImageLayout>*>*>        (new AttributeData<vector<VkImageLayout>*>        (string(g_renderPassAttachmentFinalLayout),       move(vectorAttachmentFinalLayout)));
	attributeUM->newElement<AttributeData<vector<VkAttachmentReference>*>*>(new AttributeData<vector<VkAttachmentReference>*>(string(g_renderPassAttachmentColorReference),    move(vectorColorReference)));
	attributeUM->newElement<AttributeData<vector<VkAttachmentLoadOp>*>*>   (new AttributeData<vector<VkAttachmentLoadOp>*>   (string(g_renderPassAttachmentLoadOp),            move(vectorAttachmentLoadOp)));
	m_renderPass = renderPassM->buildRenderPass(move(string("voxelrasterinscenariorenderpass")), attributeUM);

	vector<string> arrayAttachment = { "scenelightingcolor", "GBufferDepth" };
	m_framebuffer = framebufferM->buildFramebuffer(move(string("voxelrasterinscenarioFB")), coreM->getWidth(), coreM->getHeight(), move(string("voxelrasterinscenariorenderpass")), move(arrayAttachment));
}

/////////////////////////////////////////////////////////////////////////////////////////////

void VoxelRasterInScenarioTechnique::prepare(float dt)
{
	mat4 viewMatrix       = cameraM->refMainCamera()->getView();
	mat4 projectionMatrix = cameraM->refMainCamera()->getProjection();
	mat4 viewProjection   = projectionMatrix * viewMatrix;

	m_material->setViewProjection(viewProjection);

	if (sceneM->getIsVectorNodeFrameUpdated())
	{
		m_needsToRecord = true;
	}
}

/////////////////////////////////////////////////////////////////////////////////////////////

VkCommandBuffer* VoxelRasterInScenarioTechnique::record(int currentImage, uint& commandBufferID, CommandBufferType& commandBufferType)
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

	DynamicVoxelCopyToBufferTechnique* technique = static_cast<DynamicVoxelCopyToBufferTechnique*>(gpuPipelineM->getRasterTechniqueByName(move(string("DynamicVoxelCopyToBufferTechnique"))));
	uint dynamicVoxelCounter = technique->getDynamicVoxelCounter();

	if (dynamicVoxelCounter > 0)
	{
		VulkanStructInitializer::insertBufferMemoryBarrier(bufferM->getElement(move(string("dynamicVoxelBuffer"))),
														   VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT,
														   VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT,
														   VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
														   VK_PIPELINE_STAGE_VERTEX_SHADER_BIT,
														   commandBuffer);
	}

	VkRenderPassBeginInfo renderPassBegin = VulkanStructInitializer::renderPassBeginInfo(
		m_renderPass->getRenderPass(),
		m_framebuffer->getFramebuffer(),
		VkRect2D({ 0, 0, coreM->getWidth(), coreM->getHeight() }),
		m_material->refVectorClearValue());

	vkCmdBeginRenderPass(*commandBuffer, &renderPassBegin, VK_SUBPASS_CONTENTS_INLINE);

	const VkDeviceSize offsets[1] = { 0 };
	vkCmdBindVertexBuffers(*commandBuffer, 0, 1, &bufferM->getElement(move(string("voxelHashedPositionCompactedBuffer")))->getBuffer(), offsets); // Bound the command buffer with the graphics pipeline

	m_material->setPushConstantVoxelFromStaticCompacted(1);
	m_material->updatePushConstantCPUBuffer();
	CPUBuffer& cpuBufferNewCenter = m_material->refShader()->refPushConstant().refCPUBuffer();

	vkCmdPushConstants(
		*commandBuffer,
		m_material->getPipelineLayout(),
		VK_SHADER_STAGE_GEOMETRY_BIT,
		0,
		uint32_t(m_material->getPushConstantExposedStructFieldSize()),
		cpuBufferNewCenter.refUBHostMemory());

	vkCmdBindPipeline(*commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_material->getPipeline()->getPipeline());

	gpuPipelineM->initViewports((float)coreM->getWidth(), (float)coreM->getHeight(), 0.0f, 0.0f, 0.0f, 1.0f, commandBuffer);
	gpuPipelineM->initScissors(coreM->getWidth(), coreM->getHeight(), 0, 0, commandBuffer);

	uint dynamicAllignment = materialM->getMaterialUBDynamicAllignment();

	uint32_t offsetData[2];
	offsetData[0] = 0;
	offsetData[1] = static_cast<uint32_t>(m_material->getMaterialUniformBufferIndex() * dynamicAllignment);
	vkCmdBindDescriptorSets(*commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_material->getPipelineLayout(), 0, 1, &m_material->refDescriptorSet(), 2, &offsetData[0]);

	vkCmdDraw(*commandBuffer, m_numOccupiedVoxel, 1, 0, 0);

	
	if (dynamicVoxelCounter > 0)
	{
		m_material->setPushConstantVoxelFromStaticCompacted(0);
		m_material->updatePushConstantCPUBuffer();
		CPUBuffer& cpuBufferNewCenter = m_material->refShader()->refPushConstant().refCPUBuffer();

		vkCmdPushConstants(
			*commandBuffer,
			m_material->getPipelineLayout(),
			VK_SHADER_STAGE_GEOMETRY_BIT,
			0,
			uint32_t(m_material->getPushConstantExposedStructFieldSize()),
			cpuBufferNewCenter.refUBHostMemory());

		vkCmdBindVertexBuffers(*commandBuffer, 0, 1, &bufferM->getElement(move(string("dynamicVoxelBuffer")))->getBuffer(), offsets); // Bound the command buffer with the graphics pipeline

		vkCmdDraw(*commandBuffer, dynamicVoxelCounter, 1, 0, 0);
	}
	
	vkCmdEndRenderPass(*commandBuffer);

#ifdef USE_TIMESTAMP
	vkCmdWriteTimestamp(*commandBuffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, coreM->getGraphicsQueueQueryPool(), m_queryIndex1);
#endif

	coreM->endCommandBuffer(*commandBuffer);

	// NOTE: Clear command buffer if re-recorded
	m_vectorCommand.push_back(commandBuffer);

	return commandBuffer;
}

/////////////////////////////////////////////////////////////////////////////////////////////

void VoxelRasterInScenarioTechnique::postCommandSubmit()
{
	m_executeCommand = false;
	m_needsToRecord  = false;
}

/////////////////////////////////////////////////////////////////////////////////////////////

void VoxelRasterInScenarioTechnique::slotPrefixSumCompleted()
{
	BufferPrefixSumTechnique* techniquePrefixSum = static_cast<BufferPrefixSumTechnique*>(gpuPipelineM->getRasterTechniqueByName(move(string("BufferPrefixSumTechnique"))));
	m_numOccupiedVoxel = techniquePrefixSum->getFirstIndexOccupiedElement();
	bufferM->resize(m_voxelrasterinscenariodebugbuffer, nullptr, m_numOccupiedVoxel * 8 * sizeof(uint));
}

/////////////////////////////////////////////////////////////////////////////////////////////

void VoxelRasterInScenarioTechnique::slotRKeyPressed()
{
	m_active = true;
}

/////////////////////////////////////////////////////////////////////////////////////////////

void VoxelRasterInScenarioTechnique::slotNKeyPressed()
{
	m_active = true;
}

/////////////////////////////////////////////////////////////////////////////////////////////

void VoxelRasterInScenarioTechnique::slotVKeyPressed()
{
	m_active = true;
}

/////////////////////////////////////////////////////////////////////////////////////////////

void VoxelRasterInScenarioTechnique::slotLKeyPressed()
{

}

/////////////////////////////////////////////////////////////////////////////////////////////

void VoxelRasterInScenarioTechnique::slotYKeyPressed()
{
	m_active = true;
}

/////////////////////////////////////////////////////////////////////////////////////////////
