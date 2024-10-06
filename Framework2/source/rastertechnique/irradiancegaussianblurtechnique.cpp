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
#include "../../include/rastertechnique/irradiancegaussianblurtechnique.h"
#include "../../include/core/coremanager.h"
#include "../../include/parameter/attributedefines.h"
#include "../../include/parameter/attributedata.h"
#include "../../include/rastertechnique/staticscenevoxelizationtechnique.h"
#include "../../include/rastertechnique/bufferprefixsumtechnique.h"
#include "../../include/util/vulkanstructinitializer.h"
#include "../../include/rastertechnique/processcameravisibleresultstechnique.h"
#include "../../include/rastertechnique/lightbouncevoxelirradiancetechnique.h"
#include "../../include/rastertechnique/dynamiclightbouncevoxelirradiancetechnique.h"
#include "../../include/material/materiallightbouncedynamiccopyirradiance.h"
#include "../../include/material/materiallightbouncevoxel2dgaussianfilter.h"
#include "../../include/rastertechnique/resetlitvoxeldatatechnique.h"

// NAMESPACE
using namespace attributedefines;

// DEFINES

// STATIC MEMBER INITIALIZATION

/////////////////////////////////////////////////////////////////////////////////////////////

IrradianceGaussianBlurTechnique::IrradianceGaussianBlurTechnique(string &&name, string&& className) : BufferProcessTechnique(move(name), move(className))
	, m_techniquePrefixSum(nullptr)
	, m_prefixSumCompleted(false)
	, m_cameraVisibleVoxelNumber(0)
	, m_processCameraVisibleResultsTechnique(nullptr)
	, m_dynamicLightBounceVoxelIrradianceTechnique(nullptr)
	, m_cameraVisibleDynamicVoxelIrradianceBuffer(nullptr)
	, m_materialLightBounceDynamicCopyIrradiance(nullptr)
	, m_materialLightBounceVoxel2DFilter(nullptr)
	, m_voxel2dFilterDebugBuffer(nullptr)
	, m_dynamicCopyIrradianceDebugBuffer(nullptr)
	, m_resetLitvoxelData(nullptr)
	, m_staticVoxelPaddingDebugBuffer(nullptr)
{
	m_numElementPerLocalWorkgroupThread = 1;
	m_numThreadPerLocalWorkgroup        = 64;
	m_active                            = false;
	m_needsToRecord                     = false;
	m_executeCommand                    = false;
	m_rasterTechniqueType               = RasterTechniqueType::RTT_COMPUTE;
	m_computeHostSynchronize            = true;
}

/////////////////////////////////////////////////////////////////////////////////////////////

void IrradianceGaussianBlurTechnique::init()
{
	m_voxel2dFilterDebugBuffer = bufferM->buildBuffer(
		move(string("voxel2dFilterDebugBuffer")),
		nullptr,
		10000,
		VK_BUFFER_USAGE_STORAGE_BUFFER_BIT  | VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
	
	m_dynamicCopyIrradianceDebugBuffer = bufferM->buildBuffer(
		move(string("dynamicCopyIrradianceDebugBuffer")),
		nullptr,
		10000,
		VK_BUFFER_USAGE_STORAGE_BUFFER_BIT  | VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

	m_staticVoxelPaddingDebugBuffer = bufferM->buildBuffer(
		move(string("staticVoxelPaddingDebugBuffer")),
		nullptr,
		10000,
		VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

	m_processCameraVisibleResultsTechnique = static_cast<ProcessCameraVisibleResultsTechnique*>(gpuPipelineM->getRasterTechniqueByName(move(string("ProcessCameraVisibleResultsTechnique"))));

	StaticSceneVoxelizationTechnique* sceneVoxelizationTechnique = static_cast<StaticSceneVoxelizationTechnique*>(gpuPipelineM->getRasterTechniqueByName(move(string("StaticSceneVoxelizationTechnique"))));
	
	buildShaderThreadMapping();

	m_materialLightBounceDynamicCopyIrradiance = static_cast<MaterialLightBounceDynamicCopyIrradiance*>(materialM->buildMaterial(move(string("MaterialLightBounceDynamicCopyIrradiance")), move(string("MaterialLightBounceDynamicCopyIrradiance")), nullptr));
	m_vectorMaterialName.push_back("MaterialLightBounceDynamicCopyIrradiance");
	m_vectorMaterial.push_back(m_materialLightBounceDynamicCopyIrradiance);
	m_materialLightBounceDynamicCopyIrradiance->setVoxelizationSize(sceneVoxelizationTechnique->getVoxelizedSceneWidth());

	m_materialLightBounceVoxel2DFilter = static_cast<MaterialLightBounceVoxel2DFilter*>(materialM->buildMaterial(move(string("MaterialLightBounceVoxel2DFilter")), move(string("MaterialLightBounceVoxel2DFilter")), nullptr));
	m_vectorMaterialName.push_back("MaterialLightBounceVoxel2DFilter");
	m_vectorMaterial.push_back(m_materialLightBounceVoxel2DFilter);
	m_materialLightBounceVoxel2DFilter->setVoxelizationSize(sceneVoxelizationTechnique->getVoxelizedSceneWidth());

	m_dynamicLightBounceVoxelIrradianceTechnique = static_cast<DynamicLightBounceVoxelIrradianceTechnique*>(gpuPipelineM->getRasterTechniqueByName(move(string("DynamicLightBounceVoxelIrradianceTechnique"))));
	m_dynamicLightBounceVoxelIrradianceTechnique->refSignalDynamicLightBounceVoxelIrradianceCompletion().connect<IrradianceGaussianBlurTechnique, &IrradianceGaussianBlurTechnique::slotDynamicLightBounceVoxelIrradianceCompleted>(this);

	m_techniquePrefixSum = static_cast<BufferPrefixSumTechnique*>(gpuPipelineM->getRasterTechniqueByName(move(string("BufferPrefixSumTechnique"))));
	m_techniquePrefixSum->refPrefixSumComplete().connect<IrradianceGaussianBlurTechnique, &IrradianceGaussianBlurTechnique::slotPrefixSumComplete>(this);

	vectorString vectorTextureName =
	{
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

	m_cameraVisibleDynamicVoxelIrradianceBuffer = bufferM->getElement(move(string("cameraVisibleDynamicVoxelIrradianceBuffer")));

	m_resetLitvoxelData = static_cast<ResetLitvoxelData*>(gpuPipelineM->getRasterTechniqueByName(move(string("ResetLitvoxelData"))));
}

/////////////////////////////////////////////////////////////////////////////////////////////

VkCommandBuffer* IrradianceGaussianBlurTechnique::record(int currentImage, uint& commandBufferID, CommandBufferType& commandBufferType)
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

	VkImageSubresourceRange subresourceRange = {};
	subresourceRange.aspectMask   = VK_IMAGE_ASPECT_COLOR_BIT;
	subresourceRange.baseMipLevel = 0;
	subresourceRange.levelCount   = 1;
	subresourceRange.layerCount   = 1;

	VkImageMemoryBarrier imageMemoryBarrier{};
	imageMemoryBarrier.sType               = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	imageMemoryBarrier.srcQueueFamilyIndex = coreM->getComputeQueueIndex();
	imageMemoryBarrier.dstQueueFamilyIndex = coreM->getComputeQueueIndex();
	imageMemoryBarrier.srcAccessMask       = VK_ACCESS_SHADER_WRITE_BIT;
	imageMemoryBarrier.dstAccessMask       = VK_ACCESS_SHADER_READ_BIT;
	imageMemoryBarrier.oldLayout           = VK_IMAGE_LAYOUT_GENERAL;
	imageMemoryBarrier.newLayout           = VK_IMAGE_LAYOUT_GENERAL;
	imageMemoryBarrier.subresourceRange    = VkImageSubresourceRange{ VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};

	forI(m_vectorTexture.size())
	{
		imageMemoryBarrier.image = m_vectorTexture[i]->getImage();
		vkCmdPipelineBarrier(*commandBuffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 1, &imageMemoryBarrier);
	}

	uint dynamicAllignment = materialM->getMaterialUBDynamicAllignment();

	vkCmdBindPipeline(*commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, m_materialLightBounceVoxel2DFilter->getPipeline()->getPipeline());
	uint32_t offsetData = static_cast<uint32_t>(m_materialLightBounceVoxel2DFilter->getMaterialUniformBufferIndex() * dynamicAllignment);
	vkCmdBindDescriptorSets(*commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, m_materialLightBounceVoxel2DFilter->getPipelineLayout(), 0, 1, &m_materialLightBounceVoxel2DFilter->refDescriptorSet(), 1, &offsetData);
	vkCmdDispatch(*commandBuffer, m_materialLightBounceVoxel2DFilter->getLocalWorkGroupsXDimension(), m_materialLightBounceVoxel2DFilter->getLocalWorkGroupsYDimension(), 1); // Compute shader global workgroup https://www.khronos.org/registry/vulkan/specs/1.1-extensions/html/vkspec.html

	VulkanStructInitializer::insertBufferMemoryBarrier(m_cameraVisibleDynamicVoxelIrradianceBuffer,
													   VK_ACCESS_SHADER_WRITE_BIT,
													   VK_ACCESS_SHADER_READ_BIT,
													   VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
													   VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
													   commandBuffer);

	vkCmdBindPipeline(*commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, m_materialLightBounceDynamicCopyIrradiance->getPipeline()->getPipeline());
	offsetData = static_cast<uint32_t>(m_materialLightBounceDynamicCopyIrradiance->getMaterialUniformBufferIndex() * dynamicAllignment);
	vkCmdBindDescriptorSets(*commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, m_materialLightBounceDynamicCopyIrradiance->getPipelineLayout(), 0, 1, &m_materialLightBounceDynamicCopyIrradiance->refDescriptorSet(), 1, &offsetData);
	vkCmdDispatch(*commandBuffer, m_materialLightBounceDynamicCopyIrradiance->getLocalWorkGroupsXDimension(), m_materialLightBounceDynamicCopyIrradiance->getLocalWorkGroupsYDimension(), 1); // Compute shader global workgroup https://www.khronos.org/registry/vulkan/specs/1.1-extensions/html/vkspec.html

#ifdef USE_TIMESTAMP
	vkCmdWriteTimestamp(*commandBuffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, coreM->getComputeQueueQueryPool(), m_queryIndex1);
#endif

	coreM->endCommandBuffer(*commandBuffer);

	// NOTE: Clear command buffer if re-recorded
	m_vectorCommand.push_back(commandBuffer);

	return commandBuffer;
}

/////////////////////////////////////////////////////////////////////////////////////////////

void IrradianceGaussianBlurTechnique::postCommandSubmit()
{
	m_signalIrradianceGaussianBlurCompletion.emit();

	m_resetLitvoxelData->setTechniqueLock(false);

	m_executeCommand = false;
	m_active         = false;
	m_executeCommand = false;
	m_needsToRecord  = (m_vectorCommand.size() != m_usedCommandBufferNumber);
}

/////////////////////////////////////////////////////////////////////////////////////////////

void IrradianceGaussianBlurTechnique::slotPrefixSumComplete()
{
	m_prefixSumCompleted = true;
}

/////////////////////////////////////////////////////////////////////////////////////////////

void IrradianceGaussianBlurTechnique::slotDynamicLightBounceVoxelIrradianceCompleted()
{
	if (m_prefixSumCompleted)
	{
		m_cameraVisibleVoxelNumber = m_processCameraVisibleResultsTechnique->getDynamicCameraVisibleVoxelNumber();
		m_bufferNumElement         = m_cameraVisibleVoxelNumber * 6; // Each local workgroup will work one side of each voxel in the scene

		obtainDispatchWorkGroupCount();

		uint localWorkGroupsXDimension;
		uint localWorkGroupsYDimension;

		m_bufferNumElement = m_cameraVisibleVoxelNumber;
		BufferProcessTechnique::obtainDispatchWorkGroupCount(m_materialLightBounceDynamicCopyIrradiance->getNumThreadPerLocalWorkgroup(), 1, m_bufferNumElement, localWorkGroupsXDimension, localWorkGroupsYDimension);
		m_materialLightBounceDynamicCopyIrradiance->setLocalWorkGroupsXDimension(localWorkGroupsXDimension);
		m_materialLightBounceDynamicCopyIrradiance->setLocalWorkGroupsYDimension(localWorkGroupsYDimension);
		m_materialLightBounceDynamicCopyIrradiance->setNumThreadExecuted(m_bufferNumElement);

		m_bufferNumElement = m_cameraVisibleVoxelNumber * 6;
		BufferProcessTechnique::obtainDispatchWorkGroupCount(m_materialLightBounceVoxel2DFilter->getNumThreadPerLocalWorkgroup(), 1, m_bufferNumElement, localWorkGroupsXDimension, localWorkGroupsYDimension);
		m_materialLightBounceVoxel2DFilter->setLocalWorkGroupsXDimension(localWorkGroupsXDimension);
		m_materialLightBounceVoxel2DFilter->setLocalWorkGroupsYDimension(localWorkGroupsYDimension);
		m_materialLightBounceVoxel2DFilter->setNumThreadExecuted(m_bufferNumElement);

		// Each time DynamicVoxelVisibilityRayTracingTechnique this technique needs to record
		m_vectorCommand.clear();
		
		m_active = true;
	}
}

/////////////////////////////////////////////////////////////////////////////////////////////
