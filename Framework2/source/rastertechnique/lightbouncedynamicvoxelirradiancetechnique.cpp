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
#include "../../include/rastertechnique/lightbouncedynamicvoxelirradiancetechnique.h"
#include "../../include/core/coremanager.h"
#include "../../include/scene/scene.h"
#include "../../include/core/gpupipeline.h"
#include "../../include/rastertechnique/staticscenevoxelizationtechnique.h"
#include "../../include/rastertechnique/bufferprefixsumtechnique.h"
#include "../../include/rastertechnique/litvoxeltechnique.h"
#include "../../include/camera/camera.h"
#include "../../include/camera/cameramanager.h"
#include "../../include/util/vulkanstructinitializer.h"
#include "../../include/rastertechnique/processcameravisibleresultstechnique.h"
#include "../../include/material/materialmanager.h"
#include "../../include/material/materiallightbouncedynamicvoxelirradiancecompute.h"

// NAMESPACE

// DEFINES

// STATIC MEMBER INITIALIZATION

/////////////////////////////////////////////////////////////////////////////////////////////

LightBounceDynamicVoxelIrradianceTechnique::LightBounceDynamicVoxelIrradianceTechnique(string &&name, string&& className) : BufferProcessTechnique(move(name), move(className))
	, m_techniquePrefixSum(nullptr)
	, m_litVoxelTechnique(nullptr)
	, m_dynamicVisibleVoxel(0)
	, m_prefixSumCompleted(false)
	, m_processCameraVisibleResultsTechnique(nullptr)
	, m_materialLightBounceDynamicVoxelIrradianceCompute(nullptr)
	, m_emitterCamera(nullptr)
	, m_emitterRadiance(0.0f)
	, m_lightBounceDynamicVoxelIrradianceDebugBuffer(nullptr)
{
	m_active                 = false;
	m_needsToRecord          = false;
	m_executeCommand         = false;
	m_rasterTechniqueType    = RasterTechniqueType::RTT_COMPUTE;
	m_computeHostSynchronize = true;
}

/////////////////////////////////////////////////////////////////////////////////////////////

void LightBounceDynamicVoxelIrradianceTechnique::init()
{
	StaticSceneVoxelizationTechnique* sceneVoxelizationTechnique = static_cast<StaticSceneVoxelizationTechnique*>(gpuPipelineM->getRasterTechniqueByName(move(string("StaticSceneVoxelizationTechnique"))));
	int voxelizedSceneWidth                                      = sceneVoxelizationTechnique->getVoxelizedSceneWidth();

	m_materialLightBounceDynamicVoxelIrradianceCompute = static_cast<MaterialLightBounceDynamicVoxelIrradianceCompute*>(materialM->buildMaterial(move(string("MaterialLightBounceDynamicVoxelIrradianceCompute")), move(string("MaterialLightBounceDynamicVoxelIrradianceCompute")), nullptr));
	m_vectorMaterialName.push_back("MaterialLightBounceDynamicVoxelIrradianceCompute");
	m_vectorMaterial.push_back(m_materialLightBounceDynamicVoxelIrradianceCompute);

	BBox3D& box = sceneM->refBox();
	
	vec3 max3D;
	vec3 min3D;
	box.getCenteredBoxMinMax(min3D, max3D);
	
	vec3 extent3D                          = max3D - min3D;
	m_sceneMin                             = vec4(min3D.x,    min3D.y,    min3D.z, 0.0f);
	m_sceneExtent                          = vec4(extent3D.x, extent3D.y, extent3D.z, float(voxelizedSceneWidth));	
	m_emitterCamera                        = cameraM->getElement(move(string("emitter")));

	m_processCameraVisibleResultsTechnique = static_cast<ProcessCameraVisibleResultsTechnique*>(gpuPipelineM->getRasterTechniqueByName(move(string("ProcessCameraVisibleResultsTechnique"))));
	m_litVoxelTechnique                    = static_cast<LitVoxelTechnique*>(gpuPipelineM->getRasterTechniqueByName(move(string("LitVoxelTechnique"))));
	m_techniquePrefixSum                   = static_cast<BufferPrefixSumTechnique*>(gpuPipelineM->getRasterTechniqueByName(move(string("BufferPrefixSumTechnique"))));

	m_materialLightBounceDynamicVoxelIrradianceCompute->setSceneExtentAndVoxelSize(m_sceneExtent);
	m_materialLightBounceDynamicVoxelIrradianceCompute->setSceneMin(m_sceneMin);
	m_techniquePrefixSum->refPrefixSumComplete().connect<LightBounceDynamicVoxelIrradianceTechnique, &LightBounceDynamicVoxelIrradianceTechnique::slotPrefixSumComplete>(this);
	m_processCameraVisibleResultsTechnique->refSignalProcessCameraVisibleCompletion().connect<LightBounceDynamicVoxelIrradianceTechnique, &LightBounceDynamicVoxelIrradianceTechnique::slotProcessCameraVisibleResultsTechniqueCompleted>(this);

	m_lightBounceDynamicVoxelIrradianceDebugBuffer = bufferM->buildBuffer(
		move(string("lightBounceDynamicVoxelIrradianceDebugBuffer")),
		nullptr,
		1000000,
		VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
}

/////////////////////////////////////////////////////////////////////////////////////////////

VkCommandBuffer* LightBounceDynamicVoxelIrradianceTechnique::record(int currentImage, uint& commandBufferID, CommandBufferType& commandBufferType)
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

	VulkanStructInitializer::insertBufferMemoryBarrier(bufferM->getElement(move(string("cameraVisibleDynamicVoxelBuffer"))),
													   VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT,
													   VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT,
													   VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
													   VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
													   commandBuffer);

	VulkanStructInitializer::insertBufferMemoryBarrier(bufferM->getElement(move(string("dynamicVoxelVisibilityBuffer"))),
													   VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT,
													   VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT,
													   VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
													   VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
													   commandBuffer);

	uint dynamicAllignment = materialM->getMaterialUBDynamicAllignment();

	uint32_t offsetData;
	vkCmdBindPipeline(*commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, m_materialLightBounceDynamicVoxelIrradianceCompute->getPipeline()->getPipeline());
	offsetData = static_cast<uint32_t>(m_materialLightBounceDynamicVoxelIrradianceCompute->getMaterialUniformBufferIndex() * dynamicAllignment);
	vkCmdBindDescriptorSets(*commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, m_materialLightBounceDynamicVoxelIrradianceCompute->getPipelineLayout(), 0, 1, &m_materialLightBounceDynamicVoxelIrradianceCompute->refDescriptorSet(), 1, &offsetData);
	vkCmdDispatch(*commandBuffer, m_materialLightBounceDynamicVoxelIrradianceCompute->getLocalWorkGroupsXDimension(), m_materialLightBounceDynamicVoxelIrradianceCompute->getLocalWorkGroupsYDimension(), 1); // Compute shader global workgroup https://www.khronos.org/registry/vulkan/specs/1.1-extensions/html/vkspec.html

#ifdef USE_TIMESTAMP
	vkCmdWriteTimestamp(*commandBuffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, coreM->getComputeQueueQueryPool(), m_queryIndex1);
#endif

	coreM->endCommandBuffer(*commandBuffer);

	// NOTE: Clear command buffer if re-recorded
	m_vectorCommand.push_back(commandBuffer);

	return commandBuffer;
}

/////////////////////////////////////////////////////////////////////////////////////////////

void LightBounceDynamicVoxelIrradianceTechnique::postCommandSubmit()
{
	m_signalLightBounceDynamicVoxelIrradianceCompletion.emit();

	m_executeCommand = false;
	m_active         = false;
	m_executeCommand = false;
	m_needsToRecord  = (m_vectorCommand.size() != m_usedCommandBufferNumber);
}

/////////////////////////////////////////////////////////////////////////////////////////////

void LightBounceDynamicVoxelIrradianceTechnique::slotPrefixSumComplete()
{
	m_prefixSumCompleted = true;
}

/////////////////////////////////////////////////////////////////////////////////////////////

void LightBounceDynamicVoxelIrradianceTechnique::slotProcessCameraVisibleResultsTechniqueCompleted()
{
	if (m_prefixSumCompleted)
	{
		m_dynamicVisibleVoxel = m_processCameraVisibleResultsTechnique->getDynamicCameraVisibleVoxelNumber();
		m_bufferNumElement    = m_dynamicVisibleVoxel * NUM_RAY_PER_VOXEL_FACE * 6; // Each local workgroup will work one side of each voxel in the scene
		m_sceneMin.w          = float(m_bufferNumElement);

		// TODO: automatize this
		Camera* shadowMappingCamera = cameraM->getElement(move(string("emitter")));
		vec3 lightPosition          = shadowMappingCamera->getPosition();
		vec3 lightForward           = shadowMappingCamera->getLookAt();
		m_emitterRadiance           = float(gpuPipelineM->getRasterFlagValue(move(string("EMITTER_RADIANCE"))));

		m_materialLightBounceDynamicVoxelIrradianceCompute->setLightPosition(vec4(lightPosition.x, lightPosition.y, lightPosition.z, 0.0f));
		m_materialLightBounceDynamicVoxelIrradianceCompute->setLightForwardEmitterRadiance(vec4(lightForward.x, lightForward.y, lightForward.z, m_emitterRadiance));
		m_materialLightBounceDynamicVoxelIrradianceCompute->setNumThreadExecuted(m_bufferNumElement / m_materialLightBounceDynamicVoxelIrradianceCompute->getNumElementPerLocalWorkgroupThread());
		m_materialLightBounceDynamicVoxelIrradianceCompute->obtainDispatchWorkGroupCount(m_bufferNumElement);
		m_materialLightBounceDynamicVoxelIrradianceCompute->setNumberElementToProcess(m_bufferNumElement);

		m_vectorCommand.clear();

		m_active = true;
	}
}

/////////////////////////////////////////////////////////////////////////////////////////////
