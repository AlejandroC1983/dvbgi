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
#include "../../include/rastertechnique/lightbouncevoxelirradiancetechnique.h"
#include "../../include/buffer/buffer.h"
#include "../../include/buffer/buffermanager.h"
#include "../../include/material/materiallightbouncevoxelirradiance.h"
#include "../../include/core/coremanager.h"
#include "../../include/parameter/attributedefines.h"
#include "../../include/parameter/attributedata.h"
#include "../../include/scene/scene.h"
#include "../../include/rastertechnique/staticscenevoxelizationtechnique.h"
#include "../../include/rastertechnique/bufferprefixsumtechnique.h"
#include "../../include/rastertechnique/litvoxeltechnique.h"
#include "../../include/camera/camera.h"
#include "../../include/camera/cameramanager.h"
#include "../../include/uniformbuffer/uniformbuffer.h"
#include "../../include/util/vulkanstructinitializer.h"
#include "../../include/rastertechnique/resetlitvoxeldatatechnique.h"
#include "../../include/rastertechnique/processcameravisibleresultstechnique.h"
#include "../../include/rastertechnique/dynamicvoxelvisibilityraytracingtechnique.h"
#include "../../include/rastertechnique/bufferprocesstechnique.h"
#include "../../include/material/materiallightbouncestaticvoxelpadding.h"
#include "../../include/material/materiallightbouncestaticvoxelfiltering.h"
#include "../../include/util/mathutil.h"

// NAMESPACE
using namespace attributedefines;

// DEFINES

// STATIC MEMBER INITIALIZATION

/////////////////////////////////////////////////////////////////////////////////////////////

LightBounceVoxelIrradianceTechnique::LightBounceVoxelIrradianceTechnique(string &&name, string&& className) : BufferProcessTechnique(move(name), move(className))
	, m_techniquePrefixSum(nullptr)
	, m_litVoxelTechnique(nullptr)
	, m_resetLitvoxelData(nullptr)
	, m_lightBounceVoxelIrradianceBuffer(nullptr)
	, m_lightBounceVoxelDebugBuffer(nullptr)
	, m_lightBounceIndirectLitCounterBuffer(nullptr)
	, m_debugCounterBuffer(nullptr)
	, m_numOccupiedVoxel(0)
	, m_prefixSumCompleted(false)
	, m_mainCamera(nullptr)
	, m_cameraVisibleVoxelNumber(0)
	, m_lightBounceIndirectLitCounter(0)
	, m_lightBounceVoxelGaussianFilterDebugBuffer(nullptr)
	, m_processCameraVisibleResultsTechnique(nullptr)
	, m_materialLightBounceStaticVoxelPadding(nullptr)
	, m_materialLightBounceStaticVoxelFiltering(nullptr)
	, m_dynamicVoxelVisibilityRayTracingTechnique(nullptr)
	, m_staticVoxelFilteringDebugBuffer(nullptr)
	, m_rayDirectionBuffer(nullptr)
{
	m_numElementPerLocalWorkgroupThread = 1;
	m_numThreadPerLocalWorkgroup        = 128;
	m_active                            = false;
	m_needsToRecord                     = false;
	m_executeCommand                    = false;
	m_rasterTechniqueType               = RasterTechniqueType::RTT_COMPUTE;
	m_computeHostSynchronize            = true;
}

/////////////////////////////////////////////////////////////////////////////////////////////

void LightBounceVoxelIrradianceTechnique::init()
{
	m_lightBounceVoxelDebugBuffer = bufferM->buildBuffer(
		move(string("lightBounceVoxelDebugBuffer")),
		nullptr,
		256,
		VK_BUFFER_USAGE_STORAGE_BUFFER_BIT  | VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

	m_lightBounceIndirectLitCounterBuffer = bufferM->buildBuffer(
		move(string("lightBounceIndirectLitCounterBuffer")),
		(void*)(&m_lightBounceIndirectLitCounter),
		4,
		VK_BUFFER_USAGE_STORAGE_BUFFER_BIT  | VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

	m_debugCounterBuffer = bufferM->buildBuffer(
		move(string("debugCounterBuffer")),
		(void*)(&m_lightBounceIndirectLitCounter),
		4,
		VK_BUFFER_USAGE_STORAGE_BUFFER_BIT  | VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

	m_lightBounceVoxelGaussianFilterDebugBuffer = bufferM->buildBuffer(
		move(string("lightBounceVoxelGaussianFilterDebugBuffer")),
		nullptr,
		10000,
		VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

	m_staticVoxelFilteringDebugBuffer = bufferM->buildBuffer(
		move(string("staticVoxelFilteringDebugBuffer")),
		nullptr,
		1000000,
		VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

	buildRayDirectionBuffer();

	buildTriangulatedIndicesBuffer();

	StaticSceneVoxelizationTechnique* sceneVoxelizationTechnique = static_cast<StaticSceneVoxelizationTechnique*>(gpuPipelineM->getRasterTechniqueByName(move(string("StaticSceneVoxelizationTechnique"))));
	int voxelizedSceneWidth = sceneVoxelizationTechnique->getVoxelizedSceneWidth();

	// Shader storage buffer with the indices of the elements present in the buffer litHiddenVoxelBuffer
	m_lightBounceVoxelIrradianceBuffer = bufferM->getElement(move(string("lightBounceVoxelIrradianceBuffer")));

	// Assuming each thread will take care of a whole row / column
	// TODO: port per-material and inherit from a compute-focused material
	buildShaderThreadMapping();

	MultiTypeUnorderedMap* attributeMaterial = new MultiTypeUnorderedMap();
	attributeMaterial->newElement<AttributeData<string>*>(new AttributeData<string>(string(g_lightBounceVoxelIrradianceCodeChunk), string(m_computeShaderThreadMapping)));
	m_material = materialM->buildMaterial(move(string("MaterialLightBounceVoxelIrradiance")), move(string("MaterialLightBounceVoxelIrradiance")), attributeMaterial);
	m_vectorMaterialName.push_back("MaterialLightBounceVoxelIrradiance");
	m_vectorMaterial.push_back(m_material);
	
	BBox3D& box = sceneM->refBox();
	
	vec3 max3D;
	vec3 min3D;
	box.getCenteredBoxMinMax(min3D, max3D);
	
	vec3 extent3D             = max3D - min3D;
	m_sceneMin                = vec4(min3D.x,    min3D.y,    min3D.z, 0.0f);
	m_sceneExtent             = vec4(extent3D.x, extent3D.y, extent3D.z, float(voxelizedSceneWidth));

	MaterialLightBounceVoxelIrradiance* materialCasted = static_cast<MaterialLightBounceVoxelIrradiance*>(m_material);
	materialCasted->setSceneExtentAndVoxelSize(m_sceneExtent);

	m_litVoxelTechnique  = static_cast<LitVoxelTechnique*>(gpuPipelineM->getRasterTechniqueByName(move(string("LitVoxelTechnique"))));
	m_resetLitvoxelData  = static_cast<ResetLitvoxelData*>(gpuPipelineM->getRasterTechniqueByName(move(string("ResetLitvoxelData"))));

	m_techniquePrefixSum = static_cast<BufferPrefixSumTechnique*>(gpuPipelineM->getRasterTechniqueByName(move(string("BufferPrefixSumTechnique"))));
	m_techniquePrefixSum->refPrefixSumComplete().connect<LightBounceVoxelIrradianceTechnique, &LightBounceVoxelIrradianceTechnique::slotPrefixSumComplete>(this);

	m_mainCamera = cameraM->getElement(move(string("maincamera")));

	m_processCameraVisibleResultsTechnique = static_cast<ProcessCameraVisibleResultsTechnique*>(gpuPipelineM->getRasterTechniqueByName(move(string("ProcessCameraVisibleResultsTechnique"))));

	MultiTypeUnorderedMap* attributeMaterialFilter = new MultiTypeUnorderedMap();
	attributeMaterialFilter->newElement<AttributeData<string>*>(new AttributeData<string>(string(g_lightBounceVoxelGaussianFilterCodeChunk), string(m_computeShaderThreadMapping)));

	m_materialLightBounceStaticVoxelPadding = static_cast<MaterialLightBounceStaticVoxelPadding*>(materialM->buildMaterial(move(string("MaterialLightBounceStaticVoxelPadding")), move(string("MaterialLightBounceStaticVoxelPadding")), nullptr));
	m_vectorMaterialName.push_back("MaterialLightBounceStaticVoxelPadding");
	m_vectorMaterial.push_back(m_materialLightBounceStaticVoxelPadding);
	m_materialLightBounceStaticVoxelPadding->setVoxelizationSize(sceneVoxelizationTechnique->getVoxelizedSceneWidth());

	m_materialLightBounceStaticVoxelFiltering = static_cast<MaterialLightBounceStaticVoxelFiltering*>(materialM->buildMaterial(move(string("MaterialLightBounceStaticVoxelFiltering")), move(string("MaterialLightBounceStaticVoxelFiltering")), nullptr));
	m_vectorMaterialName.push_back("MaterialLightBounceStaticVoxelFiltering");
	m_vectorMaterial.push_back(m_materialLightBounceStaticVoxelFiltering);
	m_materialLightBounceStaticVoxelFiltering->setVoxelizationSize(sceneVoxelizationTechnique->getVoxelizedSceneWidth());

	m_dynamicVoxelVisibilityRayTracingTechnique = static_cast<DynamicVoxelVisibilityRayTracingTechnique*>(gpuPipelineM->getRasterTechniqueByName(move(string("DynamicVoxelVisibilityRayTracingTechnique"))));
	m_dynamicVoxelVisibilityRayTracingTechnique->refSignalDynamicVoxelVisibilityRayTracing().connect<LightBounceVoxelIrradianceTechnique, &LightBounceVoxelIrradianceTechnique::slotDynamicVoxelVisibilityRayTracingCompleted>(this);

	vectorString vectorTextureName =
	{
		"irradiance3DStaticNegativeX",
		"irradiance3DStaticPositiveX",
		"irradiance3DStaticNegativeY",
		"irradiance3DStaticPositiveY",
		"irradiance3DStaticNegativeZ",
		"irradiance3DStaticPositiveZ",
	};

	forI(m_vectorTexture.size())
	{
		m_vectorTexture[i] = textureM->getElement(move(vectorTextureName[i]));
	}
}

/////////////////////////////////////////////////////////////////////////////////////////////

VkCommandBuffer* LightBounceVoxelIrradianceTechnique::record(int currentImage, uint& commandBufferID, CommandBufferType& commandBufferType)
{
	commandBufferType = CommandBufferType::CBT_COMPUTE_QUEUE;

	VkCommandBuffer* commandBuffer;
	commandBuffer = addRecordedCommandBuffer(commandBufferID);
	addCommandBufferQueueType(commandBufferID, commandBufferType);

	coreM->allocCommandBuffer(&coreM->getLogicalDevice(), coreM->getComputeCommandPool(), commandBuffer);
	coreM->setObjectName(uint64_t(*commandBuffer), VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT, getName());

	coreM->beginCommandBuffer(*commandBuffer);

#ifdef USE_TIMESTAMP
	vkCmdResetQueryPool(*commandBuffer, coreM->getComputeQueueQueryPool(), m_queryIndex0, 1);
	vkCmdResetQueryPool(*commandBuffer, coreM->getComputeQueueQueryPool(), m_queryIndex1, 1);
	vkCmdWriteTimestamp(*commandBuffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, coreM->getComputeQueueQueryPool(), m_queryIndex0);
#endif

	VulkanStructInitializer::insertBufferMemoryBarrier(bufferM->getElement(move(string("cameraVisibleVoxelCompactedBuffer"))),
													   VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT,
													   VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT,
													   VK_PIPELINE_STAGE_RAY_TRACING_SHADER_BIT_KHR,
													   VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
													   commandBuffer);

	VulkanStructInitializer::insertBufferMemoryBarrier(bufferM->getElement(move(string("neighbourLitVoxelInformationBuffer"))),
													   VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT,
													   VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT,
													   VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
													   VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
													   commandBuffer);

	MaterialLightBounceVoxelIrradiance* castedBounce = static_cast<MaterialLightBounceVoxelIrradiance*>(m_vectorMaterial[0]);

	uint dynamicAllignment = materialM->getMaterialUBDynamicAllignment();

	uint32_t offsetData;
	vkCmdBindPipeline(*commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, castedBounce->getPipeline()->getPipeline());
	offsetData = static_cast<uint32_t>(castedBounce->getMaterialUniformBufferIndex() * dynamicAllignment);
	vkCmdBindDescriptorSets(*commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, castedBounce->getPipelineLayout(), 0, 1, &castedBounce->refDescriptorSet(), 1, &offsetData);
	vkCmdDispatch(*commandBuffer, castedBounce->getLocalWorkGroupsXDimension(), castedBounce->getLocalWorkGroupsYDimension(), 1); // Compute shader global workgroup https://www.khronos.org/registry/vulkan/specs/1.1-extensions/html/vkspec.html

	VulkanStructInitializer::insertBufferMemoryBarrier(bufferM->getElement(move(string("lightBounceVoxelIrradianceBuffer"))),
													   VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT,
													   VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT,
													   VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
													   VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
													   commandBuffer);

	VulkanStructInitializer::insertBufferMemoryBarrier(bufferM->getElement(move(string("cameraVisibleVoxelCompactedBuffer"))),
													   VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT,
													   VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT,
													   VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
													   VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
													   commandBuffer);

	VulkanStructInitializer::insertBufferMemoryBarrier(bufferM->getElement(move(string("lightBounceVoxelDebugBuffer"))),
													   VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT,
													   VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT,
													   VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
													   VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
													   commandBuffer);

	VkImageMemoryBarrier imageMemoryBarrier{};
	imageMemoryBarrier.sType               = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	imageMemoryBarrier.srcQueueFamilyIndex = coreM->getComputeQueueIndex();
	imageMemoryBarrier.dstQueueFamilyIndex = coreM->getComputeQueueIndex();
	imageMemoryBarrier.srcAccessMask       = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;
	imageMemoryBarrier.dstAccessMask       = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;
	imageMemoryBarrier.oldLayout           = VK_IMAGE_LAYOUT_GENERAL;
	imageMemoryBarrier.newLayout           = VK_IMAGE_LAYOUT_GENERAL;
	imageMemoryBarrier.subresourceRange    = VkImageSubresourceRange{ VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};

	forI(m_vectorTexture.size())
	{
		imageMemoryBarrier.image = textureM->getElement(move(string("staticVoxelNeighbourInfo")))->getImage();
		vkCmdPipelineBarrier(*commandBuffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 1, &imageMemoryBarrier);
	}

	// Gaussian filtering: Do a single, non separable pass with voxel tiling of size 2^3

	VulkanStructInitializer::insertBufferMemoryBarrier(bufferM->getElement(move(string("lightBounceVoxelIrradianceBuffer"))),
													   VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT,
													   VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT,
													   VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
													   VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
													   commandBuffer);
	
	m_materialLightBounceStaticVoxelFiltering->setPushConstantFilterCurrentAxis(vec4(1.0f, 0.0f, 0.0f, 0.0f)); // Value 1 for axis x pass
	m_materialLightBounceStaticVoxelFiltering->updatePushConstantCPUBuffer();
	CPUBuffer& cpuBufferNewCenter = m_materialLightBounceStaticVoxelFiltering->refShader()->refPushConstant().refCPUBuffer();

	vkCmdPushConstants(*commandBuffer,
		               m_materialLightBounceStaticVoxelFiltering->getPipelineLayout(),
					   VK_SHADER_STAGE_COMPUTE_BIT,
					   0,
					   uint32_t(m_materialLightBounceStaticVoxelFiltering->getPushConstantExposedStructFieldSize()),
					   cpuBufferNewCenter.refUBHostMemory());

	vkCmdBindPipeline(*commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, m_materialLightBounceStaticVoxelFiltering->getPipeline()->getPipeline());
	offsetData = static_cast<uint32_t>(m_materialLightBounceStaticVoxelFiltering->getMaterialUniformBufferIndex() * dynamicAllignment);
	vkCmdBindDescriptorSets(*commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, m_materialLightBounceStaticVoxelFiltering->getPipelineLayout(), 0, 1, &m_materialLightBounceStaticVoxelFiltering->refDescriptorSet(), 1, &offsetData);
	vkCmdDispatch(*commandBuffer, m_materialLightBounceStaticVoxelFiltering->getLocalWorkGroupsXDimension(), m_materialLightBounceStaticVoxelFiltering->getLocalWorkGroupsYDimension(), 1); // Compute shader global workgroup https://www.khronos.org/registry/vulkan/specs/1.1-extensions/html/vkspec.html

	VulkanStructInitializer::insertBufferMemoryBarrier(bufferM->getElement(move(string("lightBounceVoxelIrradianceTempBuffer"))),
													   VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT,
													   VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT,
													   VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
													   VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
													   commandBuffer);

	// Padding
	vkCmdBindPipeline(*commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, m_materialLightBounceStaticVoxelPadding->getPipeline()->getPipeline());
	offsetData = static_cast<uint32_t>(m_materialLightBounceStaticVoxelPadding->getMaterialUniformBufferIndex() * dynamicAllignment);
	vkCmdBindDescriptorSets(*commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, m_materialLightBounceStaticVoxelPadding->getPipelineLayout(), 0, 1, &m_materialLightBounceStaticVoxelPadding->refDescriptorSet(), 1, &offsetData);
	vkCmdDispatch(*commandBuffer, m_materialLightBounceStaticVoxelPadding->getLocalWorkGroupsXDimension(), m_materialLightBounceStaticVoxelPadding->getLocalWorkGroupsYDimension(), 1); // Compute shader global workgroup https://www.khronos.org/registry/vulkan/specs/1.1-extensions/html/vkspec.html
	

	VkMemoryBarrier memoryBarrier = { VK_STRUCTURE_TYPE_MEMORY_BARRIER, nullptr, VK_ACCESS_SHADER_WRITE_BIT | VK_ACCESS_SHADER_READ_BIT, VK_ACCESS_SHADER_WRITE_BIT | VK_ACCESS_SHADER_READ_BIT };
	vkCmdPipelineBarrier(*commandBuffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 1, &memoryBarrier, 0, nullptr, 0, nullptr);

	//VkImageMemoryBarrier imageMemoryBarrier{};
	imageMemoryBarrier.sType               = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	imageMemoryBarrier.srcQueueFamilyIndex = coreM->getComputeQueueIndex();
	imageMemoryBarrier.dstQueueFamilyIndex = coreM->getComputeQueueIndex();
	imageMemoryBarrier.srcAccessMask       = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;
	imageMemoryBarrier.dstAccessMask       = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;
	imageMemoryBarrier.oldLayout           = VK_IMAGE_LAYOUT_GENERAL;
	imageMemoryBarrier.newLayout           = VK_IMAGE_LAYOUT_GENERAL;
	imageMemoryBarrier.subresourceRange    = VkImageSubresourceRange{ VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};

	forI(m_vectorTexture.size())
	{
		imageMemoryBarrier.image = m_vectorTexture[i]->getImage();
		vkCmdPipelineBarrier(*commandBuffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 1, &imageMemoryBarrier);
	}

#ifdef USE_TIMESTAMP
	vkCmdWriteTimestamp(*commandBuffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, coreM->getComputeQueueQueryPool(), m_queryIndex1);
#endif

	coreM->endCommandBuffer(*commandBuffer);

	// NOTE: Clear command buffer if re-recorded
	m_vectorCommand.push_back(commandBuffer);

	return commandBuffer;
}

/////////////////////////////////////////////////////////////////////////////////////////////

void LightBounceVoxelIrradianceTechnique::postCommandSubmit()
{
	m_signalLightBounceVoxelIrradianceCompletion.emit();

	m_executeCommand = false;
	m_active         = false;
	m_executeCommand = false;
	m_needsToRecord  = (m_vectorCommand.size() != m_usedCommandBufferNumber);
}

/////////////////////////////////////////////////////////////////////////////////////////////

void LightBounceVoxelIrradianceTechnique::slotPrefixSumComplete()
{
	m_numOccupiedVoxel = m_techniquePrefixSum->getFirstIndexOccupiedElement();
	m_bufferNumElement = m_numOccupiedVoxel * m_numThreadPerLocalWorkgroup * 6; // Each local workgroup will work one side of each voxel in the scene
	m_sceneMin.w       = float(m_numOccupiedVoxel);

	obtainDispatchWorkGroupCount();

	MaterialLightBounceVoxelIrradiance* materialCasted = static_cast<MaterialLightBounceVoxelIrradiance*>(m_material);
	materialCasted->setLocalWorkGroupsXDimension(m_localWorkGroupsXDimension);
	materialCasted->setLocalWorkGroupsYDimension(m_localWorkGroupsYDimension);
	materialCasted->setSceneMinAndNumberVoxel(m_sceneMin);
	materialCasted->setNumThreadExecuted(m_bufferNumElement);

	m_bufferNumElement = m_cameraVisibleVoxelNumber * 6;

	bufferM->resize(m_lightBounceVoxelDebugBuffer,       nullptr,       3000000 * sizeof(uint));

	m_prefixSumCompleted = true;
}

/////////////////////////////////////////////////////////////////////////////////////////////

void LightBounceVoxelIrradianceTechnique::slotDynamicVoxelVisibilityRayTracingCompleted()
{
	if (m_prefixSumCompleted)
	{
		m_cameraVisibleVoxelNumber = m_processCameraVisibleResultsTechnique->getStaticCameraVisibleVoxelNumber();
		m_bufferNumElement         = m_cameraVisibleVoxelNumber * m_numThreadPerLocalWorkgroup * 6; // Each local workgroup will work one side of each voxel in the scene
		m_sceneMin.w               = float(m_cameraVisibleVoxelNumber);

		obtainDispatchWorkGroupCount();

		MaterialLightBounceVoxelIrradiance* materialCasted = static_cast<MaterialLightBounceVoxelIrradiance*>(m_material);
		vec3 cameraPosition                                = m_litVoxelTechnique->getCameraPosition();
		vec3 cameraForward                                 = m_litVoxelTechnique->getCameraForward();
		float emitterRadiance                              = m_litVoxelTechnique->getEmitterRadiance();

		vec4 lightPosition = materialCasted->getLightPosition();
		lightPosition.x    = cameraPosition.x;
		lightPosition.y    = cameraPosition.y;
		lightPosition.z    = cameraPosition.z;
		materialCasted->setLightPosition(lightPosition);
		materialCasted->setLightForwardEmitterRadiance(vec4(cameraForward.x, cameraForward.y, cameraForward.z, emitterRadiance));
		materialCasted->setLocalWorkGroupsXDimension(m_localWorkGroupsXDimension);
		materialCasted->setLocalWorkGroupsYDimension(m_localWorkGroupsYDimension);
		materialCasted->setSceneMinAndNumberVoxel(m_sceneMin);
		materialCasted->setNumThreadExecuted(m_bufferNumElement);

		m_bufferNumElement = m_cameraVisibleVoxelNumber * 6;

		uint numTilesToProcess = m_processCameraVisibleResultsTechnique->getIrradianceFilteringTagTilesNumber();
		m_bufferNumElement = numTilesToProcess * 6;
		m_materialLightBounceStaticVoxelFiltering->setLocalWorkGroupsXDimension(numTilesToProcess);
		m_materialLightBounceStaticVoxelFiltering->setLocalWorkGroupsYDimension(6);
		m_materialLightBounceStaticVoxelFiltering->setNumThreadExecuted(numTilesToProcess * 6 * m_materialLightBounceStaticVoxelFiltering->getNumThreadPerLocalWorkgroup());
		
		numTilesToProcess = m_processCameraVisibleResultsTechnique->getIrradiancePaddingTagTilesNumber();
		m_bufferNumElement = numTilesToProcess * 6;
		// TODO: Re-do computations in BufferProcessTechnique::obtainDispatchWorkGroupCount to allow workgroyup dispatch size computations for local workgroups which
		// only process a single element
		m_materialLightBounceStaticVoxelPadding->setLocalWorkGroupsXDimension(numTilesToProcess);
		m_materialLightBounceStaticVoxelPadding->setLocalWorkGroupsYDimension(6);
		m_materialLightBounceStaticVoxelPadding->setNumThreadExecuted(numTilesToProcess * 6 * m_materialLightBounceStaticVoxelPadding->getNumThreadPerLocalWorkgroup());

		// Each time DynamicVoxelVisibilityRayTracingTechnique this technique needs to record
		m_vectorCommand.clear();

		m_active = true;
	}
}

/////////////////////////////////////////////////////////////////////////////////////////////

void LightBounceVoxelIrradianceTechnique::slot3KeyPressed()
{

}

/////////////////////////////////////////////////////////////////////////////////////////////

void LightBounceVoxelIrradianceTechnique::slot4KeyPressed()
{

}

/////////////////////////////////////////////////////////////////////////////////////////////

void LightBounceVoxelIrradianceTechnique::buildRayDirectionBuffer()
{
	// TODO: Try a 16-bit floating point buffer
	vector<vec4> vectorData =
	{
		vec4(-0.993532, -0.0605193, -0.0960854, 0.0),
		vec4(-0.989063, 0.021587, 0.145907, 0.0),
		vec4(-0.987918, 0.154939, -0.00355872, 0.0),
		vec4(-0.97973, -0.193082, 0.0533808, 0.0),
		vec4(-0.966808, 0.0706117, -0.245552, 0.0),
		vec4(-0.949404, -0.106705, 0.295374, 0.0),
		vec4(-0.947152, 0.281932, -0.153025, 0.0),
		vec4(-0.944551, -0.268793, -0.188612, 0.0),
		vec4(-0.943731, 0.229174, 0.238434, 0.0),
		vec4(-0.931445, -0.134586, -0.338078, 0.0),
		vec4(-0.927918, 0.362012, 0.088968, 0.0),
		vec4(-0.926904, -0.315756, 0.202847, 0.0),
		vec4(-0.916675, 0.0961267, 0.3879, 0.0),
		vec4(-0.916024, -0.399209, -0.0391459, 0.0),
		vec4(-0.899127, 0.188497, -0.395018, 0.0),
		vec4(-0.873225, 0.483547, -0.0604982, 0.0),
		vec4(-0.873057, -0.00851604, -0.487544, 0.0),
		vec4(-0.869121, 0.391316, -0.302491, 0.0),
		vec4(-0.86872, -0.217815, 0.44484, 0.0),
		vec4(-0.850046, -0.515026, 0.11032, 0.0),
		vec4(-0.847791, 0.414384, 0.330961, 0.0),
		vec4(-0.845093, -0.45473, -0.281139, 0.0),
		vec4(-0.844715, -0.31786, -0.430605, 0.0),
		vec4(-0.842988, -0.0246745, 0.537367, 0.0),
		vec4(-0.837737, -0.41722, 0.352313, 0.0),
		vec4(-0.832267, 0.276625, 0.480427, 0.0),
		vec4(-0.818027, 0.545794, 0.181495, 0.0),
		vec4(-0.802512, -0.581925, -0.131673, 0.0),
		vec4(-0.793823, -0.182655, -0.580071, 0.0),
		vec4(-0.790645, 0.280032, -0.544484, 0.0),
		vec4(-0.784531, 0.583461, -0.209964, 0.0),
		vec4(-0.764937, 0.0953341, -0.637011, 0.0),
		vec4(-0.763121, 0.144504, 0.629893, 0.0),
		vec4(-0.757297, 0.471419, -0.451957, 0.0),
		vec4(-0.7518, -0.60606, 0.259786, 0.0),
		vec4(-0.75002, 0.660639, 0.0320285, 0.0),
		vec4(-0.747044, -0.297869, 0.594306, 0.0),
		vec4(-0.723937, -0.689636, 0.0177936, 0.0),
		vec4(-0.717181, -0.117952, 0.686833, 0.0),
		vec4(-0.715949, -0.485422, 0.501779, 0.0),
		vec4(-0.714596, -0.46442, -0.523132, 0.0),
		vec4(-0.709482, 0.563289, 0.423488, 0.0),
		vec4(-0.704106, 0.419475, 0.572954, 0.0),
		vec4(-0.703397, -0.604655, -0.373665, 0.0),
		vec4(-0.681022, -0.0631161, -0.729537, 0.0),
		vec4(-0.668713, -0.316914, -0.672598, 0.0),
		vec4(-0.667848, 0.651758, -0.359431, 0.0),
		vec4(-0.666733, 0.693094, 0.274021, 0.0),
		vec4(-0.651028, 0.749914, -0.117438, 0.0),
		vec4(-0.647711, -0.728152, -0.224199, 0.0),
		vec4(-0.640594, 0.328744, -0.69395, 0.0),
		vec4(-0.635912, 0.271523, 0.72242, 0.0),
		vec4(-0.627643, -0.662251, 0.409253, 0.0),
		vec4(-0.625684, 0.0334424, 0.779359, 0.0),
		vec4(-0.616798, -0.769145, 0.16726, 0.0),
		vec4(-0.615894, 0.508885, -0.601424, 0.0),
		vec4(-0.59722, 0.157426, -0.786477, 0.0),
		vec4(-0.586375, 0.800406, 0.124555, 0.0),
		vec4(-0.582339, -0.328153, 0.743772, 0.0),
		vec4(-0.56571, -0.505817, 0.651246, 0.0),
		vec4(-0.557499, -0.826807, -0.0747331, 0.0),
		vec4(-0.553118, -0.561271, -0.615658, 0.0),
		vec4(-0.544259, 0.510802, 0.66548, 0.0),
		vec4(-0.542459, -0.173059, -0.822064, 0.0),
		vec4(-0.540842, 0.664243, 0.516014, 0.0),
		vec4(-0.531402, -0.707302, -0.466192, 0.0),
		vec4(-0.530399, 0.678012, -0.508897, 0.0),
		vec4(-0.529099, 0.805492, -0.266904, 0.0),
		vec4(-0.52431, -0.160323, 0.836299, 0.0),
		vec4(-0.509069, -0.39425, -0.765125, 0.0),
		vec4(-0.489431, -0.812491, 0.316726, 0.0),
		vec4(-0.485998, 0.793378, 0.366548, 0.0),
		vec4(-0.48514, -0.67266, 0.558719, 0.0),
		vec4(-0.477982, 0.878016, -0.024911, 0.0),
		vec4(-0.476408, 0.0197013, -0.879004, 0.0),
		vec4(-0.471993, 0.130528, 0.871886, 0.0),
		vec4(-0.471688, 0.336709, 0.814947, 0.0),
		vec4(-0.463536, -0.827538, -0.316726, 0.0),
		vec4(-0.448482, 0.484797, -0.75089, 0.0),
		vec4(-0.442268, -0.893764, 0.0747331, 0.0),
		vec4(-0.441014, 0.306847, -0.843416, 0.0),
		vec4(-0.394348, 0.892953, 0.217082, 0.0),
		vec4(-0.393372, 0.819692, -0.41637, 0.0),
		vec4(-0.38934, -0.455275, 0.800712, 0.0),
		vec4(-0.380581, 0.649397, -0.658363, 0.0),
		vec4(-0.376035, -0.597555, -0.708185, 0.0),
		vec4(-0.369322, -0.0297376, 0.928826, 0.0),
		vec4(-0.368659, 0.538067, 0.758007, 0.0),
		vec4(-0.362808, -0.91673, -0.16726, 0.0),
		vec4(-0.358747, -0.270971, 0.893238, 0.0),
		vec4(-0.357215, 0.708573, 0.608541, 0.0),
		vec4(-0.351365, -0.811916, 0.466192, 0.0),
		vec4(-0.351279, 0.919889, -0.174377, 0.0),
		vec4(-0.349875, -0.202759, -0.914591, 0.0),
		vec4(-0.344176, -0.75457, -0.558719, 0.0),
		vec4(-0.332951, -0.62259, 0.708185, 0.0),
		vec4(-0.329473, -0.394819, -0.857651, 0.0),
		vec4(-0.32551, 0.134366, -0.935943, 0.0),
		vec4(-0.311886, -0.923288, 0.224199, 0.0),
		vec4(-0.290571, 0.839535, 0.459075, 0.0),
		vec4(-0.281545, 0.311809, 0.907473, 0.0),
		vec4(-0.277755, 0.958269, 0.0676157, 0.0),
		vec4(-0.264569, -0.873221, -0.409253, 0.0),
		vec4(-0.254427, 0.784281, -0.565836, 0.0),
		vec4(-0.25027, 0.355984, -0.900356, 0.0),
		vec4(-0.240679, -0.970442, -0.0177936, 0.0),
		vec4(-0.23376, 0.123549, 0.964413, 0.0),
		vec4(-0.233294, -0.0412664, -0.97153, 0.0),
		vec4(-0.22755, 0.543721, -0.807829, 0.0),
		vec4(-0.216646, 0.920972, -0.323843, 0.0),
		vec4(-0.21374, -0.758472, 0.615658, 0.0),
		vec4(-0.20275, -0.563696, -0.800712, 0.0),
		vec4(-0.197167, 0.487563, 0.850534, 0.0),
		vec4(-0.188404, 0.932012, 0.309609, 0.0),
		vec4(-0.17987, -0.481748, 0.857651, 0.0),
		vec4(-0.177018, -0.910516, 0.373665, 0.0),
		vec4(-0.176798, 0.69083, 0.701068, 0.0),
		vec4(-0.169189, -0.261795, 0.950178, 0.0),
		vec4(-0.159327, -0.741953, -0.651246, 0.0),
		vec4(-0.154271, -0.953264, -0.259786, 0.0),
		vec4(-0.152788, -0.0701635, 0.985765, 0.0),
		vec4(-0.147336, 0.985694, -0.0818505, 0.0),
		vec4(-0.143668, -0.276625, -0.950178, 0.0),
		vec4(-0.125075, 0.687531, -0.715302, 0.0),
		vec4(-0.108192, -0.985371, 0.131673, 0.0),
		vec4(-0.0972019, 0.828425, 0.551601, 0.0),
		vec4(-0.090446, -0.637498, 0.765125, 0.0),
		vec4(-0.0878188, 0.0804492, -0.992883, 0.0),
		vec4(0.999499, 0.0195007, 0.024911, 0.0),
		vec4(0.985746, -0.113101, -0.124555, 0.0),
		vec4(0.973892, -0.194274, 0.117438, 0.0),
		vec4(0.973138, 0.150315, 0.174377, 0.0),
		vec4(0.971063, 0.0995586, -0.217082, 0.0),
		vec4(0.970076, 0.233196, -0.0676157, 0.0),
		vec4(0.961802, -0.060821, 0.266904, 0.0),
		vec4(0.945349, -0.324483, -0.0320285, 0.0),
		vec4(0.932304, -0.236053, -0.274021, 0.0),
		vec4(0.929935, -0.0293736, -0.366548, 0.0),
		vec4(0.929322, 0.360084, 0.0818505, 0.0),
		vec4(0.907413, 0.267819, 0.323843, 0.0),
		vec4(0.906931, 0.0641287, 0.41637, 0.0),
		vec4(0.90302, 0.297822, -0.309609, 0.0),
		vec4(0.896944, -0.25749, 0.359431, 0.0),
		vec4(0.895386, -0.392683, 0.209964, 0.0),
		vec4(0.887828, 0.431411, -0.160142, 0.0),
		vec4(0.879247, -0.440436, -0.181495, 0.0),
		vec4(0.873266, 0.163268, -0.459075, 0.0),
		vec4(0.8525, -0.519214, 0.0604982, 0.0),
		vec4(0.852189, 0.469325, 0.231317, 0.0),
		vec4(0.851693, -0.125075, 0.508897, 0.0),
		vec4(0.844901, -0.140966, -0.516014, 0.0),
		vec4(0.840698, -0.337469, -0.423488, 0.0),
		vec4(0.833077, 0.553053, -0.0106762, 0.0),
		vec4(0.807364, 0.167309, 0.565836, 0.0),
		vec4(0.80407, 0.359791, 0.47331, 0.0),
		vec4(0.792487, 0.0405188, -0.608541, 0.0),
		vec4(0.787655, 0.466783, -0.402135, 0.0),
		vec4(0.784559, -0.424502, 0.451957, 0.0),
		vec4(0.779818, -0.531365, -0.330961, 0.0),
		vec4(0.773993, -0.626912, -0.088968, 0.0),
		vec4(0.770338, -0.561319, 0.302491, 0.0),
		vec4(0.767863, 0.325765, -0.551601, 0.0),
		vec4(0.759155, 0.599868, -0.252669, 0.0),
		vec4(0.752646, -0.00908358, 0.658363, 0.0),
		vec4(0.74709, -0.2831, 0.601424, 0.0),
		vec4(0.744655, 0.652859, 0.13879, 0.0),
		vec4(0.743242, 0.550087, 0.380783, 0.0),
		vec4(0.713945, -0.683276, 0.153025, 0.0),
		vec4(0.713284, -0.219914, -0.66548, 0.0),
		vec4(0.713091, -0.404011, -0.572954, 0.0),
		vec4(0.691224, 0.715234, -0.103203, 0.0),
		vec4(0.687735, 0.188479, -0.701068, 0.0),
		vec4(0.666415, -0.706428, -0.238434, 0.0),
		vec4(0.665174, 0.411939, 0.622776, 0.0),
		vec4(0.659257, 0.231781, 0.715302, 0.0),
		vec4(0.652446, -0.586092, -0.480427, 0.0),
		vec4(0.650019, -0.0538536, -0.758007, 0.0),
		vec4(0.643265, -0.149581, 0.75089, 0.0),
		vec4(0.63508, 0.593282, -0.494662, 0.0),
		vec4(0.634796, -0.548244, 0.544484, 0.0),
		vec4(0.62941, 0.721631, 0.288256, 0.0),
		vec4(0.6238, -0.781576, 0.00355872, 0.0),
		vec4(0.623598, 0.442973, -0.644128, 0.0),
		vec4(0.608988, -0.687819, 0.395018, 0.0),
		vec4(0.608148, 0.590755, 0.530249, 0.0),
		vec4(0.602815, -0.39376, 0.69395, 0.0),
		vec4(0.594269, 0.726419, -0.345196, 0.0),
		vec4(0.592208, 0.804456, 0.0462633, 0.0),
		vec4(0.585059, 0.0715412, 0.807829, 0.0),
		vec4(0.550784, -0.418027, -0.72242, 0.0),
		vec4(0.540084, -0.804993, 0.245552, 0.0),
		vec4(0.538591, 0.283068, -0.793594, 0.0),
		vec4(0.537426, -0.748803, -0.3879, 0.0),
		vec4(0.526097, -0.243071, -0.814947, 0.0),
		vec4(0.521224, 0.0701295, -0.850534, 0.0),
		vec4(0.514185, 0.835047, -0.19573, 0.0),
		vec4(0.508511, -0.848603, -0.145907, 0.0),
		vec4(0.50342, -0.591441, -0.629893, 0.0),
		vec4(0.495363, 0.750343, 0.437722, 0.0),
		vec4(0.490709, 0.403543, 0.772242, 0.0),
		vec4(0.484965, -0.231209, 0.843416, 0.0),
		vec4(0.470875, 0.860213, 0.19573, 0.0),
		vec4(0.461601, -0.617367, 0.637011, 0.0),
		vec4(0.459057, 0.66669, -0.587189, 0.0),
		vec4(0.454376, 0.500882, -0.736655, 0.0),
		vec4(0.453268, 0.576659, 0.679715, 0.0),
		vec4(0.447303, 0.228243, 0.864769, 0.0),
		vec4(0.43939, -0.893143, 0.0960854, 0.0),
		vec4(0.433203, -0.04116, 0.900356, 0.0),
		vec4(0.432497, -0.440908, 0.786477, 0.0),
		vec4(0.424915, -0.762724, 0.487544, 0.0),
		vec4(0.410311, -0.0902059, -0.907473, 0.0),
		vec4(0.406604, 0.801918, -0.437722, 0.0),
		vec4(0.405662, 0.912851, -0.0462633, 0.0),
		vec4(0.395987, -0.744601, -0.537367, 0.0),
		vec4(0.377414, -0.877675, -0.295374, 0.0),
		vec4(0.35438, 0.298671, -0.886121, 0.0),
		vec4(0.351905, 0.728953, 0.587189, 0.0),
		vec4(0.34712, -0.345431, -0.871886, 0.0),
		vec4(0.344298, -0.875878, 0.338078, 0.0),
		vec4(0.338876, -0.527031, -0.779359, 0.0),
		vec4(0.336844, 0.876, 0.345196, 0.0),
		vec4(0.317387, -0.946793, -0.0533808, 0.0),
		vec4(0.315283, 0.90416, -0.288256, 0.0),
		vec4(0.312438, 0.114102, -0.943061, 0.0),
		vec4(0.283747, 0.481607, 0.829181, 0.0),
		vec4(0.282389, -0.622922, 0.729537, 0.0),
		vec4(0.279732, 0.0730449, 0.957295, 0.0),
		vec4(0.279116, 0.954695, 0.103203, 0.0),
		vec4(0.27792, -0.216267, 0.935943, 0.0),
		vec4(0.277914, 0.484997, -0.829181, 0.0),
		vec4(0.276553, 0.679342, -0.679715, 0.0),
		vec4(0.264062, 0.284122, 0.921708, 0.0),
		vec4(0.25302, -0.404145, 0.879004, 0.0),
		vec4(0.252652, -0.681489, -0.686833, 0.0),
		vec4(0.240929, -0.862595, -0.44484, 0.0),
		vec4(0.234389, -0.780115, 0.580071, 0.0),
		vec4(0.23419, -0.953719, 0.188612, 0.0),
		vec4(0.22309, -0.141912, -0.964413, 0.0),
		vec4(0.212107, 0.820882, -0.530249, 0.0),
		vec4(0.210169, 0.642782, 0.736655, 0.0),
		vec4(0.200966, 0.845531, 0.494662, 0.0),
		vec4(0.198468, 0.970231, -0.13879, 0.0),
		vec4(0.18465, -0.961643, -0.202847, 0.0),
		vec4(0.157042, -0.33559, -0.928826, 0.0),
		vec4(0.145249, 0.956588, 0.252669, 0.0),
		vec4(0.142124, -0.89128, 0.430605, 0.0),
		vec4(0.125062, 0.163121, -0.978648, 0.0),
		vec4(0.120335, -0.534905, -0.836299, 0.0),
		vec4(0.118258, -0.556979, 0.822064, 0.0),
		vec4(0.116088, 0.370105, -0.921708, 0.0),
		vec4(0.113379, -0.0364603, 0.992883, 0.0),
		vec4(0.111128, -0.796524, -0.594306, 0.0),
		vec4(0.10988, 0.918113, -0.380783, 0.0),
		vec4(0.10761, 0.626149, -0.772242, 0.0),
		vec4(0.106759, -0.993514, 0.0391459, 0.0),
		vec4(-0.0265331, -0.993542, -0.11032, 0.0),
		vec4(0.106759, -0.993514, 0.0391459, 0.0),
		vec4(-0.108192, -0.985371, 0.131673, 0.0),
		vec4(-0.240679, -0.970442, -0.0177936, 0.0),
		vec4(0.18465, -0.961643, -0.202847, 0.0),
		vec4(0.0233991, -0.959382, 0.281139, 0.0),
		vec4(0.23419, -0.953719, 0.188612, 0.0),
		vec4(-0.154271, -0.953264, -0.259786, 0.0),
		vec4(0.317387, -0.946793, -0.0533808, 0.0),
		vec4(0.0523457, -0.934417, -0.352313, 0.0),
		vec4(-0.311886, -0.923288, 0.224199, 0.0),
		vec4(-0.362808, -0.91673, -0.16726, 0.0),
		vec4(-0.177018, -0.910516, 0.373665, 0.0),
		vec4(-0.442268, -0.893764, 0.0747331, 0.0),
		vec4(0.43939, -0.893143, 0.0960854, 0.0),
		vec4(0.142124, -0.89128, 0.430605, 0.0),
		vec4(0.377414, -0.877675, -0.295374, 0.0),
		vec4(0.344298, -0.875878, 0.338078, 0.0),
		vec4(-0.264569, -0.873221, -0.409253, 0.0),
		vec4(0.240929, -0.862595, -0.44484, 0.0),
		vec4(-0.0672145, -0.86238, -0.501779, 0.0),
		vec4(-0.0496373, -0.850805, 0.523132, 0.0),
		vec4(0.508511, -0.848603, -0.145907, 0.0),
		vec4(-0.463536, -0.827538, -0.316726, 0.0),
		vec4(-0.557499, -0.826807, -0.0747331, 0.0),
		vec4(-0.489431, -0.812491, 0.316726, 0.0),
		vec4(-0.351365, -0.811916, 0.466192, 0.0),
		vec4(0.540084, -0.804993, 0.245552, 0.0),
		vec4(0.111128, -0.796524, -0.594306, 0.0),
		vec4(0.6238, -0.781576, 0.00355872, 0.0),
		vec4(0.234389, -0.780115, 0.580071, 0.0),
		vec4(-0.616798, -0.769145, 0.16726, 0.0),
		vec4(0.424915, -0.762724, 0.487544, 0.0),
		vec4(-0.21374, -0.758472, 0.615658, 0.0),
		vec4(-0.344176, -0.75457, -0.558719, 0.0),
		vec4(0.537426, -0.748803, -0.3879, 0.0),
		vec4(0.395987, -0.744601, -0.537367, 0.0),
		vec4(-0.159327, -0.741953, -0.651246, 0.0),
		vec4(0.0557948, -0.737902, 0.672598, 0.0),
		vec4(-0.647711, -0.728152, -0.224199, 0.0),
		vec4(-0.531402, -0.707302, -0.466192, 0.0),
		vec4(0.666415, -0.706428, -0.238434, 0.0),
		vec4(-0.723937, -0.689636, 0.0177936, 0.0),
		vec4(0.608988, -0.687819, 0.395018, 0.0),
		vec4(0.713945, -0.683276, 0.153025, 0.0),
		vec4(0.252652, -0.681489, -0.686833, 0.0),
		vec4(-0.48514, -0.67266, 0.558719, 0.0),
		vec4(0.00325993, -0.668425, -0.743772, 0.0),
		vec4(-0.627643, -0.662251, 0.409253, 0.0),
		vec4(-0.090446, -0.637498, 0.765125, 0.0),
		vec4(0.773993, -0.626912, -0.088968, 0.0),
		vec4(0.282389, -0.622922, 0.729537, 0.0),
		vec4(-0.332951, -0.62259, 0.708185, 0.0),
		vec4(0.461601, -0.617367, 0.637011, 0.0),
		vec4(-0.7518, -0.60606, 0.259786, 0.0),
		vec4(-0.703397, -0.604655, -0.373665, 0.0),
		vec4(-0.376035, -0.597555, -0.708185, 0.0),
		vec4(0.50342, -0.591441, -0.629893, 0.0),
		vec4(0.652446, -0.586092, -0.480427, 0.0),
		vec4(-0.802512, -0.581925, -0.131673, 0.0),
		vec4(-0.20275, -0.563696, -0.800712, 0.0),
		vec4(0.770338, -0.561319, 0.302491, 0.0),
		vec4(-0.553118, -0.561271, -0.615658, 0.0),
		vec4(0.118258, -0.556979, 0.822064, 0.0),
		vec4(0.634796, -0.548244, 0.544484, 0.0),
		vec4(0.120335, -0.534905, -0.836299, 0.0),
		vec4(0.779818, -0.531365, -0.330961, 0.0),
		vec4(0.338876, -0.527031, -0.779359, 0.0),
		vec4(0.8525, -0.519214, 0.0604982, 0.0),
		vec4(-0.850046, -0.515026, 0.11032, 0.0),
		vec4(-0.56571, -0.505817, 0.651246, 0.0),
		vec4(-0.715949, -0.485422, 0.501779, 0.0),
		vec4(-0.17987, -0.481748, 0.857651, 0.0),
		vec4(-0.714596, -0.46442, -0.523132, 0.0),
		vec4(-0.38934, -0.455275, 0.800712, 0.0),
		vec4(-0.845093, -0.45473, -0.281139, 0.0),
		vec4(-0.0577765, -0.445855, -0.893238, 0.0),
		vec4(0.432497, -0.440908, 0.786477, 0.0),
		vec4(0.879247, -0.440436, -0.181495, 0.0),
		vec4(0.784559, -0.424502, 0.451957, 0.0),
		vec4(0.550784, -0.418027, -0.72242, 0.0),
		vec4(-0.837737, -0.41722, 0.352313, 0.0),
		vec4(-0.002908, -0.40437, 0.914591, 0.0),
		vec4(0.25302, -0.404145, 0.879004, 0.0),
		vec4(0.713091, -0.404011, -0.572954, 0.0),
		vec4(-0.916024, -0.399209, -0.0391459, 0.0),
		vec4(-0.329473, -0.394819, -0.857651, 0.0),
		vec4(-0.509069, -0.39425, -0.765125, 0.0),
		vec4(0.602815, -0.39376, 0.69395, 0.0),
		vec4(0.895386, -0.392683, 0.209964, 0.0),
		vec4(0.34712, -0.345431, -0.871886, 0.0),
		vec4(0.840698, -0.337469, -0.423488, 0.0),
		vec4(0.157042, -0.33559, -0.928826, 0.0),
		vec4(-0.582339, -0.328153, 0.743772, 0.0),
		vec4(0.945349, -0.324483, -0.0320285, 0.0),
		vec4(-0.844715, -0.31786, -0.430605, 0.0),
		vec4(-0.668713, -0.316914, -0.672598, 0.0),
		vec4(-0.926904, -0.315756, 0.202847, 0.0),
		vec4(-0.747044, -0.297869, 0.594306, 0.0),
		vec4(0.74709, -0.2831, 0.601424, 0.0),
		vec4(-0.143668, -0.276625, -0.950178, 0.0),
		vec4(-0.358747, -0.270971, 0.893238, 0.0),
		vec4(-0.944551, -0.268793, -0.188612, 0.0),
		vec4(-0.169189, -0.261795, 0.950178, 0.0),
		vec4(0.896944, -0.25749, 0.359431, 0.0),
		vec4(0.526097, -0.243071, -0.814947, 0.0),
		vec4(0.932304, -0.236053, -0.274021, 0.0),
		vec4(0.484965, -0.231209, 0.843416, 0.0),
		vec4(0.0796687, -0.223119, 0.97153, 0.0),
		vec4(0.713284, -0.219914, -0.66548, 0.0),
		vec4(-0.86872, -0.217815, 0.44484, 0.0),
		vec4(0.27792, -0.216267, 0.935943, 0.0),
		vec4(-0.349875, -0.202759, -0.914591, 0.0),
		vec4(0.973892, -0.194274, 0.117438, 0.0),
		vec4(-0.97973, -0.193082, 0.0533808, 0.0),
		vec4(-0.793823, -0.182655, -0.580071, 0.0),
		vec4(-0.542459, -0.173059, -0.822064, 0.0),
		vec4(0.0146987, -0.167485, -0.985765, 0.0),
		vec4(-0.52431, -0.160323, 0.836299, 0.0),
		vec4(0.643265, -0.149581, 0.75089, 0.0),
		vec4(0.22309, -0.141912, -0.964413, 0.0),
		vec4(0.844901, -0.140966, -0.516014, 0.0),
		vec4(-0.931445, -0.134586, -0.338078, 0.0),
		vec4(0.851693, -0.125075, 0.508897, 0.0),
		vec4(-0.717181, -0.117952, 0.686833, 0.0),
		vec4(0.985746, -0.113101, -0.124555, 0.0),
		vec4(-0.949404, -0.106705, 0.295374, 0.0),
		vec4(0.410311, -0.0902059, -0.907473, 0.0),
		vec4(0.0679731, 0.99763, 0.0106762, 0.0),
		vec4(-0.147336, 0.985694, -0.0818505, 0.0),
		vec4(-0.0648203, 0.984963, 0.160142, 0.0),
		vec4(-0.0142336, 0.972774, -0.231317, 0.0),
		vec4(0.198468, 0.970231, -0.13879, 0.0),
		vec4(-0.277755, 0.958269, 0.0676157, 0.0),
		vec4(0.145249, 0.956588, 0.252669, 0.0),
		vec4(0.279116, 0.954695, 0.103203, 0.0),
		vec4(-0.188404, 0.932012, 0.309609, 0.0),
		vec4(-0.216646, 0.920972, -0.323843, 0.0),
		vec4(-0.351279, 0.919889, -0.174377, 0.0),
		vec4(0.10988, 0.918113, -0.380783, 0.0),
		vec4(0.015514, 0.915449, 0.402135, 0.0),
		vec4(0.405662, 0.912851, -0.0462633, 0.0),
		vec4(0.315283, 0.90416, -0.288256, 0.0),
		vec4(-0.394348, 0.892953, 0.217082, 0.0),
		vec4(-0.477982, 0.878016, -0.024911, 0.0),
		vec4(-0.0855685, 0.87673, -0.47331, 0.0),
		vec4(0.336844, 0.876, 0.345196, 0.0),
		vec4(0.470875, 0.860213, 0.19573, 0.0),
		vec4(0.200966, 0.845531, 0.494662, 0.0),
		vec4(-0.290571, 0.839535, 0.459075, 0.0),
		vec4(0.514185, 0.835047, -0.19573, 0.0),
		vec4(-0.0972019, 0.828425, 0.551601, 0.0),
		vec4(0.212107, 0.820882, -0.530249, 0.0),
		vec4(-0.393372, 0.819692, -0.41637, 0.0),
		vec4(-0.529099, 0.805492, -0.266904, 0.0),
		vec4(0.592208, 0.804456, 0.0462633, 0.0),
		vec4(0.406604, 0.801918, -0.437722, 0.0),
		vec4(-0.586375, 0.800406, 0.124555, 0.0),
		vec4(-0.485998, 0.793378, 0.366548, 0.0),
		vec4(-0.254427, 0.784281, -0.565836, 0.0),
		vec4(0.0285147, 0.781881, -0.622776, 0.0),
		vec4(0.0760642, 0.761126, 0.644128, 0.0),
		vec4(0.495363, 0.750343, 0.437722, 0.0),
		vec4(-0.651028, 0.749914, -0.117438, 0.0),
		vec4(0.351905, 0.728953, 0.587189, 0.0),
		vec4(0.594269, 0.726419, -0.345196, 0.0),
		vec4(0.62941, 0.721631, 0.288256, 0.0),
		vec4(0.691224, 0.715234, -0.103203, 0.0),
		vec4(-0.357215, 0.708573, 0.608541, 0.0),
		vec4(-0.666733, 0.693094, 0.274021, 0.0),
		vec4(-0.176798, 0.69083, 0.701068, 0.0),
		vec4(-0.125075, 0.687531, -0.715302, 0.0),
		vec4(0.276553, 0.679342, -0.679715, 0.0),
		vec4(-0.530399, 0.678012, -0.508897, 0.0),
		vec4(0.459057, 0.66669, -0.587189, 0.0),
		vec4(-0.540842, 0.664243, 0.516014, 0.0),
		vec4(-0.75002, 0.660639, 0.0320285, 0.0),
		vec4(0.744655, 0.652859, 0.13879, 0.0),
		vec4(-0.667848, 0.651758, -0.359431, 0.0),
		vec4(-0.380581, 0.649397, -0.658363, 0.0),
		vec4(0.210169, 0.642782, 0.736655, 0.0),
		vec4(0.10761, 0.626149, -0.772242, 0.0),
		vec4(-0.0207677, 0.608093, 0.793594, 0.0),
		vec4(0.759155, 0.599868, -0.252669, 0.0),
		vec4(0.63508, 0.593282, -0.494662, 0.0),
		vec4(0.608148, 0.590755, 0.530249, 0.0),
		vec4(-0.784531, 0.583461, -0.209964, 0.0),
		vec4(0.453268, 0.576659, 0.679715, 0.0),
		vec4(-0.709482, 0.563289, 0.423488, 0.0),
		vec4(0.833077, 0.553053, -0.0106762, 0.0),
		vec4(0.743242, 0.550087, 0.380783, 0.0),
		vec4(-0.818027, 0.545794, 0.181495, 0.0),
		vec4(-0.22755, 0.543721, -0.807829, 0.0),
		vec4(-0.368659, 0.538067, 0.758007, 0.0),
		vec4(-0.544259, 0.510802, 0.66548, 0.0),
		vec4(-0.615894, 0.508885, -0.601424, 0.0),
		vec4(-0.0231956, 0.501634, -0.864769, 0.0),
		vec4(0.454376, 0.500882, -0.736655, 0.0),
		vec4(-0.197167, 0.487563, 0.850534, 0.0),
		vec4(0.277914, 0.484997, -0.829181, 0.0),
		vec4(-0.448482, 0.484797, -0.75089, 0.0),
		vec4(-0.873225, 0.483547, -0.0604982, 0.0),
		vec4(0.283747, 0.481607, 0.829181, 0.0),
		vec4(-0.757297, 0.471419, -0.451957, 0.0),
		vec4(0.852189, 0.469325, 0.231317, 0.0),
		vec4(0.787655, 0.466783, -0.402135, 0.0),
		vec4(0.084005, 0.455777, 0.886121, 0.0),
		vec4(0.623598, 0.442973, -0.644128, 0.0),
		vec4(0.887828, 0.431411, -0.160142, 0.0),
		vec4(-0.704106, 0.419475, 0.572954, 0.0),
		vec4(-0.847791, 0.414384, 0.330961, 0.0),
		vec4(0.665174, 0.411939, 0.622776, 0.0),
		vec4(0.490709, 0.403543, 0.772242, 0.0),
		vec4(-0.869121, 0.391316, -0.302491, 0.0),
		vec4(0.116088, 0.370105, -0.921708, 0.0),
		vec4(-0.927918, 0.362012, 0.088968, 0.0),
		vec4(0.929322, 0.360084, 0.0818505, 0.0),
		vec4(0.80407, 0.359791, 0.47331, 0.0),
		vec4(-0.25027, 0.355984, -0.900356, 0.0),
		vec4(-0.471688, 0.336709, 0.814947, 0.0),
		vec4(-0.640594, 0.328744, -0.69395, 0.0),
		vec4(-0.0555797, 0.327945, 0.943061, 0.0),
		vec4(0.767863, 0.325765, -0.551601, 0.0),
		vec4(-0.281545, 0.311809, 0.907473, 0.0),
		vec4(-0.441014, 0.306847, -0.843416, 0.0),
		vec4(0.35438, 0.298671, -0.886121, 0.0),
		vec4(0.90302, 0.297822, -0.309609, 0.0),
		vec4(0.264062, 0.284122, 0.921708, 0.0),
		vec4(0.538591, 0.283068, -0.793594, 0.0),
		vec4(-0.947152, 0.281932, -0.153025, 0.0),
		vec4(-0.790645, 0.280032, -0.544484, 0.0),
		vec4(-0.0750546, 0.2792, -0.957295, 0.0),
		vec4(-0.832267, 0.276625, 0.480427, 0.0),
		vec4(-0.635912, 0.271523, 0.72242, 0.0),
		vec4(0.907413, 0.267819, 0.323843, 0.0),
		vec4(0.970076, 0.233196, -0.0676157, 0.0),
		vec4(0.659257, 0.231781, 0.715302, 0.0),
		vec4(-0.943731, 0.229174, 0.238434, 0.0),
		vec4(0.447303, 0.228243, 0.864769, 0.0),
		vec4(0.0797913, 0.189426, 0.978648, 0.0),
		vec4(-0.899127, 0.188497, -0.395018, 0.0),
		vec4(0.687735, 0.188479, -0.701068, 0.0),
		vec4(0.807364, 0.167309, 0.565836, 0.0),
		vec4(0.873266, 0.163268, -0.459075, 0.0),
		vec4(0.125062, 0.163121, -0.978648, 0.0),
		vec4(-0.59722, 0.157426, -0.786477, 0.0),
		vec4(-0.987918, 0.154939, -0.00355872, 0.0),
		vec4(0.973138, 0.150315, 0.174377, 0.0),
		vec4(-0.763121, 0.144504, 0.629893, 0.0),
		vec4(-0.32551, 0.134366, -0.935943, 0.0),
		vec4(-0.471993, 0.130528, 0.871886, 0.0),
		vec4(-0.23376, 0.123549, 0.964413, 0.0),
		vec4(0.312438, 0.114102, -0.943061, 0.0),
		vec4(0.971063, 0.0995586, -0.217082, 0.0),
		vec4(-0.916675, 0.0961267, 0.3879, 0.0),
		vec4(-0.764937, 0.0953341, -0.637011, 0.0),
		vec4(-0.0878188, 0.0804492, -0.992883, 0.0),
		vec4(0.0146987, -0.167485, -0.985765, 0.0),
		vec4(0.125062, 0.163121, -0.978648, 0.0),
		vec4(-0.233294, -0.0412664, -0.97153, 0.0),
		vec4(0.22309, -0.141912, -0.964413, 0.0),
		vec4(-0.0750546, 0.2792, -0.957295, 0.0),
		vec4(-0.143668, -0.276625, -0.950178, 0.0),
		vec4(0.312438, 0.114102, -0.943061, 0.0),
		vec4(-0.32551, 0.134366, -0.935943, 0.0),
		vec4(0.157042, -0.33559, -0.928826, 0.0),
		vec4(0.116088, 0.370105, -0.921708, 0.0),
		vec4(-0.349875, -0.202759, -0.914591, 0.0),
		vec4(0.410311, -0.0902059, -0.907473, 0.0),
		vec4(-0.25027, 0.355984, -0.900356, 0.0),
		vec4(-0.0577765, -0.445855, -0.893238, 0.0),
		vec4(0.35438, 0.298671, -0.886121, 0.0),
		vec4(-0.476408, 0.0197013, -0.879004, 0.0),
		vec4(0.34712, -0.345431, -0.871886, 0.0),
		vec4(-0.0231956, 0.501634, -0.864769, 0.0),
		vec4(-0.329473, -0.394819, -0.857651, 0.0),
		vec4(0.521224, 0.0701295, -0.850534, 0.0),
		vec4(-0.441014, 0.306847, -0.843416, 0.0),
		vec4(0.120335, -0.534905, -0.836299, 0.0),
		vec4(0.277914, 0.484997, -0.829181, 0.0),
		vec4(-0.542459, -0.173059, -0.822064, 0.0),
		vec4(0.526097, -0.243071, -0.814947, 0.0),
		vec4(-0.22755, 0.543721, -0.807829, 0.0),
		vec4(-0.20275, -0.563696, -0.800712, 0.0),
		vec4(0.538591, 0.283068, -0.793594, 0.0),
		vec4(-0.59722, 0.157426, -0.786477, 0.0),
		vec4(0.338876, -0.527031, -0.779359, 0.0),
		vec4(0.10761, 0.626149, -0.772242, 0.0),
		vec4(-0.509069, -0.39425, -0.765125, 0.0),
		vec4(0.650019, -0.0538536, -0.758007, 0.0),
		vec4(-0.448482, 0.484797, -0.75089, 0.0),
		vec4(0.00325993, -0.668425, -0.743772, 0.0),
		vec4(0.454376, 0.500882, -0.736655, 0.0),
		vec4(-0.681022, -0.0631161, -0.729537, 0.0),
		vec4(0.550784, -0.418027, -0.72242, 0.0),
		vec4(-0.125075, 0.687531, -0.715302, 0.0),
		vec4(-0.376035, -0.597555, -0.708185, 0.0),
		vec4(0.687735, 0.188479, -0.701068, 0.0),
		vec4(-0.640594, 0.328744, -0.69395, 0.0),
		vec4(0.252652, -0.681489, -0.686833, 0.0),
		vec4(0.276553, 0.679342, -0.679715, 0.0),
		vec4(-0.668713, -0.316914, -0.672598, 0.0),
		vec4(0.713284, -0.219914, -0.66548, 0.0),
		vec4(-0.380581, 0.649397, -0.658363, 0.0),
		vec4(-0.159327, -0.741953, -0.651246, 0.0),
		vec4(0.623598, 0.442973, -0.644128, 0.0),
		vec4(-0.764937, 0.0953341, -0.637011, 0.0),
		vec4(0.50342, -0.591441, -0.629893, 0.0),
		vec4(0.0285147, 0.781881, -0.622776, 0.0),
		vec4(-0.553118, -0.561271, -0.615658, 0.0),
		vec4(0.792487, 0.0405188, -0.608541, 0.0),
		vec4(-0.615894, 0.508885, -0.601424, 0.0),
		vec4(0.111128, -0.796524, -0.594306, 0.0),
		vec4(0.459057, 0.66669, -0.587189, 0.0),
		vec4(-0.793823, -0.182655, -0.580071, 0.0),
		vec4(0.713091, -0.404011, -0.572954, 0.0),
		vec4(-0.254427, 0.784281, -0.565836, 0.0),
		vec4(-0.344176, -0.75457, -0.558719, 0.0),
		vec4(0.767863, 0.325765, -0.551601, 0.0),
		vec4(-0.790645, 0.280032, -0.544484, 0.0),
		vec4(0.395987, -0.744601, -0.537367, 0.0),
		vec4(0.212107, 0.820882, -0.530249, 0.0),
		vec4(-0.714596, -0.46442, -0.523132, 0.0),
		vec4(0.844901, -0.140966, -0.516014, 0.0),
		vec4(-0.530399, 0.678012, -0.508897, 0.0),
		vec4(-0.0672145, -0.86238, -0.501779, 0.0),
		vec4(0.63508, 0.593282, -0.494662, 0.0),
		vec4(-0.873057, -0.00851604, -0.487544, 0.0),
		vec4(0.652446, -0.586092, -0.480427, 0.0),
		vec4(-0.0855685, 0.87673, -0.47331, 0.0),
		vec4(-0.531402, -0.707302, -0.466192, 0.0),
		vec4(0.873266, 0.163268, -0.459075, 0.0),
		vec4(-0.757297, 0.471419, -0.451957, 0.0),
		vec4(0.240929, -0.862595, -0.44484, 0.0),
		vec4(0.406604, 0.801918, -0.437722, 0.0),
		vec4(-0.844715, -0.31786, -0.430605, 0.0),
		vec4(0.840698, -0.337469, -0.423488, 0.0),
		vec4(-0.393372, 0.819692, -0.41637, 0.0),
		vec4(-0.264569, -0.873221, -0.409253, 0.0),
		vec4(0.787655, 0.466783, -0.402135, 0.0),
		vec4(-0.899127, 0.188497, -0.395018, 0.0),
		vec4(0.537426, -0.748803, -0.3879, 0.0),
		vec4(0.10988, 0.918113, -0.380783, 0.0),
		vec4(-0.703397, -0.604655, -0.373665, 0.0),
		vec4(0.929935, -0.0293736, -0.366548, 0.0),
		vec4(-0.667848, 0.651758, -0.359431, 0.0),
		vec4(0.0523457, -0.934417, -0.352313, 0.0),
		vec4(0.594269, 0.726419, -0.345196, 0.0),
		vec4(-0.931445, -0.134586, -0.338078, 0.0),
		vec4(0.779818, -0.531365, -0.330961, 0.0),
		vec4(-0.216646, 0.920972, -0.323843, 0.0),
		vec4(-0.463536, -0.827538, -0.316726, 0.0),
		vec4(0.90302, 0.297822, -0.309609, 0.0),
		vec4(-0.869121, 0.391316, -0.302491, 0.0),
		vec4(0.377414, -0.877675, -0.295374, 0.0),
		vec4(0.315283, 0.90416, -0.288256, 0.0),
		vec4(-0.845093, -0.45473, -0.281139, 0.0),
		vec4(0.932304, -0.236053, -0.274021, 0.0),
		vec4(-0.529099, 0.805492, -0.266904, 0.0),
		vec4(-0.154271, -0.953264, -0.259786, 0.0),
		vec4(0.759155, 0.599868, -0.252669, 0.0),
		vec4(-0.966808, 0.0706117, -0.245552, 0.0),
		vec4(0.666415, -0.706428, -0.238434, 0.0),
		vec4(-0.0142336, 0.972774, -0.231317, 0.0),
		vec4(-0.647711, -0.728152, -0.224199, 0.0),
		vec4(0.971063, 0.0995586, -0.217082, 0.0),
		vec4(-0.784531, 0.583461, -0.209964, 0.0),
		vec4(0.18465, -0.961643, -0.202847, 0.0),
		vec4(0.514185, 0.835047, -0.19573, 0.0),
		vec4(-0.944551, -0.268793, -0.188612, 0.0),
		vec4(0.879247, -0.440436, -0.181495, 0.0),
		vec4(-0.351279, 0.919889, -0.174377, 0.0),
		vec4(-0.362808, -0.91673, -0.16726, 0.0),
		vec4(0.887828, 0.431411, -0.160142, 0.0),
		vec4(-0.947152, 0.281932, -0.153025, 0.0),
		vec4(0.508511, -0.848603, -0.145907, 0.0),
		vec4(0.198468, 0.970231, -0.13879, 0.0),
		vec4(-0.802512, -0.581925, -0.131673, 0.0),
		vec4(0.985746, -0.113101, -0.124555, 0.0),
		vec4(-0.651028, 0.749914, -0.117438, 0.0),
		vec4(-0.0265331, -0.993542, -0.11032, 0.0),
		vec4(0.691224, 0.715234, -0.103203, 0.0),
		vec4(-0.993532, -0.0605193, -0.0960854, 0.0),
		vec4(0.773993, -0.626912, -0.088968, 0.0),
		vec4(0.113379, -0.0364603, 0.992883, 0.0),
		vec4(-0.152788, -0.0701635, 0.985765, 0.0),
		vec4(0.0797913, 0.189426, 0.978648, 0.0),
		vec4(0.0796687, -0.223119, 0.97153, 0.0),
		vec4(-0.23376, 0.123549, 0.964413, 0.0),
		vec4(0.279732, 0.0730449, 0.957295, 0.0),
		vec4(-0.169189, -0.261795, 0.950178, 0.0),
		vec4(-0.0555797, 0.327945, 0.943061, 0.0),
		vec4(0.27792, -0.216267, 0.935943, 0.0),
		vec4(-0.369322, -0.0297376, 0.928826, 0.0),
		vec4(0.264062, 0.284122, 0.921708, 0.0),
		vec4(-0.002908, -0.40437, 0.914591, 0.0),
		vec4(-0.281545, 0.311809, 0.907473, 0.0),
		vec4(0.433203, -0.04116, 0.900356, 0.0),
		vec4(-0.358747, -0.270971, 0.893238, 0.0),
		vec4(0.084005, 0.455777, 0.886121, 0.0),
		vec4(0.25302, -0.404145, 0.879004, 0.0),
		vec4(-0.471993, 0.130528, 0.871886, 0.0),
		vec4(0.447303, 0.228243, 0.864769, 0.0),
		vec4(-0.17987, -0.481748, 0.857651, 0.0),
		vec4(-0.197167, 0.487563, 0.850534, 0.0),
		vec4(0.484965, -0.231209, 0.843416, 0.0),
		vec4(-0.52431, -0.160323, 0.836299, 0.0),
		vec4(0.283747, 0.481607, 0.829181, 0.0),
		vec4(0.118258, -0.556979, 0.822064, 0.0),
		vec4(-0.471688, 0.336709, 0.814947, 0.0),
		vec4(0.585059, 0.0715412, 0.807829, 0.0),
		vec4(-0.38934, -0.455275, 0.800712, 0.0),
		vec4(-0.0207677, 0.608093, 0.793594, 0.0),
		vec4(0.432497, -0.440908, 0.786477, 0.0),
		vec4(-0.625684, 0.0334424, 0.779359, 0.0),
		vec4(0.490709, 0.403543, 0.772242, 0.0),
		vec4(-0.090446, -0.637498, 0.765125, 0.0),
		vec4(-0.368659, 0.538067, 0.758007, 0.0),
		vec4(0.643265, -0.149581, 0.75089, 0.0),
		vec4(-0.582339, -0.328153, 0.743772, 0.0),
		vec4(0.210169, 0.642782, 0.736655, 0.0),
		vec4(0.282389, -0.622922, 0.729537, 0.0),
		vec4(-0.635912, 0.271523, 0.72242, 0.0),
		vec4(0.659257, 0.231781, 0.715302, 0.0),
		vec4(-0.332951, -0.62259, 0.708185, 0.0),
		vec4(-0.176798, 0.69083, 0.701068, 0.0),
		vec4(0.602815, -0.39376, 0.69395, 0.0),
		vec4(-0.717181, -0.117952, 0.686833, 0.0),
		vec4(0.453268, 0.576659, 0.679715, 0.0),
		vec4(0.0557948, -0.737902, 0.672598, 0.0),
		vec4(-0.544259, 0.510802, 0.66548, 0.0),
		vec4(0.752646, -0.00908358, 0.658363, 0.0),
		vec4(-0.56571, -0.505817, 0.651246, 0.0),
		vec4(0.0760642, 0.761126, 0.644128, 0.0),
		vec4(0.461601, -0.617367, 0.637011, 0.0),
		vec4(-0.763121, 0.144504, 0.629893, 0.0),
		vec4(0.665174, 0.411939, 0.622776, 0.0),
		vec4(-0.21374, -0.758472, 0.615658, 0.0),
		vec4(-0.357215, 0.708573, 0.608541, 0.0),
		vec4(0.74709, -0.2831, 0.601424, 0.0),
		vec4(-0.747044, -0.297869, 0.594306, 0.0),
		vec4(0.351905, 0.728953, 0.587189, 0.0),
		vec4(0.234389, -0.780115, 0.580071, 0.0),
		vec4(-0.704106, 0.419475, 0.572954, 0.0),
		vec4(0.807364, 0.167309, 0.565836, 0.0),
		vec4(-0.48514, -0.67266, 0.558719, 0.0),
		vec4(-0.0972019, 0.828425, 0.551601, 0.0),
		vec4(0.634796, -0.548244, 0.544484, 0.0),
		vec4(-0.842988, -0.0246745, 0.537367, 0.0),
		vec4(0.608148, 0.590755, 0.530249, 0.0),
		vec4(-0.0496373, -0.850805, 0.523132, 0.0),
		vec4(-0.540842, 0.664243, 0.516014, 0.0),
		vec4(0.851693, -0.125075, 0.508897, 0.0),
		vec4(-0.715949, -0.485422, 0.501779, 0.0),
		vec4(0.200966, 0.845531, 0.494662, 0.0),
		vec4(0.424915, -0.762724, 0.487544, 0.0),
		vec4(-0.832267, 0.276625, 0.480427, 0.0),
		vec4(0.80407, 0.359791, 0.47331, 0.0),
		vec4(-0.351365, -0.811916, 0.466192, 0.0),
		vec4(-0.290571, 0.839535, 0.459075, 0.0),
		vec4(0.784559, -0.424502, 0.451957, 0.0),
		vec4(-0.86872, -0.217815, 0.44484, 0.0),
		vec4(0.495363, 0.750343, 0.437722, 0.0),
		vec4(0.142124, -0.89128, 0.430605, 0.0),
		vec4(-0.709482, 0.563289, 0.423488, 0.0),
		vec4(0.906931, 0.0641287, 0.41637, 0.0),
		vec4(-0.627643, -0.662251, 0.409253, 0.0),
		vec4(0.015514, 0.915449, 0.402135, 0.0),
		vec4(0.608988, -0.687819, 0.395018, 0.0),
		vec4(-0.916675, 0.0961267, 0.3879, 0.0),
		vec4(0.743242, 0.550087, 0.380783, 0.0),
		vec4(-0.177018, -0.910516, 0.373665, 0.0),
		vec4(-0.485998, 0.793378, 0.366548, 0.0),
		vec4(0.896944, -0.25749, 0.359431, 0.0),
		vec4(-0.837737, -0.41722, 0.352313, 0.0),
		vec4(0.336844, 0.876, 0.345196, 0.0),
		vec4(0.344298, -0.875878, 0.338078, 0.0),
		vec4(-0.847791, 0.414384, 0.330961, 0.0),
		vec4(0.907413, 0.267819, 0.323843, 0.0),
		vec4(-0.489431, -0.812491, 0.316726, 0.0),
		vec4(-0.188404, 0.932012, 0.309609, 0.0),
		vec4(0.770338, -0.561319, 0.302491, 0.0),
		vec4(-0.949404, -0.106705, 0.295374, 0.0),
		vec4(0.62941, 0.721631, 0.288256, 0.0),
		vec4(0.0233991, -0.959382, 0.281139, 0.0),
		vec4(-0.666733, 0.693094, 0.274021, 0.0),
		vec4(0.961802, -0.060821, 0.266904, 0.0),
		vec4(-0.7518, -0.60606, 0.259786, 0.0),
		vec4(0.145249, 0.956588, 0.252669, 0.0),
		vec4(0.540084, -0.804993, 0.245552, 0.0),
		vec4(-0.943731, 0.229174, 0.238434, 0.0),
		vec4(0.852189, 0.469325, 0.231317, 0.0),
		vec4(-0.311886, -0.923288, 0.224199, 0.0),
		vec4(-0.394348, 0.892953, 0.217082, 0.0),
		vec4(0.895386, -0.392683, 0.209964, 0.0),
		vec4(-0.926904, -0.315756, 0.202847, 0.0),
		vec4(0.470875, 0.860213, 0.19573, 0.0),
		vec4(0.23419, -0.953719, 0.188612, 0.0),
		vec4(-0.818027, 0.545794, 0.181495, 0.0),
		vec4(0.973138, 0.150315, 0.174377, 0.0),
		vec4(-0.616798, -0.769145, 0.16726, 0.0),
		vec4(-0.0648203, 0.984963, 0.160142, 0.0),
		vec4(0.713945, -0.683276, 0.153025, 0.0),
		vec4(-0.989063, 0.021587, 0.145907, 0.0),
		vec4(0.744655, 0.652859, 0.13879, 0.0),
		vec4(-0.108192, -0.985371, 0.131673, 0.0),
		vec4(-0.586375, 0.800406, 0.124555, 0.0),
		vec4(0.973892, -0.194274, 0.117438, 0.0),
		vec4(-0.850046, -0.515026, 0.11032, 0.0),
		vec4(0.279116, 0.954695, 0.103203, 0.0),
		vec4(0.43939, -0.893143, 0.0960854, 0.0),
		vec4(-0.927918, 0.362012, 0.088968, 0.0),
	};

	m_rayDirectionBuffer = bufferM->buildBuffer(
		move(string("rayDirectionBuffer")),
		vectorData.data(),
		vectorData.size() * sizeof(vec4),
		VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
}

/////////////////////////////////////////////////////////////////////////////////////////////

void LightBounceVoxelIrradianceTechnique::buildTriangulatedIndicesBuffer()
{
	vector<uint> vectorData =
	{
		MathUtil::convertIvec3ToRGB8(ivec3(0, 1, 2)), // -x voxel face indices
		MathUtil::convertIvec3ToRGB8(ivec3(1, 0, 3)),
		MathUtil::convertIvec3ToRGB8(ivec3(0, 2, 4)),
		MathUtil::convertIvec3ToRGB8(ivec3(1, 3, 5)),
		MathUtil::convertIvec3ToRGB8(ivec3(4, 2, 6)),
		MathUtil::convertIvec3ToRGB8(ivec3(3, 0, 7)),
		MathUtil::convertIvec3ToRGB8(ivec3(2, 1, 8)),
		MathUtil::convertIvec3ToRGB8(ivec3(0, 4, 9)),
		MathUtil::convertIvec3ToRGB8(ivec3(7, 0, 9)),
		MathUtil::convertIvec3ToRGB8(ivec3(6, 2, 10)),
		MathUtil::convertIvec3ToRGB8(ivec3(2, 8, 10)),
		MathUtil::convertIvec3ToRGB8(ivec3(5, 3, 11)),
		MathUtil::convertIvec3ToRGB8(ivec3(1, 5, 12)),
		MathUtil::convertIvec3ToRGB8(ivec3(8, 1, 12)),
		MathUtil::convertIvec3ToRGB8(ivec3(3, 7, 13)),
		MathUtil::convertIvec3ToRGB8(ivec3(11, 3, 13)),
		MathUtil::convertIvec3ToRGB8(ivec3(4, 6, 14)),
		MathUtil::convertIvec3ToRGB8(ivec3(6, 10, 15)),
		MathUtil::convertIvec3ToRGB8(ivec3(9, 4, 16)),
		MathUtil::convertIvec3ToRGB8(ivec3(4, 14, 16)),
		MathUtil::convertIvec3ToRGB8(ivec3(14, 6, 17)),
		MathUtil::convertIvec3ToRGB8(ivec3(6, 15, 17)),
		MathUtil::convertIvec3ToRGB8(ivec3(5, 11, 18)),
		MathUtil::convertIvec3ToRGB8(ivec3(11, 13, 19)),
		MathUtil::convertIvec3ToRGB8(ivec3(10, 8, 20)),
		MathUtil::convertIvec3ToRGB8(ivec3(13, 7, 21)),
		MathUtil::convertIvec3ToRGB8(ivec3(7, 9, 22)),
		MathUtil::convertIvec3ToRGB8(ivec3(21, 7, 22)),
		MathUtil::convertIvec3ToRGB8(ivec3(9, 16, 22)),
		MathUtil::convertIvec3ToRGB8(ivec3(12, 5, 23)),
		MathUtil::convertIvec3ToRGB8(ivec3(5, 18, 23)),
		MathUtil::convertIvec3ToRGB8(ivec3(18, 11, 24)),
		MathUtil::convertIvec3ToRGB8(ivec3(11, 19, 24)),
		MathUtil::convertIvec3ToRGB8(ivec3(8, 12, 25)),
		MathUtil::convertIvec3ToRGB8(ivec3(20, 8, 25)),
		MathUtil::convertIvec3ToRGB8(ivec3(12, 23, 25)),
		MathUtil::convertIvec3ToRGB8(ivec3(15, 10, 26)),
		MathUtil::convertIvec3ToRGB8(ivec3(10, 20, 26)),
		MathUtil::convertIvec3ToRGB8(ivec3(19, 13, 27)),
		MathUtil::convertIvec3ToRGB8(ivec3(13, 21, 27)),
		MathUtil::convertIvec3ToRGB8(ivec3(22, 16, 28)),
		MathUtil::convertIvec3ToRGB8(ivec3(16, 14, 29)),
		MathUtil::convertIvec3ToRGB8(ivec3(14, 17, 29)),
		MathUtil::convertIvec3ToRGB8(ivec3(17, 15, 30)),
		MathUtil::convertIvec3ToRGB8(ivec3(28, 16, 31)),
		MathUtil::convertIvec3ToRGB8(ivec3(16, 29, 31)),
		MathUtil::convertIvec3ToRGB8(ivec3(25, 23, 32)),
		MathUtil::convertIvec3ToRGB8(ivec3(29, 17, 33)),
		MathUtil::convertIvec3ToRGB8(ivec3(17, 30, 33)),
		MathUtil::convertIvec3ToRGB8(ivec3(24, 19, 34)),
		MathUtil::convertIvec3ToRGB8(ivec3(15, 26, 35)),
		MathUtil::convertIvec3ToRGB8(ivec3(30, 15, 35)),
		MathUtil::convertIvec3ToRGB8(ivec3(23, 18, 36)),
		MathUtil::convertIvec3ToRGB8(ivec3(18, 24, 36)),
		MathUtil::convertIvec3ToRGB8(ivec3(19, 27, 37)),
		MathUtil::convertIvec3ToRGB8(ivec3(34, 19, 37)),
		MathUtil::convertIvec3ToRGB8(ivec3(32, 23, 38)),
		MathUtil::convertIvec3ToRGB8(ivec3(23, 36, 38)),
		MathUtil::convertIvec3ToRGB8(ivec3(24, 34, 39)),
		MathUtil::convertIvec3ToRGB8(ivec3(36, 24, 39)),
		MathUtil::convertIvec3ToRGB8(ivec3(21, 22, 40)),
		MathUtil::convertIvec3ToRGB8(ivec3(22, 28, 40)),
		MathUtil::convertIvec3ToRGB8(ivec3(26, 20, 41)),
		MathUtil::convertIvec3ToRGB8(ivec3(20, 25, 42)),
		MathUtil::convertIvec3ToRGB8(ivec3(41, 20, 42)),
		MathUtil::convertIvec3ToRGB8(ivec3(25, 32, 42)),
		MathUtil::convertIvec3ToRGB8(ivec3(27, 21, 43)),
		MathUtil::convertIvec3ToRGB8(ivec3(21, 40, 43)),
		MathUtil::convertIvec3ToRGB8(ivec3(28, 31, 44)),
		MathUtil::convertIvec3ToRGB8(ivec3(40, 28, 45)),
		MathUtil::convertIvec3ToRGB8(ivec3(28, 44, 45)),
		MathUtil::convertIvec3ToRGB8(ivec3(33, 30, 46)),
		MathUtil::convertIvec3ToRGB8(ivec3(35, 26, 47)),
		MathUtil::convertIvec3ToRGB8(ivec3(26, 41, 47)),
		MathUtil::convertIvec3ToRGB8(ivec3(30, 35, 48)),
		MathUtil::convertIvec3ToRGB8(ivec3(46, 30, 48)),
		MathUtil::convertIvec3ToRGB8(ivec3(37, 27, 49)),
		MathUtil::convertIvec3ToRGB8(ivec3(27, 43, 49)),
		MathUtil::convertIvec3ToRGB8(ivec3(31, 29, 50)),
		MathUtil::convertIvec3ToRGB8(ivec3(29, 33, 50)),
		MathUtil::convertIvec3ToRGB8(ivec3(42, 32, 51)),
		MathUtil::convertIvec3ToRGB8(ivec3(39, 34, 52)),
		MathUtil::convertIvec3ToRGB8(ivec3(32, 38, 53)),
		MathUtil::convertIvec3ToRGB8(ivec3(51, 32, 53)),
		MathUtil::convertIvec3ToRGB8(ivec3(34, 37, 54)),
		MathUtil::convertIvec3ToRGB8(ivec3(52, 34, 54)),
		MathUtil::convertIvec3ToRGB8(ivec3(33, 46, 55)),
		MathUtil::convertIvec3ToRGB8(ivec3(50, 33, 55)),
		MathUtil::convertIvec3ToRGB8(ivec3(44, 31, 56)),
		MathUtil::convertIvec3ToRGB8(ivec3(31, 50, 56)),
		MathUtil::convertIvec3ToRGB8(ivec3(35, 47, 57)),
		MathUtil::convertIvec3ToRGB8(ivec3(48, 35, 57)),
		MathUtil::convertIvec3ToRGB8(ivec3(38, 36, 58)),
		MathUtil::convertIvec3ToRGB8(ivec3(53, 38, 58)),
		MathUtil::convertIvec3ToRGB8(ivec3(36, 39, 58)),
		MathUtil::convertIvec3ToRGB8(ivec3(39, 52, 59)),
		MathUtil::convertIvec3ToRGB8(ivec3(58, 39, 59)),
		MathUtil::convertIvec3ToRGB8(ivec3(37, 49, 60)),
		MathUtil::convertIvec3ToRGB8(ivec3(54, 37, 60)),
		MathUtil::convertIvec3ToRGB8(ivec3(43, 40, 61)),
		MathUtil::convertIvec3ToRGB8(ivec3(40, 45, 61)),
		MathUtil::convertIvec3ToRGB8(ivec3(41, 42, 62)),
		MathUtil::convertIvec3ToRGB8(ivec3(42, 51, 62)),
		MathUtil::convertIvec3ToRGB8(ivec3(45, 44, 63)),
		MathUtil::convertIvec3ToRGB8(ivec3(44, 56, 63)),
		MathUtil::convertIvec3ToRGB8(ivec3(47, 41, 64)),
		MathUtil::convertIvec3ToRGB8(ivec3(41, 62, 64)),
		MathUtil::convertIvec3ToRGB8(ivec3(49, 43, 65)),
		MathUtil::convertIvec3ToRGB8(ivec3(43, 61, 65)),
		MathUtil::convertIvec3ToRGB8(ivec3(55, 46, 66)),
		MathUtil::convertIvec3ToRGB8(ivec3(46, 48, 67)),
		MathUtil::convertIvec3ToRGB8(ivec3(66, 46, 67)),
		MathUtil::convertIvec3ToRGB8(ivec3(53, 58, 68)),
		MathUtil::convertIvec3ToRGB8(ivec3(61, 45, 69)),
		MathUtil::convertIvec3ToRGB8(ivec3(45, 63, 69)),
		MathUtil::convertIvec3ToRGB8(ivec3(52, 54, 70)),
		MathUtil::convertIvec3ToRGB8(ivec3(57, 47, 71)),
		MathUtil::convertIvec3ToRGB8(ivec3(47, 64, 71)),
		MathUtil::convertIvec3ToRGB8(ivec3(59, 52, 72)),
		MathUtil::convertIvec3ToRGB8(ivec3(52, 70, 72)),
		MathUtil::convertIvec3ToRGB8(ivec3(48, 57, 73)),
		MathUtil::convertIvec3ToRGB8(ivec3(67, 48, 73)),
		MathUtil::convertIvec3ToRGB8(ivec3(57, 71, 73)),
		MathUtil::convertIvec3ToRGB8(ivec3(63, 56, 74)),
		MathUtil::convertIvec3ToRGB8(ivec3(51, 53, 75)),
		MathUtil::convertIvec3ToRGB8(ivec3(53, 68, 75)),
		MathUtil::convertIvec3ToRGB8(ivec3(62, 51, 76)),
		MathUtil::convertIvec3ToRGB8(ivec3(51, 75, 76)),
		MathUtil::convertIvec3ToRGB8(ivec3(0, 1, 2)), // +x voxel face indices
		MathUtil::convertIvec3ToRGB8(ivec3(1, 0, 4)),
		MathUtil::convertIvec3ToRGB8(ivec3(0, 3, 5)),
		MathUtil::convertIvec3ToRGB8(ivec3(4, 0, 5)),
		MathUtil::convertIvec3ToRGB8(ivec3(0, 2, 6)),
		MathUtil::convertIvec3ToRGB8(ivec3(3, 0, 6)),
		MathUtil::convertIvec3ToRGB8(ivec3(2, 1, 7)),
		MathUtil::convertIvec3ToRGB8(ivec3(7, 1, 8)),
		MathUtil::convertIvec3ToRGB8(ivec3(1, 4, 9)),
		MathUtil::convertIvec3ToRGB8(ivec3(8, 1, 9)),
		MathUtil::convertIvec3ToRGB8(ivec3(5, 3, 10)),
		MathUtil::convertIvec3ToRGB8(ivec3(10, 3, 11)),
		MathUtil::convertIvec3ToRGB8(ivec3(3, 6, 12)),
		MathUtil::convertIvec3ToRGB8(ivec3(11, 3, 12)),
		MathUtil::convertIvec3ToRGB8(ivec3(4, 5, 13)),
		MathUtil::convertIvec3ToRGB8(ivec3(6, 2, 14)),
		MathUtil::convertIvec3ToRGB8(ivec3(2, 7, 15)),
		MathUtil::convertIvec3ToRGB8(ivec3(14, 2, 15)),
		MathUtil::convertIvec3ToRGB8(ivec3(5, 10, 16)),
		MathUtil::convertIvec3ToRGB8(ivec3(13, 5, 16)),
		MathUtil::convertIvec3ToRGB8(ivec3(7, 8, 17)),
		MathUtil::convertIvec3ToRGB8(ivec3(9, 4, 18)),
		MathUtil::convertIvec3ToRGB8(ivec3(4, 13, 18)),
		MathUtil::convertIvec3ToRGB8(ivec3(15, 7, 19)),
		MathUtil::convertIvec3ToRGB8(ivec3(7, 17, 19)),
		MathUtil::convertIvec3ToRGB8(ivec3(10, 11, 20)),
		MathUtil::convertIvec3ToRGB8(ivec3(12, 6, 21)),
		MathUtil::convertIvec3ToRGB8(ivec3(6, 14, 21)),
		MathUtil::convertIvec3ToRGB8(ivec3(8, 9, 22)),
		MathUtil::convertIvec3ToRGB8(ivec3(9, 18, 22)),
		MathUtil::convertIvec3ToRGB8(ivec3(17, 8, 23)),
		MathUtil::convertIvec3ToRGB8(ivec3(8, 22, 23)),
		MathUtil::convertIvec3ToRGB8(ivec3(16, 10, 24)),
		MathUtil::convertIvec3ToRGB8(ivec3(10, 20, 24)),
		MathUtil::convertIvec3ToRGB8(ivec3(11, 12, 25)),
		MathUtil::convertIvec3ToRGB8(ivec3(12, 21, 25)),
		MathUtil::convertIvec3ToRGB8(ivec3(20, 11, 26)),
		MathUtil::convertIvec3ToRGB8(ivec3(11, 25, 26)),
		MathUtil::convertIvec3ToRGB8(ivec3(22, 18, 27)),
		MathUtil::convertIvec3ToRGB8(ivec3(13, 16, 28)),
		MathUtil::convertIvec3ToRGB8(ivec3(14, 15, 29)),
		MathUtil::convertIvec3ToRGB8(ivec3(21, 14, 29)),
		MathUtil::convertIvec3ToRGB8(ivec3(17, 23, 30)),
		MathUtil::convertIvec3ToRGB8(ivec3(19, 17, 31)),
		MathUtil::convertIvec3ToRGB8(ivec3(17, 30, 31)),
		MathUtil::convertIvec3ToRGB8(ivec3(15, 19, 32)),
		MathUtil::convertIvec3ToRGB8(ivec3(29, 15, 32)),
		MathUtil::convertIvec3ToRGB8(ivec3(18, 13, 33)),
		MathUtil::convertIvec3ToRGB8(ivec3(13, 28, 33)),
		MathUtil::convertIvec3ToRGB8(ivec3(27, 18, 33)),
		MathUtil::convertIvec3ToRGB8(ivec3(16, 24, 34)),
		MathUtil::convertIvec3ToRGB8(ivec3(28, 16, 34)),
		MathUtil::convertIvec3ToRGB8(ivec3(25, 21, 35)),
		MathUtil::convertIvec3ToRGB8(ivec3(21, 29, 36)),
		MathUtil::convertIvec3ToRGB8(ivec3(35, 21, 36)),
		MathUtil::convertIvec3ToRGB8(ivec3(24, 20, 37)),
		MathUtil::convertIvec3ToRGB8(ivec3(20, 26, 38)),
		MathUtil::convertIvec3ToRGB8(ivec3(37, 20, 38)),
		MathUtil::convertIvec3ToRGB8(ivec3(19, 31, 39)),
		MathUtil::convertIvec3ToRGB8(ivec3(32, 19, 39)),
		MathUtil::convertIvec3ToRGB8(ivec3(23, 22, 40)),
		MathUtil::convertIvec3ToRGB8(ivec3(22, 27, 40)),
		MathUtil::convertIvec3ToRGB8(ivec3(30, 23, 41)),
		MathUtil::convertIvec3ToRGB8(ivec3(23, 40, 41)),
		MathUtil::convertIvec3ToRGB8(ivec3(34, 24, 42)),
		MathUtil::convertIvec3ToRGB8(ivec3(24, 37, 42)),
		MathUtil::convertIvec3ToRGB8(ivec3(27, 33, 43)),
		MathUtil::convertIvec3ToRGB8(ivec3(31, 30, 44)),
		MathUtil::convertIvec3ToRGB8(ivec3(26, 25, 45)),
		MathUtil::convertIvec3ToRGB8(ivec3(38, 26, 45)),
		MathUtil::convertIvec3ToRGB8(ivec3(25, 35, 46)),
		MathUtil::convertIvec3ToRGB8(ivec3(45, 25, 46)),
		MathUtil::convertIvec3ToRGB8(ivec3(30, 41, 47)),
		MathUtil::convertIvec3ToRGB8(ivec3(44, 30, 47)),
		MathUtil::convertIvec3ToRGB8(ivec3(40, 27, 48)),
		MathUtil::convertIvec3ToRGB8(ivec3(27, 43, 48)),
		MathUtil::convertIvec3ToRGB8(ivec3(35, 36, 49)),
		MathUtil::convertIvec3ToRGB8(ivec3(33, 28, 50)),
		MathUtil::convertIvec3ToRGB8(ivec3(28, 34, 50)),
		MathUtil::convertIvec3ToRGB8(ivec3(29, 32, 51)),
		MathUtil::convertIvec3ToRGB8(ivec3(36, 29, 51)),
		MathUtil::convertIvec3ToRGB8(ivec3(37, 38, 52)),
		MathUtil::convertIvec3ToRGB8(ivec3(39, 31, 53)),
		MathUtil::convertIvec3ToRGB8(ivec3(31, 44, 53)),
		MathUtil::convertIvec3ToRGB8(ivec3(43, 33, 54)),
		MathUtil::convertIvec3ToRGB8(ivec3(33, 50, 54)),
		MathUtil::convertIvec3ToRGB8(ivec3(32, 39, 55)),
		MathUtil::convertIvec3ToRGB8(ivec3(51, 32, 55)),
		MathUtil::convertIvec3ToRGB8(ivec3(38, 45, 56)),
		MathUtil::convertIvec3ToRGB8(ivec3(52, 38, 56)),
		MathUtil::convertIvec3ToRGB8(ivec3(49, 36, 57)),
		MathUtil::convertIvec3ToRGB8(ivec3(36, 51, 57)),
		MathUtil::convertIvec3ToRGB8(ivec3(34, 42, 58)),
		MathUtil::convertIvec3ToRGB8(ivec3(50, 34, 58)),
		MathUtil::convertIvec3ToRGB8(ivec3(42, 37, 59)),
		MathUtil::convertIvec3ToRGB8(ivec3(37, 52, 59)),
		MathUtil::convertIvec3ToRGB8(ivec3(46, 35, 60)),
		MathUtil::convertIvec3ToRGB8(ivec3(35, 49, 60)),
		MathUtil::convertIvec3ToRGB8(ivec3(41, 40, 61)),
		MathUtil::convertIvec3ToRGB8(ivec3(47, 41, 61)),
		MathUtil::convertIvec3ToRGB8(ivec3(39, 53, 62)),
		MathUtil::convertIvec3ToRGB8(ivec3(55, 39, 62)),
		MathUtil::convertIvec3ToRGB8(ivec3(43, 54, 63)),
		MathUtil::convertIvec3ToRGB8(ivec3(44, 47, 64)),
		MathUtil::convertIvec3ToRGB8(ivec3(40, 48, 65)),
		MathUtil::convertIvec3ToRGB8(ivec3(61, 40, 65)),
		MathUtil::convertIvec3ToRGB8(ivec3(48, 43, 66)),
		MathUtil::convertIvec3ToRGB8(ivec3(43, 63, 66)),
		MathUtil::convertIvec3ToRGB8(ivec3(65, 48, 66)),
		MathUtil::convertIvec3ToRGB8(ivec3(58, 42, 67)),
		MathUtil::convertIvec3ToRGB8(ivec3(42, 59, 67)),
		MathUtil::convertIvec3ToRGB8(ivec3(53, 44, 68)),
		MathUtil::convertIvec3ToRGB8(ivec3(44, 64, 68)),
		MathUtil::convertIvec3ToRGB8(ivec3(62, 53, 68)),
		MathUtil::convertIvec3ToRGB8(ivec3(47, 61, 69)),
		MathUtil::convertIvec3ToRGB8(ivec3(64, 47, 69)),
		MathUtil::convertIvec3ToRGB8(ivec3(52, 56, 70)),
		MathUtil::convertIvec3ToRGB8(ivec3(45, 46, 71)),
		MathUtil::convertIvec3ToRGB8(ivec3(46, 60, 71)),
		MathUtil::convertIvec3ToRGB8(ivec3(56, 45, 71)),
		MathUtil::convertIvec3ToRGB8(ivec3(49, 57, 72)),
		MathUtil::convertIvec3ToRGB8(ivec3(60, 49, 72)),
		MathUtil::convertIvec3ToRGB8(ivec3(59, 52, 73)),
		MathUtil::convertIvec3ToRGB8(ivec3(52, 70, 73)),
		MathUtil::convertIvec3ToRGB8(ivec3(67, 59, 73)),
		MathUtil::convertIvec3ToRGB8(ivec3(51, 55, 74)),
		MathUtil::convertIvec3ToRGB8(ivec3(57, 51, 74)),
		MathUtil::convertIvec3ToRGB8(ivec3(54, 50, 75)),
		MathUtil::convertIvec3ToRGB8(ivec3(1, 0, 2)), // -y voxel face indices
		MathUtil::convertIvec3ToRGB8(ivec3(2, 0, 3)),
		MathUtil::convertIvec3ToRGB8(ivec3(0, 1, 4)),
		MathUtil::convertIvec3ToRGB8(ivec3(1, 2, 5)),
		MathUtil::convertIvec3ToRGB8(ivec3(1, 5, 6)),
		MathUtil::convertIvec3ToRGB8(ivec3(3, 0, 7)),
		MathUtil::convertIvec3ToRGB8(ivec3(4, 1, 8)),
		MathUtil::convertIvec3ToRGB8(ivec3(1, 6, 8)),
		MathUtil::convertIvec3ToRGB8(ivec3(0, 4, 9)),
		MathUtil::convertIvec3ToRGB8(ivec3(7, 0, 9)),
		MathUtil::convertIvec3ToRGB8(ivec3(2, 3, 10)),
		MathUtil::convertIvec3ToRGB8(ivec3(3, 7, 11)),
		MathUtil::convertIvec3ToRGB8(ivec3(5, 2, 12)),
		MathUtil::convertIvec3ToRGB8(ivec3(2, 10, 12)),
		MathUtil::convertIvec3ToRGB8(ivec3(10, 3, 13)),
		MathUtil::convertIvec3ToRGB8(ivec3(3, 11, 13)),
		MathUtil::convertIvec3ToRGB8(ivec3(8, 6, 14)),
		MathUtil::convertIvec3ToRGB8(ivec3(6, 5, 15)),
		MathUtil::convertIvec3ToRGB8(ivec3(4, 8, 16)),
		MathUtil::convertIvec3ToRGB8(ivec3(14, 6, 17)),
		MathUtil::convertIvec3ToRGB8(ivec3(6, 15, 17)),
		MathUtil::convertIvec3ToRGB8(ivec3(11, 7, 18)),
		MathUtil::convertIvec3ToRGB8(ivec3(9, 4, 19)),
		MathUtil::convertIvec3ToRGB8(ivec3(4, 16, 19)),
		MathUtil::convertIvec3ToRGB8(ivec3(7, 9, 20)),
		MathUtil::convertIvec3ToRGB8(ivec3(18, 7, 20)),
		MathUtil::convertIvec3ToRGB8(ivec3(9, 19, 20)),
		MathUtil::convertIvec3ToRGB8(ivec3(5, 12, 21)),
		MathUtil::convertIvec3ToRGB8(ivec3(15, 5, 21)),
		MathUtil::convertIvec3ToRGB8(ivec3(8, 14, 22)),
		MathUtil::convertIvec3ToRGB8(ivec3(16, 8, 22)),
		MathUtil::convertIvec3ToRGB8(ivec3(11, 18, 23)),
		MathUtil::convertIvec3ToRGB8(ivec3(13, 11, 24)),
		MathUtil::convertIvec3ToRGB8(ivec3(11, 23, 24)),
		MathUtil::convertIvec3ToRGB8(ivec3(10, 13, 25)),
		MathUtil::convertIvec3ToRGB8(ivec3(12, 10, 26)),
		MathUtil::convertIvec3ToRGB8(ivec3(10, 25, 26)),
		MathUtil::convertIvec3ToRGB8(ivec3(21, 12, 26)),
		MathUtil::convertIvec3ToRGB8(ivec3(14, 17, 27)),
		MathUtil::convertIvec3ToRGB8(ivec3(20, 19, 28)),
		MathUtil::convertIvec3ToRGB8(ivec3(22, 14, 29)),
		MathUtil::convertIvec3ToRGB8(ivec3(14, 27, 29)),
		MathUtil::convertIvec3ToRGB8(ivec3(17, 15, 30)),
		MathUtil::convertIvec3ToRGB8(ivec3(15, 21, 30)),
		MathUtil::convertIvec3ToRGB8(ivec3(13, 24, 31)),
		MathUtil::convertIvec3ToRGB8(ivec3(25, 13, 31)),
		MathUtil::convertIvec3ToRGB8(ivec3(27, 17, 32)),
		MathUtil::convertIvec3ToRGB8(ivec3(17, 30, 32)),
		MathUtil::convertIvec3ToRGB8(ivec3(21, 26, 33)),
		MathUtil::convertIvec3ToRGB8(ivec3(18, 20, 34)),
		MathUtil::convertIvec3ToRGB8(ivec3(23, 18, 34)),
		MathUtil::convertIvec3ToRGB8(ivec3(16, 22, 35)),
		MathUtil::convertIvec3ToRGB8(ivec3(19, 16, 36)),
		MathUtil::convertIvec3ToRGB8(ivec3(16, 35, 36)),
		MathUtil::convertIvec3ToRGB8(ivec3(28, 19, 36)),
		MathUtil::convertIvec3ToRGB8(ivec3(20, 28, 37)),
		MathUtil::convertIvec3ToRGB8(ivec3(34, 20, 37)),
		MathUtil::convertIvec3ToRGB8(ivec3(30, 21, 38)),
		MathUtil::convertIvec3ToRGB8(ivec3(21, 33, 38)),
		MathUtil::convertIvec3ToRGB8(ivec3(24, 23, 39)),
		MathUtil::convertIvec3ToRGB8(ivec3(23, 34, 40)),
		MathUtil::convertIvec3ToRGB8(ivec3(39, 23, 40)),
		MathUtil::convertIvec3ToRGB8(ivec3(22, 29, 41)),
		MathUtil::convertIvec3ToRGB8(ivec3(35, 22, 41)),
		MathUtil::convertIvec3ToRGB8(ivec3(31, 24, 42)),
		MathUtil::convertIvec3ToRGB8(ivec3(24, 39, 42)),
		MathUtil::convertIvec3ToRGB8(ivec3(27, 32, 43)),
		MathUtil::convertIvec3ToRGB8(ivec3(29, 27, 44)),
		MathUtil::convertIvec3ToRGB8(ivec3(27, 43, 44)),
		MathUtil::convertIvec3ToRGB8(ivec3(28, 36, 45)),
		MathUtil::convertIvec3ToRGB8(ivec3(26, 25, 46)),
		MathUtil::convertIvec3ToRGB8(ivec3(33, 26, 46)),
		MathUtil::convertIvec3ToRGB8(ivec3(37, 28, 47)),
		MathUtil::convertIvec3ToRGB8(ivec3(28, 45, 47)),
		MathUtil::convertIvec3ToRGB8(ivec3(25, 31, 48)),
		MathUtil::convertIvec3ToRGB8(ivec3(46, 25, 48)),
		MathUtil::convertIvec3ToRGB8(ivec3(38, 33, 49)),
		MathUtil::convertIvec3ToRGB8(ivec3(41, 29, 50)),
		MathUtil::convertIvec3ToRGB8(ivec3(29, 44, 50)),
		MathUtil::convertIvec3ToRGB8(ivec3(30, 38, 51)),
		MathUtil::convertIvec3ToRGB8(ivec3(33, 46, 52)),
		MathUtil::convertIvec3ToRGB8(ivec3(49, 33, 52)),
		MathUtil::convertIvec3ToRGB8(ivec3(32, 30, 53)),
		MathUtil::convertIvec3ToRGB8(ivec3(30, 51, 53)),
		MathUtil::convertIvec3ToRGB8(ivec3(43, 32, 53)),
		MathUtil::convertIvec3ToRGB8(ivec3(31, 42, 54)),
		MathUtil::convertIvec3ToRGB8(ivec3(48, 31, 54)),
		MathUtil::convertIvec3ToRGB8(ivec3(39, 40, 55)),
		MathUtil::convertIvec3ToRGB8(ivec3(34, 37, 56)),
		MathUtil::convertIvec3ToRGB8(ivec3(40, 34, 56)),
		MathUtil::convertIvec3ToRGB8(ivec3(36, 35, 57)),
		MathUtil::convertIvec3ToRGB8(ivec3(45, 36, 57)),
		MathUtil::convertIvec3ToRGB8(ivec3(35, 41, 58)),
		MathUtil::convertIvec3ToRGB8(ivec3(57, 35, 58)),
		MathUtil::convertIvec3ToRGB8(ivec3(42, 39, 59)),
		MathUtil::convertIvec3ToRGB8(ivec3(39, 55, 59)),
		MathUtil::convertIvec3ToRGB8(ivec3(37, 47, 60)),
		MathUtil::convertIvec3ToRGB8(ivec3(56, 37, 60)),
		MathUtil::convertIvec3ToRGB8(ivec3(44, 43, 61)),
		MathUtil::convertIvec3ToRGB8(ivec3(55, 40, 62)),
		MathUtil::convertIvec3ToRGB8(ivec3(40, 56, 62)),
		MathUtil::convertIvec3ToRGB8(ivec3(38, 49, 63)),
		MathUtil::convertIvec3ToRGB8(ivec3(51, 38, 63)),
		MathUtil::convertIvec3ToRGB8(ivec3(43, 53, 64)),
		MathUtil::convertIvec3ToRGB8(ivec3(61, 43, 64)),
		MathUtil::convertIvec3ToRGB8(ivec3(47, 45, 65)),
		MathUtil::convertIvec3ToRGB8(ivec3(60, 47, 65)),
		MathUtil::convertIvec3ToRGB8(ivec3(41, 50, 66)),
		MathUtil::convertIvec3ToRGB8(ivec3(58, 41, 66)),
		MathUtil::convertIvec3ToRGB8(ivec3(45, 57, 67)),
		MathUtil::convertIvec3ToRGB8(ivec3(65, 45, 67)),
		MathUtil::convertIvec3ToRGB8(ivec3(50, 44, 68)),
		MathUtil::convertIvec3ToRGB8(ivec3(44, 61, 68)),
		MathUtil::convertIvec3ToRGB8(ivec3(66, 50, 68)),
		MathUtil::convertIvec3ToRGB8(ivec3(54, 42, 69)),
		MathUtil::convertIvec3ToRGB8(ivec3(42, 59, 69)),
		MathUtil::convertIvec3ToRGB8(ivec3(46, 48, 70)),
		MathUtil::convertIvec3ToRGB8(ivec3(52, 46, 70)),
		MathUtil::convertIvec3ToRGB8(ivec3(48, 54, 71)),
		MathUtil::convertIvec3ToRGB8(ivec3(70, 48, 71)),
		MathUtil::convertIvec3ToRGB8(ivec3(54, 69, 71)),
		MathUtil::convertIvec3ToRGB8(ivec3(49, 52, 72)),
		MathUtil::convertIvec3ToRGB8(ivec3(63, 49, 72)),
		MathUtil::convertIvec3ToRGB8(ivec3(55, 62, 73)),
		MathUtil::convertIvec3ToRGB8(ivec3(52, 70, 74)),
		MathUtil::convertIvec3ToRGB8(ivec3(72, 52, 74)),
		MathUtil::convertIvec3ToRGB8(ivec3(59, 55, 75)),
		MathUtil::convertIvec3ToRGB8(ivec3(55, 73, 75)),
		MathUtil::convertIvec3ToRGB8(ivec3(0, 1, 2)), // +y voxel face indices
		MathUtil::convertIvec3ToRGB8(ivec3(1, 0, 3)),
		MathUtil::convertIvec3ToRGB8(ivec3(3, 0, 4)),
		MathUtil::convertIvec3ToRGB8(ivec3(2, 1, 5)),
		MathUtil::convertIvec3ToRGB8(ivec3(0, 2, 6)),
		MathUtil::convertIvec3ToRGB8(ivec3(4, 0, 7)),
		MathUtil::convertIvec3ToRGB8(ivec3(0, 6, 7)),
		MathUtil::convertIvec3ToRGB8(ivec3(2, 5, 8)),
		MathUtil::convertIvec3ToRGB8(ivec3(1, 3, 9)),
		MathUtil::convertIvec3ToRGB8(ivec3(5, 1, 10)),
		MathUtil::convertIvec3ToRGB8(ivec3(1, 9, 10)),
		MathUtil::convertIvec3ToRGB8(ivec3(3, 4, 11)),
		MathUtil::convertIvec3ToRGB8(ivec3(6, 2, 12)),
		MathUtil::convertIvec3ToRGB8(ivec3(2, 8, 12)),
		MathUtil::convertIvec3ToRGB8(ivec3(4, 7, 13)),
		MathUtil::convertIvec3ToRGB8(ivec3(11, 4, 14)),
		MathUtil::convertIvec3ToRGB8(ivec3(4, 13, 14)),
		MathUtil::convertIvec3ToRGB8(ivec3(8, 5, 15)),
		MathUtil::convertIvec3ToRGB8(ivec3(5, 10, 16)),
		MathUtil::convertIvec3ToRGB8(ivec3(15, 5, 16)),
		MathUtil::convertIvec3ToRGB8(ivec3(9, 3, 17)),
		MathUtil::convertIvec3ToRGB8(ivec3(3, 11, 17)),
		MathUtil::convertIvec3ToRGB8(ivec3(7, 6, 18)),
		MathUtil::convertIvec3ToRGB8(ivec3(13, 7, 19)),
		MathUtil::convertIvec3ToRGB8(ivec3(7, 18, 19)),
		MathUtil::convertIvec3ToRGB8(ivec3(6, 12, 20)),
		MathUtil::convertIvec3ToRGB8(ivec3(18, 6, 20)),
		MathUtil::convertIvec3ToRGB8(ivec3(8, 15, 21)),
		MathUtil::convertIvec3ToRGB8(ivec3(14, 13, 22)),
		MathUtil::convertIvec3ToRGB8(ivec3(12, 8, 23)),
		MathUtil::convertIvec3ToRGB8(ivec3(8, 21, 23)),
		MathUtil::convertIvec3ToRGB8(ivec3(20, 12, 23)),
		MathUtil::convertIvec3ToRGB8(ivec3(11, 14, 24)),
		MathUtil::convertIvec3ToRGB8(ivec3(17, 11, 24)),
		MathUtil::convertIvec3ToRGB8(ivec3(10, 9, 25)),
		MathUtil::convertIvec3ToRGB8(ivec3(16, 10, 26)),
		MathUtil::convertIvec3ToRGB8(ivec3(10, 25, 26)),
		MathUtil::convertIvec3ToRGB8(ivec3(13, 19, 27)),
		MathUtil::convertIvec3ToRGB8(ivec3(22, 13, 27)),
		MathUtil::convertIvec3ToRGB8(ivec3(14, 22, 28)),
		MathUtil::convertIvec3ToRGB8(ivec3(24, 14, 28)),
		MathUtil::convertIvec3ToRGB8(ivec3(15, 16, 29)),
		MathUtil::convertIvec3ToRGB8(ivec3(21, 15, 30)),
		MathUtil::convertIvec3ToRGB8(ivec3(15, 29, 30)),
		MathUtil::convertIvec3ToRGB8(ivec3(9, 17, 31)),
		MathUtil::convertIvec3ToRGB8(ivec3(25, 9, 31)),
		MathUtil::convertIvec3ToRGB8(ivec3(17, 24, 32)),
		MathUtil::convertIvec3ToRGB8(ivec3(31, 17, 32)),
		MathUtil::convertIvec3ToRGB8(ivec3(20, 23, 33)),
		MathUtil::convertIvec3ToRGB8(ivec3(19, 18, 34)),
		MathUtil::convertIvec3ToRGB8(ivec3(18, 20, 34)),
		MathUtil::convertIvec3ToRGB8(ivec3(16, 26, 35)),
		MathUtil::convertIvec3ToRGB8(ivec3(29, 16, 35)),
		MathUtil::convertIvec3ToRGB8(ivec3(20, 33, 36)),
		MathUtil::convertIvec3ToRGB8(ivec3(34, 20, 36)),
		MathUtil::convertIvec3ToRGB8(ivec3(28, 22, 37)),
		MathUtil::convertIvec3ToRGB8(ivec3(27, 19, 38)),
		MathUtil::convertIvec3ToRGB8(ivec3(19, 34, 38)),
		MathUtil::convertIvec3ToRGB8(ivec3(22, 27, 39)),
		MathUtil::convertIvec3ToRGB8(ivec3(37, 22, 39)),
		MathUtil::convertIvec3ToRGB8(ivec3(23, 21, 40)),
		MathUtil::convertIvec3ToRGB8(ivec3(21, 30, 40)),
		MathUtil::convertIvec3ToRGB8(ivec3(30, 29, 41)),
		MathUtil::convertIvec3ToRGB8(ivec3(33, 23, 42)),
		MathUtil::convertIvec3ToRGB8(ivec3(23, 40, 42)),
		MathUtil::convertIvec3ToRGB8(ivec3(31, 32, 43)),
		MathUtil::convertIvec3ToRGB8(ivec3(24, 28, 44)),
		MathUtil::convertIvec3ToRGB8(ivec3(32, 24, 44)),
		MathUtil::convertIvec3ToRGB8(ivec3(26, 25, 45)),
		MathUtil::convertIvec3ToRGB8(ivec3(25, 31, 45)),
		MathUtil::convertIvec3ToRGB8(ivec3(28, 37, 46)),
		MathUtil::convertIvec3ToRGB8(ivec3(44, 28, 46)),
		MathUtil::convertIvec3ToRGB8(ivec3(40, 30, 47)),
		MathUtil::convertIvec3ToRGB8(ivec3(30, 41, 47)),
		MathUtil::convertIvec3ToRGB8(ivec3(29, 35, 48)),
		MathUtil::convertIvec3ToRGB8(ivec3(41, 29, 48)),
		MathUtil::convertIvec3ToRGB8(ivec3(27, 38, 49)),
		MathUtil::convertIvec3ToRGB8(ivec3(39, 27, 49)),
		MathUtil::convertIvec3ToRGB8(ivec3(35, 26, 50)),
		MathUtil::convertIvec3ToRGB8(ivec3(26, 45, 50)),
		MathUtil::convertIvec3ToRGB8(ivec3(31, 43, 51)),
		MathUtil::convertIvec3ToRGB8(ivec3(45, 31, 51)),
		MathUtil::convertIvec3ToRGB8(ivec3(36, 33, 52)),
		MathUtil::convertIvec3ToRGB8(ivec3(43, 32, 53)),
		MathUtil::convertIvec3ToRGB8(ivec3(32, 44, 53)),
		MathUtil::convertIvec3ToRGB8(ivec3(33, 42, 54)),
		MathUtil::convertIvec3ToRGB8(ivec3(52, 33, 54)),
		MathUtil::convertIvec3ToRGB8(ivec3(37, 39, 55)),
		MathUtil::convertIvec3ToRGB8(ivec3(46, 37, 56)),
		MathUtil::convertIvec3ToRGB8(ivec3(37, 55, 56)),
		MathUtil::convertIvec3ToRGB8(ivec3(34, 36, 57)),
		MathUtil::convertIvec3ToRGB8(ivec3(38, 34, 57)),
		MathUtil::convertIvec3ToRGB8(ivec3(48, 35, 58)),
		MathUtil::convertIvec3ToRGB8(ivec3(35, 50, 58)),
		MathUtil::convertIvec3ToRGB8(ivec3(36, 52, 59)),
		MathUtil::convertIvec3ToRGB8(ivec3(57, 36, 59)),
		MathUtil::convertIvec3ToRGB8(ivec3(47, 41, 60)),
		MathUtil::convertIvec3ToRGB8(ivec3(39, 49, 61)),
		MathUtil::convertIvec3ToRGB8(ivec3(55, 39, 61)),
		MathUtil::convertIvec3ToRGB8(ivec3(49, 38, 62)),
		MathUtil::convertIvec3ToRGB8(ivec3(38, 57, 62)),
		MathUtil::convertIvec3ToRGB8(ivec3(41, 48, 63)),
		MathUtil::convertIvec3ToRGB8(ivec3(60, 41, 63)),
		MathUtil::convertIvec3ToRGB8(ivec3(51, 43, 64)),
		MathUtil::convertIvec3ToRGB8(ivec3(43, 53, 64)),
		MathUtil::convertIvec3ToRGB8(ivec3(42, 40, 65)),
		MathUtil::convertIvec3ToRGB8(ivec3(54, 42, 65)),
		MathUtil::convertIvec3ToRGB8(ivec3(40, 47, 65)),
		MathUtil::convertIvec3ToRGB8(ivec3(47, 60, 66)),
		MathUtil::convertIvec3ToRGB8(ivec3(65, 47, 66)),
		MathUtil::convertIvec3ToRGB8(ivec3(50, 45, 67)),
		MathUtil::convertIvec3ToRGB8(ivec3(45, 51, 67)),
		MathUtil::convertIvec3ToRGB8(ivec3(64, 53, 68)),
		MathUtil::convertIvec3ToRGB8(ivec3(44, 46, 69)),
		MathUtil::convertIvec3ToRGB8(ivec3(46, 56, 69)),
		MathUtil::convertIvec3ToRGB8(ivec3(54, 65, 70)),
		MathUtil::convertIvec3ToRGB8(ivec3(53, 44, 71)),
		MathUtil::convertIvec3ToRGB8(ivec3(44, 69, 71)),
		MathUtil::convertIvec3ToRGB8(ivec3(68, 53, 71)),
		MathUtil::convertIvec3ToRGB8(ivec3(51, 64, 72)),
		MathUtil::convertIvec3ToRGB8(ivec3(67, 51, 72)),
		MathUtil::convertIvec3ToRGB8(ivec3(48, 58, 73)),
		MathUtil::convertIvec3ToRGB8(ivec3(63, 48, 73)),
		MathUtil::convertIvec3ToRGB8(ivec3(52, 54, 74)),
		MathUtil::convertIvec3ToRGB8(ivec3(59, 52, 74)),
		MathUtil::convertIvec3ToRGB8(ivec3(58, 50, 75)),
		MathUtil::convertIvec3ToRGB8(ivec3(50, 67, 75)),
		MathUtil::convertIvec3ToRGB8(ivec3(73, 58, 75)),
		MathUtil::convertIvec3ToRGB8(ivec3(1, 0, 2)), // -z voxel face indices
		MathUtil::convertIvec3ToRGB8(ivec3(0, 1, 3)),
		MathUtil::convertIvec3ToRGB8(ivec3(1, 2, 4)),
		MathUtil::convertIvec3ToRGB8(ivec3(2, 0, 5)),
		MathUtil::convertIvec3ToRGB8(ivec3(3, 1, 6)),
		MathUtil::convertIvec3ToRGB8(ivec3(4, 2, 7)),
		MathUtil::convertIvec3ToRGB8(ivec3(0, 3, 8)),
		MathUtil::convertIvec3ToRGB8(ivec3(5, 0, 8)),
		MathUtil::convertIvec3ToRGB8(ivec3(1, 4, 9)),
		MathUtil::convertIvec3ToRGB8(ivec3(6, 1, 9)),
		MathUtil::convertIvec3ToRGB8(ivec3(2, 5, 10)),
		MathUtil::convertIvec3ToRGB8(ivec3(3, 6, 11)),
		MathUtil::convertIvec3ToRGB8(ivec3(4, 7, 12)),
		MathUtil::convertIvec3ToRGB8(ivec3(5, 8, 13)),
		MathUtil::convertIvec3ToRGB8(ivec3(6, 9, 14)),
		MathUtil::convertIvec3ToRGB8(ivec3(7, 2, 15)),
		MathUtil::convertIvec3ToRGB8(ivec3(2, 10, 15)),
		MathUtil::convertIvec3ToRGB8(ivec3(8, 3, 16)),
		MathUtil::convertIvec3ToRGB8(ivec3(3, 11, 16)),
		MathUtil::convertIvec3ToRGB8(ivec3(9, 4, 17)),
		MathUtil::convertIvec3ToRGB8(ivec3(4, 12, 17)),
		MathUtil::convertIvec3ToRGB8(ivec3(10, 5, 18)),
		MathUtil::convertIvec3ToRGB8(ivec3(5, 13, 18)),
		MathUtil::convertIvec3ToRGB8(ivec3(11, 6, 19)),
		MathUtil::convertIvec3ToRGB8(ivec3(6, 14, 19)),
		MathUtil::convertIvec3ToRGB8(ivec3(12, 7, 20)),
		MathUtil::convertIvec3ToRGB8(ivec3(7, 15, 20)),
		MathUtil::convertIvec3ToRGB8(ivec3(13, 8, 21)),
		MathUtil::convertIvec3ToRGB8(ivec3(8, 16, 21)),
		MathUtil::convertIvec3ToRGB8(ivec3(14, 9, 22)),
		MathUtil::convertIvec3ToRGB8(ivec3(9, 17, 22)),
		MathUtil::convertIvec3ToRGB8(ivec3(15, 10, 23)),
		MathUtil::convertIvec3ToRGB8(ivec3(10, 18, 23)),
		MathUtil::convertIvec3ToRGB8(ivec3(16, 11, 24)),
		MathUtil::convertIvec3ToRGB8(ivec3(11, 19, 24)),
		MathUtil::convertIvec3ToRGB8(ivec3(17, 12, 25)),
		MathUtil::convertIvec3ToRGB8(ivec3(12, 20, 25)),
		MathUtil::convertIvec3ToRGB8(ivec3(18, 13, 26)),
		MathUtil::convertIvec3ToRGB8(ivec3(13, 21, 26)),
		MathUtil::convertIvec3ToRGB8(ivec3(19, 14, 27)),
		MathUtil::convertIvec3ToRGB8(ivec3(14, 22, 27)),
		MathUtil::convertIvec3ToRGB8(ivec3(20, 15, 28)),
		MathUtil::convertIvec3ToRGB8(ivec3(21, 16, 29)),
		MathUtil::convertIvec3ToRGB8(ivec3(22, 17, 30)),
		MathUtil::convertIvec3ToRGB8(ivec3(23, 18, 31)),
		MathUtil::convertIvec3ToRGB8(ivec3(24, 19, 32)),
		MathUtil::convertIvec3ToRGB8(ivec3(25, 20, 33)),
		MathUtil::convertIvec3ToRGB8(ivec3(26, 21, 34)),
		MathUtil::convertIvec3ToRGB8(ivec3(27, 22, 35)),
		MathUtil::convertIvec3ToRGB8(ivec3(15, 23, 36)),
		MathUtil::convertIvec3ToRGB8(ivec3(28, 15, 36)),
		MathUtil::convertIvec3ToRGB8(ivec3(16, 24, 37)),
		MathUtil::convertIvec3ToRGB8(ivec3(29, 16, 37)),
		MathUtil::convertIvec3ToRGB8(ivec3(17, 25, 38)),
		MathUtil::convertIvec3ToRGB8(ivec3(30, 17, 38)),
		MathUtil::convertIvec3ToRGB8(ivec3(18, 26, 39)),
		MathUtil::convertIvec3ToRGB8(ivec3(31, 18, 39)),
		MathUtil::convertIvec3ToRGB8(ivec3(19, 27, 40)),
		MathUtil::convertIvec3ToRGB8(ivec3(32, 19, 40)),
		MathUtil::convertIvec3ToRGB8(ivec3(20, 28, 41)),
		MathUtil::convertIvec3ToRGB8(ivec3(33, 20, 41)),
		MathUtil::convertIvec3ToRGB8(ivec3(21, 29, 42)),
		MathUtil::convertIvec3ToRGB8(ivec3(34, 21, 42)),
		MathUtil::convertIvec3ToRGB8(ivec3(22, 30, 43)),
		MathUtil::convertIvec3ToRGB8(ivec3(35, 22, 43)),
		MathUtil::convertIvec3ToRGB8(ivec3(23, 31, 44)),
		MathUtil::convertIvec3ToRGB8(ivec3(36, 23, 44)),
		MathUtil::convertIvec3ToRGB8(ivec3(24, 32, 45)),
		MathUtil::convertIvec3ToRGB8(ivec3(37, 24, 45)),
		MathUtil::convertIvec3ToRGB8(ivec3(25, 33, 46)),
		MathUtil::convertIvec3ToRGB8(ivec3(38, 25, 46)),
		MathUtil::convertIvec3ToRGB8(ivec3(26, 34, 47)),
		MathUtil::convertIvec3ToRGB8(ivec3(39, 26, 47)),
		MathUtil::convertIvec3ToRGB8(ivec3(27, 35, 48)),
		MathUtil::convertIvec3ToRGB8(ivec3(40, 27, 48)),
		MathUtil::convertIvec3ToRGB8(ivec3(28, 36, 49)),
		MathUtil::convertIvec3ToRGB8(ivec3(41, 28, 49)),
		MathUtil::convertIvec3ToRGB8(ivec3(29, 37, 50)),
		MathUtil::convertIvec3ToRGB8(ivec3(42, 29, 50)),
		MathUtil::convertIvec3ToRGB8(ivec3(30, 38, 51)),
		MathUtil::convertIvec3ToRGB8(ivec3(43, 30, 51)),
		MathUtil::convertIvec3ToRGB8(ivec3(31, 39, 52)),
		MathUtil::convertIvec3ToRGB8(ivec3(44, 31, 52)),
		MathUtil::convertIvec3ToRGB8(ivec3(32, 40, 53)),
		MathUtil::convertIvec3ToRGB8(ivec3(45, 32, 53)),
		MathUtil::convertIvec3ToRGB8(ivec3(33, 41, 54)),
		MathUtil::convertIvec3ToRGB8(ivec3(46, 33, 54)),
		MathUtil::convertIvec3ToRGB8(ivec3(34, 42, 55)),
		MathUtil::convertIvec3ToRGB8(ivec3(47, 34, 55)),
		MathUtil::convertIvec3ToRGB8(ivec3(35, 43, 56)),
		MathUtil::convertIvec3ToRGB8(ivec3(48, 35, 56)),
		MathUtil::convertIvec3ToRGB8(ivec3(36, 44, 57)),
		MathUtil::convertIvec3ToRGB8(ivec3(49, 36, 57)),
		MathUtil::convertIvec3ToRGB8(ivec3(37, 45, 58)),
		MathUtil::convertIvec3ToRGB8(ivec3(50, 37, 58)),
		MathUtil::convertIvec3ToRGB8(ivec3(38, 46, 59)),
		MathUtil::convertIvec3ToRGB8(ivec3(51, 38, 59)),
		MathUtil::convertIvec3ToRGB8(ivec3(39, 47, 60)),
		MathUtil::convertIvec3ToRGB8(ivec3(52, 39, 60)),
		MathUtil::convertIvec3ToRGB8(ivec3(40, 48, 61)),
		MathUtil::convertIvec3ToRGB8(ivec3(53, 40, 61)),
		MathUtil::convertIvec3ToRGB8(ivec3(41, 49, 62)),
		MathUtil::convertIvec3ToRGB8(ivec3(54, 41, 62)),
		MathUtil::convertIvec3ToRGB8(ivec3(42, 50, 63)),
		MathUtil::convertIvec3ToRGB8(ivec3(55, 42, 63)),
		MathUtil::convertIvec3ToRGB8(ivec3(43, 51, 64)),
		MathUtil::convertIvec3ToRGB8(ivec3(56, 43, 64)),
		MathUtil::convertIvec3ToRGB8(ivec3(44, 52, 65)),
		MathUtil::convertIvec3ToRGB8(ivec3(57, 44, 65)),
		MathUtil::convertIvec3ToRGB8(ivec3(45, 53, 66)),
		MathUtil::convertIvec3ToRGB8(ivec3(58, 45, 66)),
		MathUtil::convertIvec3ToRGB8(ivec3(46, 54, 67)),
		MathUtil::convertIvec3ToRGB8(ivec3(59, 46, 67)),
		MathUtil::convertIvec3ToRGB8(ivec3(47, 55, 68)),
		MathUtil::convertIvec3ToRGB8(ivec3(60, 47, 68)),
		MathUtil::convertIvec3ToRGB8(ivec3(48, 56, 69)),
		MathUtil::convertIvec3ToRGB8(ivec3(61, 48, 69)),
		MathUtil::convertIvec3ToRGB8(ivec3(49, 57, 70)),
		MathUtil::convertIvec3ToRGB8(ivec3(62, 49, 70)),
		MathUtil::convertIvec3ToRGB8(ivec3(50, 58, 71)),
		MathUtil::convertIvec3ToRGB8(ivec3(63, 50, 71)),
		MathUtil::convertIvec3ToRGB8(ivec3(51, 59, 72)),
		MathUtil::convertIvec3ToRGB8(ivec3(64, 51, 72)),
		MathUtil::convertIvec3ToRGB8(ivec3(52, 60, 73)),
		MathUtil::convertIvec3ToRGB8(ivec3(65, 52, 73)),
		MathUtil::convertIvec3ToRGB8(ivec3(53, 61, 74)),
		MathUtil::convertIvec3ToRGB8(ivec3(66, 53, 74)),
		MathUtil::convertIvec3ToRGB8(ivec3(54, 62, 75)),
		MathUtil::convertIvec3ToRGB8(ivec3(0, 1, 2)), //+z voxel face indices
		MathUtil::convertIvec3ToRGB8(ivec3(1, 0, 3)),
		MathUtil::convertIvec3ToRGB8(ivec3(2, 1, 4)),
		MathUtil::convertIvec3ToRGB8(ivec3(0, 2, 5)),
		MathUtil::convertIvec3ToRGB8(ivec3(1, 3, 6)),
		MathUtil::convertIvec3ToRGB8(ivec3(2, 4, 7)),
		MathUtil::convertIvec3ToRGB8(ivec3(3, 0, 8)),
		MathUtil::convertIvec3ToRGB8(ivec3(0, 5, 8)),
		MathUtil::convertIvec3ToRGB8(ivec3(4, 1, 9)),
		MathUtil::convertIvec3ToRGB8(ivec3(1, 6, 9)),
		MathUtil::convertIvec3ToRGB8(ivec3(5, 2, 10)),
		MathUtil::convertIvec3ToRGB8(ivec3(6, 3, 11)),
		MathUtil::convertIvec3ToRGB8(ivec3(7, 4, 12)),
		MathUtil::convertIvec3ToRGB8(ivec3(8, 5, 13)),
		MathUtil::convertIvec3ToRGB8(ivec3(9, 6, 14)),
		MathUtil::convertIvec3ToRGB8(ivec3(2, 7, 15)),
		MathUtil::convertIvec3ToRGB8(ivec3(10, 2, 15)),
		MathUtil::convertIvec3ToRGB8(ivec3(3, 8, 16)),
		MathUtil::convertIvec3ToRGB8(ivec3(11, 3, 16)),
		MathUtil::convertIvec3ToRGB8(ivec3(4, 9, 17)),
		MathUtil::convertIvec3ToRGB8(ivec3(12, 4, 17)),
		MathUtil::convertIvec3ToRGB8(ivec3(5, 10, 18)),
		MathUtil::convertIvec3ToRGB8(ivec3(13, 5, 18)),
		MathUtil::convertIvec3ToRGB8(ivec3(6, 11, 19)),
		MathUtil::convertIvec3ToRGB8(ivec3(14, 6, 19)),
		MathUtil::convertIvec3ToRGB8(ivec3(7, 12, 20)),
		MathUtil::convertIvec3ToRGB8(ivec3(15, 7, 20)),
		MathUtil::convertIvec3ToRGB8(ivec3(8, 13, 21)),
		MathUtil::convertIvec3ToRGB8(ivec3(16, 8, 21)),
		MathUtil::convertIvec3ToRGB8(ivec3(9, 14, 22)),
		MathUtil::convertIvec3ToRGB8(ivec3(17, 9, 22)),
		MathUtil::convertIvec3ToRGB8(ivec3(10, 15, 23)),
		MathUtil::convertIvec3ToRGB8(ivec3(18, 10, 23)),
		MathUtil::convertIvec3ToRGB8(ivec3(11, 16, 24)),
		MathUtil::convertIvec3ToRGB8(ivec3(19, 11, 24)),
		MathUtil::convertIvec3ToRGB8(ivec3(12, 17, 25)),
		MathUtil::convertIvec3ToRGB8(ivec3(20, 12, 25)),
		MathUtil::convertIvec3ToRGB8(ivec3(13, 18, 26)),
		MathUtil::convertIvec3ToRGB8(ivec3(21, 13, 26)),
		MathUtil::convertIvec3ToRGB8(ivec3(14, 19, 27)),
		MathUtil::convertIvec3ToRGB8(ivec3(22, 14, 27)),
		MathUtil::convertIvec3ToRGB8(ivec3(15, 20, 28)),
		MathUtil::convertIvec3ToRGB8(ivec3(16, 21, 29)),
		MathUtil::convertIvec3ToRGB8(ivec3(17, 22, 30)),
		MathUtil::convertIvec3ToRGB8(ivec3(18, 23, 31)),
		MathUtil::convertIvec3ToRGB8(ivec3(19, 24, 32)),
		MathUtil::convertIvec3ToRGB8(ivec3(20, 25, 33)),
		MathUtil::convertIvec3ToRGB8(ivec3(21, 26, 34)),
		MathUtil::convertIvec3ToRGB8(ivec3(22, 27, 35)),
		MathUtil::convertIvec3ToRGB8(ivec3(23, 15, 36)),
		MathUtil::convertIvec3ToRGB8(ivec3(15, 28, 36)),
		MathUtil::convertIvec3ToRGB8(ivec3(24, 16, 37)),
		MathUtil::convertIvec3ToRGB8(ivec3(16, 29, 37)),
		MathUtil::convertIvec3ToRGB8(ivec3(25, 17, 38)),
		MathUtil::convertIvec3ToRGB8(ivec3(17, 30, 38)),
		MathUtil::convertIvec3ToRGB8(ivec3(26, 18, 39)),
		MathUtil::convertIvec3ToRGB8(ivec3(18, 31, 39)),
		MathUtil::convertIvec3ToRGB8(ivec3(27, 19, 40)),
		MathUtil::convertIvec3ToRGB8(ivec3(19, 32, 40)),
		MathUtil::convertIvec3ToRGB8(ivec3(28, 20, 41)),
		MathUtil::convertIvec3ToRGB8(ivec3(20, 33, 41)),
		MathUtil::convertIvec3ToRGB8(ivec3(29, 21, 42)),
		MathUtil::convertIvec3ToRGB8(ivec3(21, 34, 42)),
		MathUtil::convertIvec3ToRGB8(ivec3(30, 22, 43)),
		MathUtil::convertIvec3ToRGB8(ivec3(22, 35, 43)),
		MathUtil::convertIvec3ToRGB8(ivec3(31, 23, 44)),
		MathUtil::convertIvec3ToRGB8(ivec3(23, 36, 44)),
		MathUtil::convertIvec3ToRGB8(ivec3(32, 24, 45)),
		MathUtil::convertIvec3ToRGB8(ivec3(24, 37, 45)),
		MathUtil::convertIvec3ToRGB8(ivec3(33, 25, 46)),
		MathUtil::convertIvec3ToRGB8(ivec3(25, 38, 46)),
		MathUtil::convertIvec3ToRGB8(ivec3(34, 26, 47)),
		MathUtil::convertIvec3ToRGB8(ivec3(26, 39, 47)),
		MathUtil::convertIvec3ToRGB8(ivec3(35, 27, 48)),
		MathUtil::convertIvec3ToRGB8(ivec3(27, 40, 48)),
		MathUtil::convertIvec3ToRGB8(ivec3(36, 28, 49)),
		MathUtil::convertIvec3ToRGB8(ivec3(28, 41, 49)),
		MathUtil::convertIvec3ToRGB8(ivec3(37, 29, 50)),
		MathUtil::convertIvec3ToRGB8(ivec3(29, 42, 50)),
		MathUtil::convertIvec3ToRGB8(ivec3(38, 30, 51)),
		MathUtil::convertIvec3ToRGB8(ivec3(30, 43, 51)),
		MathUtil::convertIvec3ToRGB8(ivec3(39, 31, 52)),
		MathUtil::convertIvec3ToRGB8(ivec3(31, 44, 52)),
		MathUtil::convertIvec3ToRGB8(ivec3(40, 32, 53)),
		MathUtil::convertIvec3ToRGB8(ivec3(32, 45, 53)),
		MathUtil::convertIvec3ToRGB8(ivec3(41, 33, 54)),
		MathUtil::convertIvec3ToRGB8(ivec3(33, 46, 54)),
		MathUtil::convertIvec3ToRGB8(ivec3(42, 34, 55)),
		MathUtil::convertIvec3ToRGB8(ivec3(34, 47, 55)),
		MathUtil::convertIvec3ToRGB8(ivec3(43, 35, 56)),
		MathUtil::convertIvec3ToRGB8(ivec3(35, 48, 56)),
		MathUtil::convertIvec3ToRGB8(ivec3(44, 36, 57)),
		MathUtil::convertIvec3ToRGB8(ivec3(36, 49, 57)),
		MathUtil::convertIvec3ToRGB8(ivec3(45, 37, 58)),
		MathUtil::convertIvec3ToRGB8(ivec3(37, 50, 58)),
		MathUtil::convertIvec3ToRGB8(ivec3(46, 38, 59)),
		MathUtil::convertIvec3ToRGB8(ivec3(38, 51, 59)),
		MathUtil::convertIvec3ToRGB8(ivec3(47, 39, 60)),
		MathUtil::convertIvec3ToRGB8(ivec3(39, 52, 60)),
		MathUtil::convertIvec3ToRGB8(ivec3(48, 40, 61)),
		MathUtil::convertIvec3ToRGB8(ivec3(40, 53, 61)),
		MathUtil::convertIvec3ToRGB8(ivec3(49, 41, 62)),
		MathUtil::convertIvec3ToRGB8(ivec3(41, 54, 62)),
		MathUtil::convertIvec3ToRGB8(ivec3(50, 42, 63)),
		MathUtil::convertIvec3ToRGB8(ivec3(42, 55, 63)),
		MathUtil::convertIvec3ToRGB8(ivec3(51, 43, 64)),
		MathUtil::convertIvec3ToRGB8(ivec3(43, 56, 64)),
		MathUtil::convertIvec3ToRGB8(ivec3(52, 44, 65)),
		MathUtil::convertIvec3ToRGB8(ivec3(44, 57, 65)),
		MathUtil::convertIvec3ToRGB8(ivec3(53, 45, 66)),
		MathUtil::convertIvec3ToRGB8(ivec3(45, 58, 66)),
		MathUtil::convertIvec3ToRGB8(ivec3(54, 46, 67)),
		MathUtil::convertIvec3ToRGB8(ivec3(46, 59, 67)),
		MathUtil::convertIvec3ToRGB8(ivec3(55, 47, 68)),
		MathUtil::convertIvec3ToRGB8(ivec3(47, 60, 68)),
		MathUtil::convertIvec3ToRGB8(ivec3(56, 48, 69)),
		MathUtil::convertIvec3ToRGB8(ivec3(48, 61, 69)),
		MathUtil::convertIvec3ToRGB8(ivec3(57, 49, 70)),
		MathUtil::convertIvec3ToRGB8(ivec3(49, 62, 70)),
		MathUtil::convertIvec3ToRGB8(ivec3(58, 50, 71)),
		MathUtil::convertIvec3ToRGB8(ivec3(50, 63, 71)),
		MathUtil::convertIvec3ToRGB8(ivec3(59, 51, 72)),
		MathUtil::convertIvec3ToRGB8(ivec3(51, 64, 72)),
		MathUtil::convertIvec3ToRGB8(ivec3(60, 52, 73)),
		MathUtil::convertIvec3ToRGB8(ivec3(52, 65, 73)),
		MathUtil::convertIvec3ToRGB8(ivec3(61, 53, 74)),
		MathUtil::convertIvec3ToRGB8(ivec3(53, 66, 74)),
		MathUtil::convertIvec3ToRGB8(ivec3(62, 54, 75)),
	};

	m_triangulatedIndicesBuffer = bufferM->buildBuffer(
		move(string("triangulatedIndicesBuffer")),
		vectorData.data(),
		vectorData.size() * sizeof(uint),
		VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
}

/////////////////////////////////////////////////////////////////////////////////////////////
