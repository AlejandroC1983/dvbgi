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
#include "../../include/rastertechnique/gbuffertechnique.h"
#include "../../include/material/materialmanager.h"
#include "../../include/core/coremanager.h"
#include "../../include/scene/scene.h"
#include "../../include/buffer/buffer.h"
#include "../../include/buffer/buffermanager.h"
#include "../../include/texture/texture.h"
#include "../../include/texture/texturemanager.h"
#include "../../include/core/gpupipeline.h"
#include "../../include/uniformbuffer/uniformbuffer.h"
#include "../../include/util/vulkanstructinitializer.h"
#include "../../include/renderpass/renderpass.h"
#include "../../include/renderpass/renderpassmanager.h"
#include "../../include/framebuffer/framebuffer.h"
#include "../../include/framebuffer/framebuffermanager.h"
#include "../../include/parameter/attributedefines.h"
#include "../../include/parameter/attributedata.h"
#include "../../include/material/materialindirectcolortexture.h"
#include "../../include/material/materialcolortexture.h"
#include "../../include/material/material.h"
#include "../../include/material/materialgbuffer.h"
#include "../../include/component/basecomponent.h"
#include "../../include/component/rendercomponent.h"
#include "../../include/rastertechnique/staticscenevoxelizationtechnique.h"

// NAMESPACE
using namespace attributedefines;

// DEFINES

// STATIC MEMBER INITIALIZATION
string GBufferTechnique::m_GBufferMaterialSuffix = "_GBuffer";

/////////////////////////////////////////////////////////////////////////////////////////////

GBufferTechnique::GBufferTechnique(string &&name, string&& className) : RasterTechnique(move(name), move(className))
	, m_indirectCommandBufferMainCamera(nullptr)
	, m_GBufferNormal(nullptr)
	, m_GBufferReflectance(nullptr)
	, m_GBufferPosition(nullptr)
	, m_GBufferDepth(nullptr)
	, m_offscreenWidth(coreM->getWidth())
	, m_offscreenHeight(coreM->getHeight())
{
}

/////////////////////////////////////////////////////////////////////////////////////////////

GBufferTechnique::~GBufferTechnique()
{

}

/////////////////////////////////////////////////////////////////////////////////////////////

void GBufferTechnique::init()
{
	m_arrayNode            = sceneM->getByNodeType(eNodeType::E_NT_STATIC_ELEMENT | eNodeType::E_NT_DYNAMIC_ELEMENT);
	m_arrayNodeSkinnedMesh = sceneM->getByNodeType(eNodeType::E_NT_SKINNEDMESH_ELEMENT);

	m_indirectCommandBufferMainCamera = bufferM->getElement(move(string("indirectCommandBufferMainCamera")));

	m_GBufferNormal = textureM->buildTexture(move(string("GBufferNormal")),
						   VK_FORMAT_R8G8B8A8_UNORM,
						   { m_offscreenWidth, m_offscreenHeight, 1 },
						   VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
						   VK_IMAGE_ASPECT_COLOR_BIT,
						   VK_IMAGE_ASPECT_COLOR_BIT,
						   VK_IMAGE_LAYOUT_UNDEFINED,
						   VK_IMAGE_LAYOUT_GENERAL,
						   VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
						   VK_SAMPLE_COUNT_1_BIT,
						   VK_IMAGE_TILING_OPTIMAL,
						   VK_IMAGE_VIEW_TYPE_2D,
						   0);

	m_GBufferReflectance = textureM->buildTexture(move(string("GBufferReflectance")),
						   VK_FORMAT_R8G8B8A8_UNORM,
						   { m_offscreenWidth, m_offscreenHeight, 1 },
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

	m_GBufferPosition = textureM->buildTexture(move(string("GBufferPosition")),
						   VK_FORMAT_R16G16B16A16_SFLOAT,
						   { m_offscreenWidth, m_offscreenHeight, 1 },
						   VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
						   VK_IMAGE_ASPECT_COLOR_BIT,
						   VK_IMAGE_ASPECT_COLOR_BIT,
						   VK_IMAGE_LAYOUT_UNDEFINED,
						   VK_IMAGE_LAYOUT_GENERAL,
						   VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
						   VK_SAMPLE_COUNT_1_BIT,
						   VK_IMAGE_TILING_OPTIMAL,
						   VK_IMAGE_VIEW_TYPE_2D,
						   0);

	m_GBufferDepth = textureM->buildTexture(move(string("GBufferDepth")),
						   VK_FORMAT_D24_UNORM_S8_UINT,
						   { m_offscreenWidth, m_offscreenHeight, 1 },
						   VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
						   VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT,
						   VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT,
						   VK_IMAGE_LAYOUT_UNDEFINED,
						   VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
						   VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
						   VK_SAMPLE_COUNT_1_BIT,
						   VK_IMAGE_TILING_OPTIMAL,
						   VK_IMAGE_VIEW_TYPE_2D,
						   0);

	VkAttachmentReference* depthReference  = new VkAttachmentReference({ 3, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL });
	VkPipelineBindPoint* pipelineBindPoint = new VkPipelineBindPoint(VK_PIPELINE_BIND_POINT_GRAPHICS);

	vector<VkFormat>* vectorAttachmentFormat = new vector<VkFormat>;
	vectorAttachmentFormat->push_back(VK_FORMAT_R8G8B8A8_UNORM);
	vectorAttachmentFormat->push_back(VK_FORMAT_R8G8B8A8_UNORM);
	vectorAttachmentFormat->push_back(VK_FORMAT_R16G16B16A16_SFLOAT);
	vectorAttachmentFormat->push_back(VK_FORMAT_D24_UNORM_S8_UINT);

	vector<VkSampleCountFlagBits>* vectorAttachmentSamplesPerPixel = new vector<VkSampleCountFlagBits>;
	vectorAttachmentSamplesPerPixel->push_back(VK_SAMPLE_COUNT_1_BIT);
	vectorAttachmentSamplesPerPixel->push_back(VK_SAMPLE_COUNT_1_BIT);
	vectorAttachmentSamplesPerPixel->push_back(VK_SAMPLE_COUNT_1_BIT);
	vectorAttachmentSamplesPerPixel->push_back(VK_SAMPLE_COUNT_1_BIT);

	vector<VkImageLayout>* vectorAttachmentFinalLayout = new vector<VkImageLayout>;
	vectorAttachmentFinalLayout->push_back(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
	vectorAttachmentFinalLayout->push_back(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
	vectorAttachmentFinalLayout->push_back(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
	vectorAttachmentFinalLayout->push_back(VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);

	vector<VkAttachmentReference>* vectorColorReference = new vector<VkAttachmentReference>;
	vectorColorReference->push_back({ 0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL });
	vectorColorReference->push_back({ 1, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL });
	vectorColorReference->push_back({ 2, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL });

	MultiTypeUnorderedMap *attributeUM = new MultiTypeUnorderedMap();
	attributeUM->newElement<AttributeData<VkAttachmentReference*>*>        (new AttributeData<VkAttachmentReference*>        (string(g_renderPassAttachmentDepthReference),    move(depthReference)));
	attributeUM->newElement<AttributeData<VkPipelineBindPoint*>*>          (new AttributeData<VkPipelineBindPoint*>          (string(g_renderPassAttachmentPipelineBindPoint), move(pipelineBindPoint)));
	attributeUM->newElement<AttributeData<vector<VkFormat>*>*>             (new AttributeData<vector<VkFormat>*>             (string(g_renderPassAttachmentFormat),            move(vectorAttachmentFormat)));
	attributeUM->newElement<AttributeData<vector<VkSampleCountFlagBits>*>*>(new AttributeData<vector<VkSampleCountFlagBits>*>(string(g_renderPassAttachmentSamplesPerPixel),   move(vectorAttachmentSamplesPerPixel)));
	attributeUM->newElement<AttributeData<vector<VkImageLayout>*>*>        (new AttributeData<vector<VkImageLayout>*>        (string(g_renderPassAttachmentFinalLayout),       move(vectorAttachmentFinalLayout)));
	attributeUM->newElement<AttributeData<vector<VkAttachmentReference>*>*>(new AttributeData<vector<VkAttachmentReference>*>(string(g_renderPassAttachmentColorReference),    move(vectorColorReference)));

	m_renderPass = renderPassM->buildRenderPass(move(string("gbufferrenderpass")), attributeUM);

	vector<string> arrayAttachment;
	arrayAttachment.resize(4);
	arrayAttachment[0] = m_GBufferNormal->getName();
	arrayAttachment[1] = m_GBufferReflectance->getName();
	arrayAttachment[2] = m_GBufferPosition->getName();
	arrayAttachment[3] = m_GBufferDepth->getName();

	m_framebuffer = framebufferM->buildFramebuffer(move(string("gbufferFB")), m_offscreenWidth, m_offscreenHeight, move(string("gbufferrenderpass")), move(arrayAttachment));

	m_vectorGBufferClearValue.resize(4);
	m_vectorGBufferClearValue[0].color        = { { 0.0f, 0.0f, 0.0f, 0.0f } };
	m_vectorGBufferClearValue[1].color        = { { 0.0f, 0.0f, 0.0f, 0.0f } };
	m_vectorGBufferClearValue[2].color        = { { 0.0f, 0.0f, 0.0f, 0.0f } };
	m_vectorGBufferClearValue[3].depthStencil = { 1.0f, 0 };

	generateGBufferMaterials();
}

/////////////////////////////////////////////////////////////////////////////////////////////

void GBufferTechnique::prepare(float dt)
{

}

/////////////////////////////////////////////////////////////////////////////////////////////

VkCommandBuffer* GBufferTechnique::record(int currentImage, uint& commandBufferID, CommandBufferType& commandBufferType)
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
		VkRect2D({ 0, 0, m_offscreenWidth, m_offscreenHeight }),
		m_vectorGBufferClearValue);

	vkCmdBeginRenderPass(*commandBuffer, &renderPassBegin, VK_SUBPASS_CONTENTS_INLINE); // Start recording the render pass instance

	const VkDeviceSize offsets[1] = { 0 };
	Buffer* instanceDataBuffer = bufferM->getElement(move(string("instanceDataBuffer")));
	vkCmdBindVertexBuffers(*commandBuffer, 0, 1, &bufferM->getElement(move(string("vertexBuffer")))->getBuffer(), offsets);
	vkCmdBindVertexBuffers(*commandBuffer, 1, 1, &instanceDataBuffer->getBuffer(), offsets);
	vkCmdBindIndexBuffer(*commandBuffer, bufferM->getElement(move(string("indexBuffer")))->getBuffer(), 0, VK_INDEX_TYPE_UINT32);

	gpuPipelineM->initViewports((float)m_offscreenWidth, (float)m_offscreenHeight, 0.0f, 0.0f, 0.0f, 1.0f, commandBuffer);
	gpuPipelineM->initScissors(m_offscreenWidth, m_offscreenHeight, 0, 0, commandBuffer);

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

void GBufferTechnique::postCommandSubmit()
{
	m_executeCommand = false;
	m_needsToRecord  = (m_vectorCommand.size() != m_usedCommandBufferNumber);
	m_signalGBuffer.emit();
}

/////////////////////////////////////////////////////////////////////////////////////////////

void GBufferTechnique::drawIndirectNodeVector(VkCommandBuffer commandBuffer, const vectorNodePtr& vectorNode)
{
	// TODO: Sort by material attributes and avoid duplicated API calls when possible
	uint dynamicAllignment = materialM->getMaterialUBDynamicAllignment();

	forI(vectorNode.size())
	{
		string materialName                = vectorNode[i]->refRenderComponent()->refMaterial()->getName();
		materialName                      += m_GBufferMaterialSuffix;
		MaterialGBuffer* currentMaterial   = static_cast<MaterialGBuffer*>(materialM->getElement(move(materialName)));

		vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, currentMaterial->getPipeline()->getPipeline()); // Bound the command buffer with the graphics pipeline

		uint32_t elementIndex;
		uint32_t offsetData[3];
		uint32_t sceneDataBufferOffset = static_cast<uint32_t>(gpuPipelineM->getSceneUniformData()->getDynamicAllignment());

		elementIndex = sceneM->getElementIndex(vectorNode[i]);

		offsetData[0] = elementIndex * sceneDataBufferOffset;
		offsetData[1] = 0;
		offsetData[2] = static_cast<uint32_t>(currentMaterial->getMaterialUniformBufferIndex() * dynamicAllignment);

		vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, currentMaterial->getPipelineLayout(), 0, 1, &currentMaterial->refDescriptorSet(), 3, &offsetData[0]);
		vkCmdDrawIndexedIndirect(commandBuffer, m_indirectCommandBufferMainCamera->getBuffer(), sceneM->getElementIndex(vectorNode[i]) * sizeof(VkDrawIndexedIndirectCommand), 1, sizeof(VkDrawIndexedIndirectCommand));
	}
}

/////////////////////////////////////////////////////////////////////////////////////////////

// TODO: refactor with StaticSceneVoxelizationTechnique::generateVoxelizationMaterials
void GBufferTechnique::generateGBufferMaterials()
{
	StaticSceneVoxelizationTechnique* sceneVoxelizationTechnique = static_cast<StaticSceneVoxelizationTechnique*>(gpuPipelineM->getRasterTechniqueByName(move(string("StaticSceneVoxelizationTechnique"))));

	BBox3D& box = sceneM->refBox();

	vec3 max3D;
	vec3 min3D;
	box.getCenteredBoxMinMax(min3D, max3D);

	// TODO: use descriptor sets to avoid material instances
	vec3 extent3D                    = max3D - min3D;
	vec4 sceneMin                    = vec4(min3D.x, min3D.y, min3D.z, 0.0f);
	vec4 sceneExtentVoxelizationSize = vec4(extent3D.x, extent3D.y, extent3D.z, float(sceneVoxelizationTechnique->getVoxelizedSceneWidth()));
	const vectorNodePtr arrayNode    = sceneM->getByMeshType(eMeshType::E_MT_RENDER_MODEL);
	const uint maxIndex              = uint(arrayNode.size());

	const Node* node;
	const Material* material;
	const string materialName = "MaterialColorTexture";
	const MaterialColorTexture* materialCasted;

	vectorString vectorMaterialName;
	vectorMaterialPtr vectorMaterial;

	forI(maxIndex)
	{
		node     = arrayNode[i];
		material = node->getRenderComponent()->getMaterial();

		if ((material != nullptr) && (material->getClassName() == materialName))
		{
			materialCasted               = static_cast<const MaterialColorTexture*>(material);
			string newName               = material->getName() + m_GBufferMaterialSuffix;
			MaterialGBuffer* newMaterial = static_cast<MaterialGBuffer*>(materialM->getElement(move(string(newName))));

			if (newMaterial == nullptr)
			{
				MultiTypeUnorderedMap* attributeMaterial = new MultiTypeUnorderedMap();
				attributeMaterial->newElement<AttributeData<string>*>(new AttributeData<string>(string(g_reflectanceTextureResourceName), string(materialCasted->getReflectanceTextureName())));
				attributeMaterial->newElement<AttributeData<string>*>(new AttributeData<string>(string(g_normalTextureResourceName), string(materialCasted->getNormalTextureName())));
				newMaterial = static_cast<MaterialGBuffer*>(materialM->buildMaterial(move(string("MaterialGBuffer")), move(string(newName)), attributeMaterial));
				newMaterial->setSceneMin(sceneMin);
				newMaterial->setSceneExtentVoxelizationSize(sceneExtentVoxelizationSize);
				vectorMaterialName.push_back(newName);
				vectorMaterial.push_back(newMaterial);
				
			}
		}
	}

	m_vectorMaterialName = vectorMaterialName;
	m_vectorMaterial     = vectorMaterial;
}

/////////////////////////////////////////////////////////////////////////////////////////////
