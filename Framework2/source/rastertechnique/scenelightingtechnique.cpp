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
#include "../../include/rastertechnique/scenelightingtechnique.h"
#include "../../include/material/materialmanager.h"
#include "../../include/scene/scene.h"
#include "../../include/buffer/buffer.h"
#include "../../include/buffer/buffermanager.h"
#include "../../include/uniformbuffer/uniformbuffer.h"
#include "../../include/material/materialcolortexture.h"
#include "../../include/material/materiallighting.h"
#include "../../include/util/vulkanstructinitializer.h"
#include "../../include/core/coremanager.h"
#include "../../include/parameter/attributedefines.h"
#include "../../include/parameter/attributedata.h"
#include "../../include/rastertechnique/staticscenevoxelizationtechnique.h"
#include "../../include/rastertechnique/litvoxeltechnique.h"
#include "../../include/camera/camera.h"
#include "../../include/camera/cameramanager.h"
#include "../../include/renderpass/renderpass.h"
#include "../../include/renderpass/renderpassmanager.h"
#include "../../include/framebuffer/framebuffer.h"
#include "../../include/framebuffer/framebuffermanager.h"
#include "../../include/component/basecomponent.h"
#include "../../include/component/rendercomponent.h"

// NAMESPACE
using namespace attributedefines;

// DEFINES

// STATIC MEMBER INITIALIZATION
string SceneLightingTechnique::m_lightingMaterialSuffix = "_Lighting";

/////////////////////////////////////////////////////////////////////////////////////////////

SceneLightingTechnique::SceneLightingTechnique(string &&name, string&& className) :
	  RasterTechnique(move(name), move(className))
	, m_renderTargetColor(nullptr)
	, m_renderTargetDepth(nullptr)
	, m_renderPass(nullptr)
	, m_framebuffer(nullptr)
	, m_indirectCommandBufferMainCamera(nullptr)
{
	m_active = false;

	m_vectorTexture.resize(12);
}

/////////////////////////////////////////////////////////////////////////////////////////////

SceneLightingTechnique::~SceneLightingTechnique()
{

}

/////////////////////////////////////////////////////////////////////////////////////////////

void SceneLightingTechnique::init()
{
	inputM->refEventSinglePressSignalSlot().addKeyDownSignal(KeyCode::KEY_CODE_L);
	SignalVoid* signalAdd = inputM->refEventSinglePressSignalSlot().refKeyDownSignalByKey(KeyCode::KEY_CODE_L);
	signalAdd->connect<SceneLightingTechnique, &SceneLightingTechnique::slotLKeyPressed>(this);

	m_indirectCommandBufferMainCamera = bufferM->getElement(move(string("indirectCommandBufferMainCamera")));
	m_arrayNode                       = sceneM->getByNodeType(eNodeType::E_NT_STATIC_ELEMENT | eNodeType::E_NT_DYNAMIC_ELEMENT);
	m_arrayNodeSkinnedMesh            = sceneM->getByNodeType(eNodeType::E_NT_SKINNEDMESH_ELEMENT);

	vector<uint> vectorData;
	vectorData.resize(36000000);
	memset(vectorData.data(), 0, vectorData.size() * size_t(sizeof(uint)));

	bufferM->buildBuffer(
		move(string("debugBuffer")),
		vectorData.data(),
		vectorData.size() * sizeof(uint),
		VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

	uint debugValue = 0;
	bufferM->buildBuffer(
		move(string("debugFragmentCounterBuffer")),
		(void*)(&debugValue),
		4,
		VK_BUFFER_USAGE_STORAGE_BUFFER_BIT  | VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

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
	};

	forI(m_vectorTexture.size())
	{
		m_vectorTexture[i] = textureM->getElement(move(vectorTextureName[i]));
	}

	m_litVoxelTechnique = static_cast<LitVoxelTechnique*>(gpuPipelineM->getRasterTechniqueByName(move(string("LitVoxelTechnique"))));
	m_renderTargetColor   = textureM->getElement(move(string("scenelightingcolor")));
	m_renderTargetDepth   = textureM->getElement(move(string("scenelightingdepth")));
	m_renderPass          = renderPassM->getElement(move(string("scenelightingrenderpass")));
	m_framebuffer         = framebufferM->getElement(move(string("scenelightingrenderpassFB")));

	generateLightingMaterials();

	inputM->refEventSinglePressSignalSlot().addKeyDownSignal(KeyCode::KEY_CODE_1);
	signalAdd = inputM->refEventSinglePressSignalSlot().refKeyDownSignalByKey(KeyCode::KEY_CODE_1);
	signalAdd->connect<SceneLightingTechnique, &SceneLightingTechnique::slot1KeyPressed>(this);

	inputM->refEventSinglePressSignalSlot().addKeyDownSignal(KeyCode::KEY_CODE_2);
	signalAdd = inputM->refEventSinglePressSignalSlot().refKeyDownSignalByKey(KeyCode::KEY_CODE_2);
	signalAdd->connect<SceneLightingTechnique, &SceneLightingTechnique::slot2KeyPressed>(this);

	inputM->refEventSinglePressSignalSlot().addKeyDownSignal(KeyCode::KEY_CODE_7);
	signalAdd = inputM->refEventSinglePressSignalSlot().refKeyDownSignalByKey(KeyCode::KEY_CODE_7);
	signalAdd->connect<SceneLightingTechnique, &SceneLightingTechnique::slot7KeyPressed>(this);

	inputM->refEventSinglePressSignalSlot().addKeyDownSignal(KeyCode::KEY_CODE_8);
	signalAdd = inputM->refEventSinglePressSignalSlot().refKeyDownSignalByKey(KeyCode::KEY_CODE_8);
	signalAdd->connect<SceneLightingTechnique, &SceneLightingTechnique::slot8KeyPressed>(this);
}

/////////////////////////////////////////////////////////////////////////////////////////////

void SceneLightingTechnique::prepare(float dt)
{
	// TODO: automatize this
	const uint maxIndex         = uint(m_vectorMaterial.size());
	Camera* shadowMappingCamera = cameraM->getElement(move(string("emitter")));
	vec3 position               = shadowMappingCamera->getPosition();
	mat4 viewprojectionMatrix   = shadowMappingCamera->getProjection() * shadowMappingCamera->getView();
	vec4 offsetAndSize          = vec4(256.0f, 512.0f, 256.0f, 1024.0f);
	vec3 cameraPosition         = m_litVoxelTechnique->getCameraPosition();
	vec3  cameraForward         = m_litVoxelTechnique->getCameraForward();
	float emitterRadiance       = m_litVoxelTechnique->getEmitterRadiance();

	MaterialLighting* materialCasted = nullptr;
	forI(maxIndex)
	{
		materialCasted = static_cast<MaterialLighting*>(m_vectorMaterial[i]);
		materialCasted->setShadowViewProjection(viewprojectionMatrix);
		materialCasted->setLightPosition(vec4(position.x, position.y, position.z, 0.0f));
		materialCasted->setOffsetAndSize(offsetAndSize);
		materialCasted->setLightForwardEmitterRadiance(vec4(cameraForward.x, cameraForward.y, cameraForward.z, emitterRadiance));
	}
}

/////////////////////////////////////////////////////////////////////////////////////////////

VkCommandBuffer* SceneLightingTechnique::record(int currentImage, uint& commandBufferID, CommandBufferType& commandBufferType)
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

	VkMemoryBarrier memoryBarrier = { VK_STRUCTURE_TYPE_MEMORY_BARRIER, nullptr, VK_ACCESS_SHADER_WRITE_BIT | VK_ACCESS_SHADER_READ_BIT, VK_ACCESS_SHADER_WRITE_BIT | VK_ACCESS_SHADER_READ_BIT };
	vkCmdPipelineBarrier(*commandBuffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 1, &memoryBarrier, 0, nullptr, 0, nullptr);

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

	if (m_arrayNodeSkinnedMesh.size() > 0)
	{
		// TODO: Review how many of this barriers could be removed
		VulkanStructInitializer::insertBufferMemoryBarrier(bufferM->getElement(move(string("vertexBufferSkinnedMesh"))),
														   VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT,
														   VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT,
														   VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
														   VK_PIPELINE_STAGE_VERTEX_SHADER_BIT,
														   commandBuffer);
	}

	MaterialLighting* material = static_cast<MaterialLighting*>(m_vectorMaterial[0]);

	VkRenderPassBeginInfo renderPassBegin = VulkanStructInitializer::renderPassBeginInfo(
		m_renderPass->getRenderPass(),
		m_framebuffer->getFramebuffer(),
		VkRect2D({ 0, 0, coreM->getWidth(), coreM->getHeight() }),
		material->refVectorClearValue());

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

void SceneLightingTechnique::postCommandSubmit()
{
	m_executeCommand = false;
	m_needsToRecord  = (m_vectorCommand.size() != m_usedCommandBufferNumber);
}

/////////////////////////////////////////////////////////////////////////////////////////////

void SceneLightingTechnique::slotLKeyPressed()
{
	m_active = true;
}

/////////////////////////////////////////////////////////////////////////////////////////////

// TODO: refactor with StaticSceneVoxelizationTechnique::generateVoxelizationMaterials
void SceneLightingTechnique::generateLightingMaterials()
{
	StaticSceneVoxelizationTechnique* technique = static_cast<StaticSceneVoxelizationTechnique*>(gpuPipelineM->getRasterTechniqueByName(move(string("StaticSceneVoxelizationTechnique"))));
	vec4 voxelSize                        = vec4(float(technique->getVoxelizedSceneWidth()), float(technique->getVoxelizedSceneHeight()), float(technique->getVoxelizedSceneHeight()), 0.0f);

	vec3 min3D;
	vec3 max3D;
	BBox3D& box = sceneM->refBox();

	box.getCenteredBoxMinMax(min3D, max3D);

	vec3 extent3D = max3D - min3D;
	vec4 extent   = vec4(extent3D.x, extent3D.y, extent3D.z, 0.0f);
	vec4 min      = vec4(min3D.x,    min3D.y,    min3D.z,    0.0f);
	vec4 max      = vec4(max3D.x,    max3D.y,    max3D.z,    0.0f);

	// TODO: use descriptor sets to avoid material instances
	const vectorNodePtr arrayNode = sceneM->getByMeshType(eMeshType::E_MT_RENDER_MODEL);
	const uint maxIndex = uint(arrayNode.size());

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
			materialCasted        = static_cast<const MaterialColorTexture*>(material);
			string newName        = material->getName() + m_lightingMaterialSuffix;
			Material* newMaterial = materialM->getElement(move(string(newName)));

			if (newMaterial == nullptr)
			{
				MultiTypeUnorderedMap *attributeMaterial = new MultiTypeUnorderedMap();
				attributeMaterial->newElement<AttributeData<string>*>(new AttributeData<string>(string(g_reflectanceTextureResourceName), string(materialCasted->getReflectanceTextureName())));
				attributeMaterial->newElement<AttributeData<string>*>(new AttributeData<string>(string(g_normalTextureResourceName), string(materialCasted->getNormalTextureName())));

				MaterialSurfaceType surfaceType = materialCasted->getMaterialSurfaceType();
				if (surfaceType != MaterialSurfaceType::MST_OPAQUE)
				{
					attributeMaterial->newElement<AttributeData<MaterialSurfaceType>*>(new AttributeData<MaterialSurfaceType>(string(g_materialSurfaceType), move(surfaceType)));
				}

				newMaterial = materialM->buildMaterial(move(string("MaterialLighting")), move(string(newName)), attributeMaterial);
				vectorMaterialName.push_back(newName);
				vectorMaterial.push_back(newMaterial);
				MaterialLighting* newMaterialCasted = static_cast<MaterialLighting*>(newMaterial);
				newMaterialCasted->setVoxelSize(voxelSize);
				newMaterialCasted->setSceneMin(min);
				newMaterialCasted->setSceneExtent(extent);
				newMaterialCasted->setZFar(vec4(ZFAR, 0.0f, 0.0f, 0.0f));
				newMaterialCasted->setIrradianceFieldGridDensity(0);
			}
		}
	}

	m_vectorMaterialName = vectorMaterialName;
	m_vectorMaterial     = vectorMaterial;
}

/////////////////////////////////////////////////////////////////////////////////////////////

void SceneLightingTechnique::slot1KeyPressed()
{
	const uint maxIndex = uint(m_vectorMaterial.size());
	MaterialLighting* materialCasted = nullptr;

	forI(maxIndex)
	{
		materialCasted = static_cast<MaterialLighting*>(m_vectorMaterial[i]);
		materialCasted->setIrradianceMultiplier(materialCasted->getIrradianceMultiplier() - 1000.0f);

		if (i == 0)
		{
			cout << "New value of irradianceMultiplier " << materialCasted->getIrradianceMultiplier() << endl;
		}
	}
}

/////////////////////////////////////////////////////////////////////////////////////////////

void SceneLightingTechnique::slot2KeyPressed()
{
	const uint maxIndex = uint(m_vectorMaterial.size());
	MaterialLighting* materialCasted = nullptr;

	forI(maxIndex)
	{
		materialCasted = static_cast<MaterialLighting*>(m_vectorMaterial[i]);
		materialCasted->setIrradianceMultiplier(materialCasted->getIrradianceMultiplier() + 1000.0f);

		if (i == 0)
		{
			cout << "New value of irradianceMultiplier " << materialCasted->getIrradianceMultiplier() << endl;
		}
	}
}

/////////////////////////////////////////////////////////////////////////////////////////////

void SceneLightingTechnique::slot7KeyPressed()
{
	const uint maxIndex = uint(m_vectorMaterial.size());
	MaterialLighting* materialCasted = nullptr;

	forI(maxIndex)
	{
		materialCasted = static_cast<MaterialLighting*>(m_vectorMaterial[i]);
		materialCasted->setDirectIrradianceMultiplier(materialCasted->getDirectIrradianceMultiplier() - 100.0f);

		if (i == 0)
		{
			cout << "New value of directIrradianceMultiplier " << materialCasted->getDirectIrradianceMultiplier() << endl;
		}
	}
}

/////////////////////////////////////////////////////////////////////////////////////////////

void SceneLightingTechnique::slot8KeyPressed()
{
	const uint maxIndex = uint(m_vectorMaterial.size());
	MaterialLighting* materialCasted = nullptr;

	forI(maxIndex)
	{
		materialCasted = static_cast<MaterialLighting*>(m_vectorMaterial[i]);
		materialCasted->setDirectIrradianceMultiplier(materialCasted->getDirectIrradianceMultiplier() + 100.0f);

		if (i == 0)
		{
			cout << "New value of directIrradianceMultiplier " << materialCasted->getDirectIrradianceMultiplier() << endl;
		}
	}
}

/////////////////////////////////////////////////////////////////////////////////////////////

void SceneLightingTechnique::drawIndirectNodeVector(VkCommandBuffer commandBuffer, const vectorNodePtr& vectorNode)
{
	// TODO: Sort by material attributes and avoid duplicated API calls when possible
	uint dynamicAllignment = materialM->getMaterialUBDynamicAllignment();

	forI(vectorNode.size())
	{
		string materialName                = vectorNode[i]->refRenderComponent()->refMaterial()->getName();
		materialName                      += m_lightingMaterialSuffix;
		MaterialLighting* currentMaterial  = static_cast<MaterialLighting*>(materialM->getElement(move(materialName)));

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
