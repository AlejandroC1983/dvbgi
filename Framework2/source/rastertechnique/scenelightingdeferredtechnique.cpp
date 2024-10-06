/*
Copyright 2023 Alejandro Cosin

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
#include "../../include/rastertechnique/scenelightingdeferredtechnique.h"
#include "../../include/rastertechnique/litvoxeltechnique.h"
#include "../../include/core/gpupipeline.h"
#include "../../include/material/materialmanager.h"
#include "../../include/material/material.h"
#include "../../include/util/vulkanstructinitializer.h"
#include "../../include/parameter/attributedefines.h"
#include "../../include/parameter/attributedata.h"
#include "../../include/renderpass/renderpassmanager.h"
#include "../../include/renderpass/renderpass.h"
#include "../../include/material/materialscenelightingdeferred.h"
#include "../../include/core/input.h"
#include "../../include/rastertechnique/staticscenevoxelizationtechnique.h"
#include "../../include/scene/scene.h"
#include "../../include/material/materialcolortexture.h"
#include "../../include/material/materiallighting.h"
#include "../../include/framebuffer/framebuffer.h"
#include "../../include/framebuffer/framebuffermanager.h"
#include "../../include/camera/cameramanager.h"
#include "../../include/camera/camera.h"
#include "../../include/buffer/buffermanager.h"
#include "../../include/buffer/buffer.h"

// NAMESPACE
using namespace attributedefines;

// DEFINES

// STATIC MEMBER INITIALIZATION
string SceneLightingDeferredTechnique::m_lightingMaterialSuffix = "_Lighting";

/////////////////////////////////////////////////////////////////////////////////////////////

SceneLightingDeferredTechnique::SceneLightingDeferredTechnique(string &&name, string&& className) : RasterTechnique(move(name), move(className))
	, m_litVoxelTechnique(nullptr)
	, m_renderPass(nullptr)
	, m_materialSceneLightingDeferred(nullptr)
{
	m_active = false;

	m_vectorTexture.resize(13);
}

/////////////////////////////////////////////////////////////////////////////////////////////

SceneLightingDeferredTechnique::~SceneLightingDeferredTechnique()
{

}

/////////////////////////////////////////////////////////////////////////////////////////////

void SceneLightingDeferredTechnique::init()
{
	m_litVoxelTechnique = static_cast<LitVoxelTechnique*>(gpuPipelineM->getRasterTechniqueByName(move(string("LitVoxelTechnique"))));
	
	bufferM->buildBuffer(
		move(string("sceneLightingDeferredDebugBuffer")),
		nullptr,
		256 * 1024 * 4,
		VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

	VkPipelineBindPoint* pipelineBindPoint = new VkPipelineBindPoint(VK_PIPELINE_BIND_POINT_GRAPHICS);
	vector<VkFormat>* vectorAttachmentFormat = new vector<VkFormat>;
	vector<VkSampleCountFlagBits>* vectorAttachmentSamplesPerPixel = new vector<VkSampleCountFlagBits>;
	vector<VkImageLayout>* vectorAttachmentFinalLayout = new vector<VkImageLayout>;
	vector<VkAttachmentReference>* vectorColorReference = new vector<VkAttachmentReference>;
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
	m_renderPass = renderPassM->buildRenderPass(move(string("scenelightingdeferredrenderpass")), attributeUM);

	vector<string> arrayAttachment = { "scenelightingcolor" };
	m_framebuffer = framebufferM->buildFramebuffer(move(string("scenelightingdeferredrenderpassFB")), coreM->getWidth(), coreM->getHeight(), move(string("scenelightingdeferredrenderpass")), move(arrayAttachment));

	m_materialSceneLightingDeferred = static_cast<MaterialSceneLightingDeferred*>(materialM->buildMaterial(move(string("MaterialSceneLightingDeferred")), move(string("MaterialSceneLightingDeferred")), nullptr));

	StaticSceneVoxelizationTechnique* technique = static_cast<StaticSceneVoxelizationTechnique*>(gpuPipelineM->getRasterTechniqueByName(move(string("StaticSceneVoxelizationTechnique"))));
	vec4 voxelSize                              = vec4(float(technique->getVoxelizedSceneWidth()), float(technique->getVoxelizedSceneHeight()), float(technique->getVoxelizedSceneHeight()), 0.0f);

	vec3 min3D;
	vec3 max3D;
	BBox3D& box = sceneM->refBox();

	box.getCenteredBoxMinMax(min3D, max3D);

	vec3 extent3D = max3D - min3D;
	vec4 extent   = vec4(extent3D.x, extent3D.y, extent3D.z, 0.0f);
	vec4 min      = vec4(min3D.x,    min3D.y,    min3D.z,    0.0f);
	vec4 max      = vec4(max3D.x,    max3D.y,    max3D.z,    0.0f);

	float emitterRadiance = float(gpuPipelineM->getRasterFlagValue(move(string("EMITTER_RADIANCE"))));
	m_materialSceneLightingDeferred->setSceneMinEmitterRadiance(vec4(min.x, min.y, min.z, emitterRadiance));
	m_materialSceneLightingDeferred->setSceneExtentVoxelSize(vec4(extent.x, extent.y, extent.z, voxelSize.x));

	m_vectorMaterialName.push_back("MaterialSceneLightingDeferred");
	m_vectorMaterial.push_back(m_materialSceneLightingDeferred);

	inputM->refEventSinglePressSignalSlot().addKeyDownSignal(KeyCode::KEY_CODE_L);
	SignalVoid* signalAdd = inputM->refEventSinglePressSignalSlot().refKeyDownSignalByKey(KeyCode::KEY_CODE_L);
	signalAdd->connect<SceneLightingDeferredTechnique, &SceneLightingDeferredTechnique::slotLKeyPressed>(this);

	inputM->refEventSinglePressSignalSlot().addKeyDownSignal(KeyCode::KEY_CODE_1);
	signalAdd = inputM->refEventSinglePressSignalSlot().refKeyDownSignalByKey(KeyCode::KEY_CODE_1);
	signalAdd->connect<SceneLightingDeferredTechnique, &SceneLightingDeferredTechnique::slot1KeyPressed>(this);

	inputM->refEventSinglePressSignalSlot().addKeyDownSignal(KeyCode::KEY_CODE_2);
	signalAdd = inputM->refEventSinglePressSignalSlot().refKeyDownSignalByKey(KeyCode::KEY_CODE_2);
	signalAdd->connect<SceneLightingDeferredTechnique, &SceneLightingDeferredTechnique::slot2KeyPressed>(this);

	inputM->refEventSinglePressSignalSlot().addKeyDownSignal(KeyCode::KEY_CODE_7);
	signalAdd = inputM->refEventSinglePressSignalSlot().refKeyDownSignalByKey(KeyCode::KEY_CODE_7);
	signalAdd->connect<SceneLightingDeferredTechnique, &SceneLightingDeferredTechnique::slot7KeyPressed>(this);

	inputM->refEventSinglePressSignalSlot().addKeyDownSignal(KeyCode::KEY_CODE_8);
	signalAdd = inputM->refEventSinglePressSignalSlot().refKeyDownSignalByKey(KeyCode::KEY_CODE_8);
	signalAdd->connect<SceneLightingDeferredTechnique, &SceneLightingDeferredTechnique::slot8KeyPressed>(this);

	vectorString vectorTextureName = 
	{
		"irradiance3DStaticNegativeX",
		"irradiance3DStaticPositiveX",
		"irradiance3DStaticNegativeY",
		"irradiance3DStaticPositiveY",
		"irradiance3DStaticNegativeZ",
		"irradiance3DStaticPositiveZ",
		"irradiance3DDynamicNegativeX",
		"irradiance3DDynamicPositiveX",
		"irradiance3DDynamicNegativeY",
		"irradiance3DDynamicPositiveY",
		"irradiance3DDynamicNegativeZ",
		"irradiance3DDynamicPositiveZ",
		//"GBufferNormal",
		"GBufferReflectance",
		//"GBufferPosition"
	};

	forI(m_vectorTexture.size())
	{
		m_vectorTexture[i] = textureM->getElement(move(vectorTextureName[i]));
	}
}

/////////////////////////////////////////////////////////////////////////////////////////////

void SceneLightingDeferredTechnique::prepare(float dt)
{
	// TODO: automatize this
	Camera* shadowMappingCamera = cameraM->getElement(move(string("emitter")));
	vec3 position               = shadowMappingCamera->getPosition();
	vec3 cameraForward          = shadowMappingCamera->getLookAt();

	m_materialSceneLightingDeferred->setLightPosition(vec4(position.x, position.y, position.z, 0.0f));
	m_materialSceneLightingDeferred->setLightForward(vec4(cameraForward.x, cameraForward.y, cameraForward.z, 0.0f));
}

/////////////////////////////////////////////////////////////////////////////////////////////

VkCommandBuffer* SceneLightingDeferredTechnique::record(int currentImage, uint& commandBufferID, CommandBufferType& commandBufferType)
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

	VkImageMemoryBarrier imageMemoryBarrier{};
	imageMemoryBarrier.sType               = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	imageMemoryBarrier.srcQueueFamilyIndex = coreM->getGraphicsQueueIndex();
	imageMemoryBarrier.dstQueueFamilyIndex = coreM->getGraphicsQueueIndex();
	imageMemoryBarrier.srcAccessMask       = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;
	imageMemoryBarrier.dstAccessMask       = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;
	imageMemoryBarrier.oldLayout           = VK_IMAGE_LAYOUT_GENERAL;
	imageMemoryBarrier.newLayout           = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	imageMemoryBarrier.subresourceRange    = VkImageSubresourceRange{ VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};

	forI(m_vectorTexture.size())
	{
		imageMemoryBarrier.image = m_vectorTexture[i]->getImage();
		vkCmdPipelineBarrier(*commandBuffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 1, &imageMemoryBarrier);
	}

	forI(m_vectorTexture.size())
	{
		textureM->setImageLayout(*commandBuffer, m_vectorTexture[i], VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, subresourceRange, coreM->getGraphicsQueueIndex());
	}

	VkRenderPassBeginInfo renderPassBeginInfo = VulkanStructInitializer::renderPassBeginInfo(
		m_renderPass->getRenderPass(),
		m_framebuffer->getFramebuffer(),
		VkRect2D({ 0, 0, coreM->getWidth(), coreM->getHeight() }),
		m_materialSceneLightingDeferred->refVectorClearValue());

	vkCmdBeginRenderPass(*commandBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

	gpuPipelineM->initViewports(float(coreM->getWidth()), float(coreM->getHeight()), 0.0f, 0.0f, 0.0f, 1.0f, commandBuffer);
	gpuPipelineM->initScissors(coreM->getWidth(), coreM->getHeight(), 0, 0, commandBuffer);

	uint dynamicAllignment = materialM->getMaterialUBDynamicAllignment();
	uint32_t offsetData    = static_cast<uint32_t>(m_materialSceneLightingDeferred->getMaterialUniformBufferIndex() * dynamicAllignment);

	vkCmdBindDescriptorSets(*commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_materialSceneLightingDeferred->getPipelineLayout(), 0, 1, &m_materialSceneLightingDeferred->refDescriptorSet(), 1, &offsetData);
	vkCmdBindPipeline(*commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_materialSceneLightingDeferred->getPipeline()->getPipeline());
	vkCmdDraw(*commandBuffer, 3, 1, 0, 0);

	vkCmdEndRenderPass(*commandBuffer);

	forI(m_vectorTexture.size())
	{
		textureM->setImageLayout(*commandBuffer, m_vectorTexture[i], VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_LAYOUT_GENERAL, subresourceRange, coreM->getGraphicsQueueIndex());
	}

#ifdef USE_TIMESTAMP
	vkCmdWriteTimestamp(*commandBuffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, coreM->getGraphicsQueueQueryPool(), m_queryIndex1);
#endif

	coreM->endCommandBuffer(*commandBuffer);

	// NOTE: Clear command buffer if re-recorded
	m_vectorCommand.push_back(commandBuffer);

	return commandBuffer;
}

/////////////////////////////////////////////////////////////////////////////////////////////

void SceneLightingDeferredTechnique::postCommandSubmit()
{
	m_executeCommand = false;
	m_needsToRecord  = (m_vectorCommand.size() != m_usedCommandBufferNumber);
}

/////////////////////////////////////////////////////////////////////////////////////////////

void SceneLightingDeferredTechnique::slotLKeyPressed()
{
	m_active = true;
}

/////////////////////////////////////////////////////////////////////////////////////////////

void SceneLightingDeferredTechnique::slot1KeyPressed()
{
	m_materialSceneLightingDeferred->setIrradianceMultiplier(m_materialSceneLightingDeferred->getIrradianceMultiplier() - 1000.0f);
	cout << "New value of irradianceMultiplier " << m_materialSceneLightingDeferred->getIrradianceMultiplier() << endl;
}

/////////////////////////////////////////////////////////////////////////////////////////////

void SceneLightingDeferredTechnique::slot2KeyPressed()
{
	m_materialSceneLightingDeferred->setIrradianceMultiplier(m_materialSceneLightingDeferred->getIrradianceMultiplier() + 1000.0f);
	cout << "New value of irradianceMultiplier " << m_materialSceneLightingDeferred->getIrradianceMultiplier() << endl;
}

/////////////////////////////////////////////////////////////////////////////////////////////

void SceneLightingDeferredTechnique::slot7KeyPressed()
{
	m_materialSceneLightingDeferred->setDirectIrradianceMultiplier(m_materialSceneLightingDeferred->getDirectIrradianceMultiplier() - 1.0f);
	cout << "New value of directIrradianceMultiplier " << m_materialSceneLightingDeferred->getDirectIrradianceMultiplier() / 1000.0f << endl;
}

/////////////////////////////////////////////////////////////////////////////////////////////

void SceneLightingDeferredTechnique::slot8KeyPressed()
{
	m_materialSceneLightingDeferred->setDirectIrradianceMultiplier(m_materialSceneLightingDeferred->getDirectIrradianceMultiplier() + 1.0f);
	cout << "New value of directIrradianceMultiplier " << m_materialSceneLightingDeferred->getDirectIrradianceMultiplier() / 1000.0f << endl;
}

/////////////////////////////////////////////////////////////////////////////////////////////

void SceneLightingDeferredTechnique::slotNumpad7KeyPressed()
{
	m_materialSceneLightingDeferred->setBrightness(m_materialSceneLightingDeferred->getBrightness() - 0.01f);

	cout << "SceneLightingDeferredTechnique: Brightness value now is " << m_materialSceneLightingDeferred->getBrightness() << endl;
}

/////////////////////////////////////////////////////////////////////////////////////////////

void SceneLightingDeferredTechnique::slotNumpad8KeyPressed()
{
	m_materialSceneLightingDeferred->setBrightness(m_materialSceneLightingDeferred->getBrightness() + 0.01f);

	cout << "SceneLightingDeferredTechnique: Brightness value now is " << m_materialSceneLightingDeferred->getBrightness() << endl;
}

/////////////////////////////////////////////////////////////////////////////////////////////

void SceneLightingDeferredTechnique::slotNumpad4KeyPressed()
{
	m_materialSceneLightingDeferred->setContrast(m_materialSceneLightingDeferred->getContrast() - 0.001f);

	cout << "SceneLightingDeferredTechnique: Contrast value now is " << m_materialSceneLightingDeferred->getContrast() << endl;
}

/////////////////////////////////////////////////////////////////////////////////////////////

void SceneLightingDeferredTechnique::slotNumpad5KeyPressed()
{
	m_materialSceneLightingDeferred->setContrast(m_materialSceneLightingDeferred->getContrast() + 0.001f);

	cout << "SceneLightingDeferredTechnique: Contrast value now is " << m_materialSceneLightingDeferred->getContrast() << endl;
}

/////////////////////////////////////////////////////////////////////////////////////////////

void SceneLightingDeferredTechnique::slotNumpad1KeyPressed()
{
	m_materialSceneLightingDeferred->setSaturation(m_materialSceneLightingDeferred->getSaturation() - 0.01f);

	cout << "SceneLightingDeferredTechnique: Saturation value now is " << m_materialSceneLightingDeferred->getSaturation() << endl;
}

/////////////////////////////////////////////////////////////////////////////////////////////

void SceneLightingDeferredTechnique::slotNumpad2KeyPressed()
{
	m_materialSceneLightingDeferred->setSaturation(m_materialSceneLightingDeferred->getSaturation() + 0.01f);

	cout << "SceneLightingDeferredTechnique: Saturation value now is " << m_materialSceneLightingDeferred->getSaturation() << endl;
}

/////////////////////////////////////////////////////////////////////////////////////////////

void SceneLightingDeferredTechnique::slotNumpad9KeyPressed()
{
	Camera* camera = cameraM->getElement(move(string("maincamera")));
	float fov = camera->getFov();
	camera->setFov(fov - 0.0001f);

	cout << "SceneLightingDeferredTechnique: FOV value now is " << camera->getFov() << endl;
}

/////////////////////////////////////////////////////////////////////////////////////////////

void SceneLightingDeferredTechnique::slotNumpad6KeyPressed()
{
	Camera* camera = cameraM->getElement(move(string("maincamera")));
	float fov = camera->getFov();
	camera->setFov(fov + 0.0001f);

	cout << "SceneLightingDeferredTechnique: FOV value now is " << camera->getFov() << endl;
}

/////////////////////////////////////////////////////////////////////////////////////////////
