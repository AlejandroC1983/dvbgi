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
#include "../../include/rastertechnique/scenevoxelizationtechnique.h"
#include "../../include/material/materialmanager.h"
#include "../../include/shader/shader.h"
#include "../../include/scene/scene.h"
#include "../../include/core/gpupipeline.h"
#include "../../include/uniformbuffer/uniformbuffer.h"
#include "../../include/material/materialscenevoxelization.h"
#include "../../include/material/materialdynamicscenevoxelization.h"
#include "../../include/parameter/attributedefines.h"
#include "../../include/parameter/attributedata.h"
#include "../../include/framebuffer/framebuffer.h"
#include "../../include/framebuffer/framebuffermanager.h"
#include "../../include/util/vulkanstructinitializer.h"
#include "../../include/material/materialcolortexture.h"
#include "../../include/util/mathutil.h"
#include "../../include/component/basecomponent.h"
#include "../../include/component/rendercomponent.h"
#include "../../include/component/dynamiccomponent.h"

// NAMESPACE
using namespace attributedefines;

// DEFINES

// STATIC MEMBER INITIALIZATION
string SceneVoxelizationTechnique::m_voxelizationMaterialSuffix        = "_Voxelization";
string SceneVoxelizationTechnique::m_voxelizationDynamicMaterialSuffix = "_VoxelizationDynamic";

/////////////////////////////////////////////////////////////////////////////////////////////

SceneVoxelizationTechnique::SceneVoxelizationTechnique(string &&name, string&& className) : RasterTechnique(move(name), move(className))
	, m_voxelizedSceneWidth(0)
	, m_voxelizedSceneHeight(0)
	, m_voxelizedSceneDepth(0)
	, m_renderPass(nullptr)
	, m_framebuffer(nullptr)
	, m_voxelFirstIndexBuffer(nullptr)
	, m_fragmentDataBuffer(nullptr)
	, m_voxelizationOutputTexture(nullptr)
	, m_isStaticSceneVoxelization(true)
{
	m_recordPolicy                  = CommandRecordPolicy::CRP_SINGLE_TIME;
	m_neededSemaphoreNumber         = 2;
	int sceneVoxelizationResolution = gpuPipelineM->getRasterFlagValue(move(string("SCENE_VOXELIZATION_RESOLUTION")));
	m_voxelizedSceneWidth           = sceneVoxelizationResolution;
	m_voxelizedSceneHeight          = sceneVoxelizationResolution;
	m_voxelizedSceneDepth           = sceneVoxelizationResolution;

	cout << "Voxelization texture resolution is " << sceneVoxelizationResolution << endl;

	BBox3D& sceneAABB = sceneM->refBox();

	vec3 m;
	vec3 M;

	sceneAABB.getCenteredBoxMinMax(m, M);

	vec3 center         = (m + M) * 0.5f;
	float maxLen        = glm::max(M.x - m.x, glm::max(M.y - m.y, M.z - m.z)) * 0.5f;
	float maxLen2       = maxLen * 2.0f;
	float halfVoxelSize = maxLen * 0.5f;
	m_projection        = MathUtil::orthographicRH(-maxLen, maxLen, -maxLen, maxLen, 0.0f, maxLen2);
	m_projection[1][1] *= -1;
	m_viewX             = lookAt(center - vec3(maxLen, 0.0f,   0.0f),   center, vec3( 0.0f, 1.0f, 0.0f));
	m_viewY             = lookAt(center - vec3(0.0f,   maxLen, 0.0f),   center, vec3(-1.0f, 0.0f, 0.0f));
	m_viewZ             = lookAt(center - vec3(0.0f,   0.0f,   maxLen), center, vec3( 0.0f, 1.0f, 0.0f));
}

/////////////////////////////////////////////////////////////////////////////////////////////

SceneVoxelizationTechnique::~SceneVoxelizationTechnique()
{

}

/////////////////////////////////////////////////////////////////////////////////////////////

void SceneVoxelizationTechnique::prepare(float dt)
{

}

/////////////////////////////////////////////////////////////////////////////////////////////

void SceneVoxelizationTechnique::init()
{
	m_voxelizationOutputTexture = textureM->buildTexture(
		move(string("voxelizationoutputimage")),
		VK_FORMAT_R16G16B16A16_SFLOAT,
		VkExtent3D({ uint32_t(m_voxelizedSceneWidth), uint32_t(m_voxelizedSceneHeight), uint32_t(1) }),
		VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
		VK_IMAGE_ASPECT_COLOR_BIT,
		VK_IMAGE_ASPECT_COLOR_BIT,
		VK_IMAGE_LAYOUT_UNDEFINED,
		VK_IMAGE_LAYOUT_GENERAL,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		VK_SAMPLE_COUNT_8_BIT,
		VK_IMAGE_TILING_OPTIMAL,
		VK_IMAGE_VIEW_TYPE_2D,
		0);

	VkPipelineBindPoint* pipelineBindPoint                         = new VkPipelineBindPoint(VK_PIPELINE_BIND_POINT_GRAPHICS);
	vector<VkFormat>* vectorAttachmentFormat                       = new vector<VkFormat>;
	vector<VkSampleCountFlagBits>* vectorAttachmentSamplesPerPixel = new vector<VkSampleCountFlagBits>;
	vector<VkImageLayout>* vectorAttachmentFinalLayout             = new vector<VkImageLayout>;
	vector<VkAttachmentReference>* vectorColorReference            = new vector<VkAttachmentReference>;
	vectorAttachmentFormat->push_back(VK_FORMAT_R16G16B16A16_SFLOAT);
	vectorAttachmentSamplesPerPixel->push_back(VK_SAMPLE_COUNT_8_BIT);
	vectorAttachmentFinalLayout->push_back(VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
	vectorColorReference->push_back({ 0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL });
	MultiTypeUnorderedMap *attributeUM = new MultiTypeUnorderedMap();
	attributeUM->newElement<AttributeData<VkPipelineBindPoint*>*>(new AttributeData<VkPipelineBindPoint*>(string(g_renderPassAttachmentPipelineBindPoint), move(pipelineBindPoint)));
	attributeUM->newElement<AttributeData<vector<VkFormat>*>*>(new AttributeData<vector<VkFormat>*>(string(g_renderPassAttachmentFormat), move(vectorAttachmentFormat)));
	attributeUM->newElement<AttributeData<vector<VkSampleCountFlagBits>*>*>(new AttributeData<vector<VkSampleCountFlagBits>*>(string(g_renderPassAttachmentSamplesPerPixel), move(vectorAttachmentSamplesPerPixel)));
	attributeUM->newElement<AttributeData<vector<VkImageLayout>*>*>(new AttributeData<vector<VkImageLayout>*>(string(g_renderPassAttachmentFinalLayout), move(vectorAttachmentFinalLayout)));
	attributeUM->newElement<AttributeData<vector<VkAttachmentReference>*>*>(new AttributeData<vector<VkAttachmentReference>*>(string(g_renderPassAttachmentColorReference), move(vectorColorReference)));
	m_renderPass = renderPassM->buildRenderPass(move(string("voxelizationrenderpass")), attributeUM);

	vector<string> arrayAttachment;
	arrayAttachment.push_back(m_voxelizationOutputTexture->getName());
	m_framebuffer = framebufferM->buildFramebuffer(move(string("voxelizationoutputFB")), (uint32_t)(m_voxelizedSceneWidth), (uint32_t)(m_voxelizedSceneHeight), move(string(m_renderPass->getName())), move(arrayAttachment));
}

/////////////////////////////////////////////////////////////////////////////////////////////

VkCommandBuffer* SceneVoxelizationTechnique::record(int currentImage, uint& commandBufferID, CommandBufferType& commandBufferType)
{
	return nullptr;
}

/////////////////////////////////////////////////////////////////////////////////////////////

void SceneVoxelizationTechnique::postCommandSubmit()
{

}

/////////////////////////////////////////////////////////////////////////////////////////////

void SceneVoxelizationTechnique::recordRasterizationCommands(VkCommandBuffer* commandBuffer)
{
	VkRenderPassBeginInfo renderPassBegin = VulkanStructInitializer::renderPassBeginInfo(
		*m_renderPass->refRenderPass(),
		m_framebuffer->getFramebuffer(),
		VkRect2D({ 0, 0, uint32_t(m_voxelizedSceneWidth), uint32_t(m_voxelizedSceneHeight) }),
		m_vectorMaterial[0]->refVectorClearValue());

	vkCmdBeginRenderPass(*commandBuffer, &renderPassBegin, VK_SUBPASS_CONTENTS_INLINE); // Start recording the render pass instance

	const VkDeviceSize offsets[1] = { 0 };
	vkCmdBindVertexBuffers(*commandBuffer, 0, 1, &bufferM->getElement(move(string("vertexBuffer")))->getBuffer(), offsets); // Bound the command buffer with the graphics pipeline
	vkCmdBindIndexBuffer(*commandBuffer, bufferM->getElement(move(string("indexBuffer")))->getBuffer(), 0, VK_INDEX_TYPE_UINT32);

	gpuPipelineM->initViewports(float(m_voxelizedSceneWidth), float(m_voxelizedSceneHeight), 0.0f, 0.0f, 0.0f, 1.0f, commandBuffer);
	gpuPipelineM->initScissors(m_voxelizedSceneWidth, m_voxelizedSceneHeight, 0, 0, commandBuffer);

	

	vectorNodePtr arrayNode;
	vectorNodePtr arrayNodeSkinned;

	if (m_isStaticSceneVoxelization)
	{
		arrayNode = sceneM->getByNodeType(eNodeType::E_NT_STATIC_ELEMENT);
	}
	else
	{
		arrayNode        = sceneM->getByNodeType(eNodeType::E_NT_DYNAMIC_ELEMENT);
		arrayNodeSkinned = sceneM->getByNodeType(eNodeType::E_NT_SKINNEDMESH_ELEMENT);
	}

	drawNodeVector(*commandBuffer, arrayNode);

	if (arrayNodeSkinned.size() > 0)
	{
		vkCmdBindVertexBuffers(*commandBuffer, 0, 1, &bufferM->getElement(move(string("vertexBufferSkinnedMesh")))->getBuffer(), offsets); // Bound the command buffer with the graphics pipeline
		vkCmdBindIndexBuffer(*commandBuffer, bufferM->getElement(move(string("indexBufferSkinnedMesh")))->getBuffer(), 0, VK_INDEX_TYPE_UINT32);
		drawNodeVector(*commandBuffer, arrayNodeSkinned);
	}

	vkCmdEndRenderPass(*commandBuffer);
}

/////////////////////////////////////////////////////////////////////////////////////////////

void SceneVoxelizationTechnique::drawNodeVector(VkCommandBuffer commandBuffer, vectorNodePtr& vectorNode)
{
	if (vectorNode.size() > 0)
	{
		uint dynamicAllignment = materialM->getMaterialUBDynamicAllignment();

		sceneM->sortByMaterial(vectorNode);

		string materialName = vectorNode[0]->refRenderComponent()->refMaterial()->getName();
		materialName       += m_isStaticSceneVoxelization ? m_voxelizationMaterialSuffix : m_voxelizationDynamicMaterialSuffix;
		Material* previous  = materialM->getElement(move(materialName));
		Material* current   = previous;

		vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, current->getPipeline()->getPipeline()); // Bound the command buffer with the graphics pipeline

		bool bindMaterialPipeline = false;

		uint maxIndex = uint(vectorNode.size());

		uint32_t elementIndex;
		uint32_t offsetData[3];
		uint32_t sceneDataBufferOffset = static_cast<uint32_t>(gpuPipelineM->getSceneUniformData()->getDynamicAllignment());
		forI(maxIndex)
		{
			if (bindMaterialPipeline)
			{
				vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, current->getPipeline()->getPipeline()); // Bound the command buffer with the graphics pipeline
				bindMaterialPipeline = false;
			}

			elementIndex = sceneM->getElementIndex(vectorNode[i]);

			RenderComponent* renderComponent = vectorNode[i]->refRenderComponent();

			// Only models that are meant to be rasterized and are not emitters nor debug elements
			if (renderComponent->getMeshType() & eMeshType::E_MT_RENDER_MODEL)
			{
				offsetData[0] = elementIndex * sceneDataBufferOffset;
				offsetData[1] = 0;
				offsetData[2] = static_cast<uint32_t>(current->getMaterialUniformBufferIndex() * dynamicAllignment);
				vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, current->getPipelineLayout(), 0, 1, &current->refDescriptorSet(), 3, &offsetData[0]);
				vkCmdDrawIndexed(commandBuffer, renderComponent->getIndexSize(), 1, renderComponent->getStartIndex(), 0, 0);
			}

			if ((i + 1) < maxIndex)
			{
				previous = current;

				materialName  = vectorNode[i + 1]->refRenderComponent()->refMaterial()->getName();
				materialName += m_isStaticSceneVoxelization ? m_voxelizationMaterialSuffix : m_voxelizationDynamicMaterialSuffix;
				current       = materialM->getElement(move(materialName));

				if (previous != current)
				{
					bindMaterialPipeline = true;
				}
			}
		}
	}
}

/////////////////////////////////////////////////////////////////////////////////////////////

// TODO: refactor with SceneLightingTechnique::generateVoxelizationMaterials
void SceneVoxelizationTechnique::generateVoxelizationMaterials()
{
	// TODO: use descriptor sets to avoid material instances
	vectorNodePtr arrayNode;
	
	if (m_isStaticSceneVoxelization)
	{
		arrayNode = sceneM->getByNodeType(eNodeType::E_NT_STATIC_ELEMENT);
	}
	else
	{
		arrayNode = sceneM->getByNodeType(eNodeType::E_NT_DYNAMIC_ELEMENT | eNodeType::E_NT_SKINNEDMESH_ELEMENT);
	}
	
	const uint maxIndex = uint(arrayNode.size());

	const Node* node;
	const Material* material;
	const string materialName = "MaterialColorTexture";
	const MaterialColorTexture* materialCasted;

	bool isDynamicVoxelizationMaterial = false;

	BBox3D& box = sceneM->refBox();

	vec3 max3D;
	vec3 min3D;
	box.getCenteredBoxMinMax(min3D, max3D);

	vec3 extent3D = max3D - min3D;
	vec4 sceneMin = vec4(min3D.x, min3D.y, min3D.z, 0.0f);
	vec4 sceneExtent = vec4(extent3D.x, extent3D.y, extent3D.z, 0.0);

	forI(maxIndex)
	{
		node     = arrayNode[i];
		material = node->getRenderComponent()->getMaterial();

		if ((material != nullptr) && (material->getClassName() == materialName))
		{
			materialCasted = static_cast<const MaterialColorTexture*>(material);
			string newName = material->getName() + (m_isStaticSceneVoxelization ? m_voxelizationMaterialSuffix : m_voxelizationDynamicMaterialSuffix);

			Material* newMaterial = materialM->getElement(move(string(newName)));

			if (newMaterial == nullptr)
			{
				MultiTypeUnorderedMap* attributeMaterial = new MultiTypeUnorderedMap();
				attributeMaterial->newElement<AttributeData<string>*>(new AttributeData<string>(string(g_reflectanceTextureResourceName), string(materialCasted->getReflectanceTextureName())));
				attributeMaterial->newElement<AttributeData<string>*>(new AttributeData<string>(string(g_normalTextureResourceName), string(materialCasted->getNormalTextureName())));

				if (m_isStaticSceneVoxelization)
				{
					newMaterial = materialM->buildMaterial(move(string("MaterialSceneVoxelization")), move(string(newName)), attributeMaterial);
				}
				else
				{
					newMaterial = materialM->buildMaterial(move(string("MaterialDynamicSceneVoxelization")), move(string(newName)), attributeMaterial);
				}

				if (m_isStaticSceneVoxelization)
				{
					MaterialSceneVoxelization* materialVoxelizationCasted = static_cast<MaterialSceneVoxelization*>(newMaterial);
					materialVoxelizationCasted->setProjection(m_projection);
					materialVoxelizationCasted->setViewX(m_viewX);
					materialVoxelizationCasted->setViewY(m_viewY);
					materialVoxelizationCasted->setViewZ(m_viewZ);
					materialVoxelizationCasted->setVoxelizationSize(vec4(float(m_voxelizedSceneWidth), float(m_voxelizedSceneWidth), float(m_voxelizedSceneWidth), 0.0));
				}
				else
				{
					MaterialDynamicSceneVoxelization* materialVoxelizationDynamicCasted = static_cast<MaterialDynamicSceneVoxelization*>(newMaterial);
					materialVoxelizationDynamicCasted->setProjection(m_projection);
					materialVoxelizationDynamicCasted->setViewX(m_viewX);
					materialVoxelizationDynamicCasted->setViewY(m_viewY);
					materialVoxelizationDynamicCasted->setViewZ(m_viewZ);
					materialVoxelizationDynamicCasted->setVoxelizationSize(vec4(float(m_voxelizedSceneWidth), float(m_voxelizedSceneWidth), float(m_voxelizedSceneWidth), 0.0));
					materialVoxelizationDynamicCasted->setSceneMin(sceneMin);
					materialVoxelizationDynamicCasted->setSceneExtent(sceneExtent);
				}
				
				m_vectorMaterialName.push_back(newName);
				m_vectorMaterial.push_back(newMaterial);
			}
		}
	}
}

/////////////////////////////////////////////////////////////////////////////////////////////
