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
#include "../../include/rastertechnique/skeletalanimationupdatetechnique.h"
#include "../../include/scene/scene.h"
#include "../../include/parameter/attributedefines.h"
#include "../../include/parameter/attributedata.h"
#include "../../include/core/coremanager.h"
#include "../../include/component/skinnedmeshrendercomponent.h"
#include "../../include/buffer/buffer.h"
#include "../../include/buffer/buffermanager.h"
#include "../../include/util/vulkanstructinitializer.h"
#include "../../include/material/materialskeletalanimationupdate.h"
#include "../../include/accelerationstructure/accelerationstructure.h"
#include "../../include/accelerationstructure/accelerationstructuremanager.h"
#include "../../include/uniformbuffer/uniformbuffer.h"

// NAMESPACE
using namespace attributedefines;

// DEFINES

// STATIC MEMBER INITIALIZATION

/////////////////////////////////////////////////////////////////////////////////////////////

SkeletalAnimationUpdateTechnique::SkeletalAnimationUpdateTechnique(string&& name, string&& className) : BufferProcessTechnique(move(name), move(className))
	, m_poseMatrixBuffer(nullptr)
	, m_materialSkeletalAnimationUpdate(nullptr)
	, m_skeletalMeshDebugBuffer(nullptr)
	, m_sceneLoadTransformBuffer(nullptr)
	, m_vertexBufferSkinnedMeshBuffer(nullptr)
	, m_raytracedaccelerationstructure(nullptr)
	, m_dynamicraytracedaccelerationstructure(nullptr)
	, m_cachedAccelerationStructures(false)
{
	m_numElementPerLocalWorkgroupThread = 1;
	m_numThreadPerLocalWorkgroup        = 64;
	m_rasterTechniqueType               = RasterTechniqueType::RTT_COMPUTE;
	m_computeHostSynchronize            = true;
	m_active                            = true;
	m_needsToRecord                     = true;
	m_executeCommand                    = true;

	m_vectorSkinnedMesh = sceneM->getByComponentType(GenericResourceType::GRT_SKINNEDMESHRENDERCOMPONENT);

	forI(m_vectorSkinnedMesh.size())
	{
		m_vectorSkinnedMeshComponent.push_back(static_cast<SkinnedMeshRenderComponent*>(m_vectorSkinnedMesh[i]->refRenderComponent()));
	}

	m_numSkinnedMeshComponent = m_vectorSkinnedMeshComponent.size();

	m_boneMatrixOffset.resize(m_numSkinnedMeshComponent);
	m_vectorNumVertexToProcess.resize(m_numSkinnedMeshComponent);
	m_vectorDispatchSize.resize(m_numSkinnedMeshComponent);
	m_vectorVertexOffset.resize(m_numSkinnedMeshComponent);

	int boneOffsetAccumulatedValue   = 0;
	int vertexOffsetAccumulatedValue = 0;

	forI(m_numSkinnedMeshComponent)
	{
		m_vectorVertexOffset[i]       = float(vertexOffsetAccumulatedValue);
		m_boneMatrixOffset[i]         = boneOffsetAccumulatedValue;
		m_vectorNumVertexToProcess[i] = m_vectorSkinnedMeshComponent[i]->getBoneWeight().size();
		boneOffsetAccumulatedValue   += m_vectorSkinnedMeshComponent[i]->getVectorUsedBoneCurrentPose().size();
		vertexOffsetAccumulatedValue += m_vectorNumVertexToProcess[i];
	}
}

/////////////////////////////////////////////////////////////////////////////////////////////

void SkeletalAnimationUpdateTechnique::init()
{
	int matrixElementByteSize = 0;
	vectorFloat vectorSceneLoadTransform(m_numSkinnedMeshComponent * 16);

	forI(m_numSkinnedMeshComponent)
	{
		matrixElementByteSize    += m_vectorSkinnedMeshComponent[i]->getVectorUsedBoneCurrentPose().size() * 16 * sizeof(float);
		memcpy(vectorSceneLoadTransform.data() + i * 16, value_ptr(m_vectorSkinnedMeshComponent[i]->getSceneLoadTransform()), 16 * sizeof(float));
		m_bufferNumElement        = m_vectorSkinnedMeshComponent[i]->getBoneWeight().size();

		obtainDispatchWorkGroupCount();
		m_vectorDispatchSize[i]   = uvec2(m_localWorkGroupsXDimension, m_localWorkGroupsYDimension);
	}

	m_poseMatrixBuffer = bufferM->buildBuffer(
		move(string("poseMatrixBuffer")),
		nullptr,
		matrixElementByteSize,
		VK_BUFFER_USAGE_STORAGE_BUFFER_BIT  | VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT); // TODO: Change to VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT and test

	m_skeletalMeshDebugBuffer = bufferM->buildBuffer(move(string("skeletalMeshDebugBuffer")),
		nullptr,
		180000000,
		VK_BUFFER_USAGE_STORAGE_BUFFER_BIT  | VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VK_MEMORY_PROPERTY_HOST_COHERENT_BIT); // TODO: Change to VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT and test

	m_sceneLoadTransformBuffer = bufferM->buildBuffer(
		move(string("sceneLoadTransformBuffer")),
		vectorSceneLoadTransform.data(),
		vectorSceneLoadTransform.size() * sizeof(float),
		VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
		VK_MEMORY_PROPERTY_HOST_COHERENT_BIT); // TODO: Change to VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT and test

	m_vectorPoseMatrixData.resize(matrixElementByteSize / 4);

	buildShaderThreadMapping();

	MultiTypeUnorderedMap* attributeMaterial = new MultiTypeUnorderedMap();
	attributeMaterial->newElement<AttributeData<string>*>(new AttributeData<string>(string(g_shaderCodeChunk), string(m_computeShaderThreadMapping)));
	m_material                               = materialM->buildMaterial(move(string("MaterialSkeletalAnimationUpdate")), move(string("MaterialSkeletalAnimationUpdate")), attributeMaterial);
	m_materialSkeletalAnimationUpdate        = static_cast<MaterialSkeletalAnimationUpdate*>(m_material);
	m_vectorMaterialName.push_back("MaterialSkeletalAnimationUpdate");
	m_vectorMaterial.push_back(m_material);

	m_vertexBufferSkinnedMeshBuffer = bufferM->getElement(move(string("vertexBufferSkinnedMesh")));
}

/////////////////////////////////////////////////////////////////////////////////////////////

void SkeletalAnimationUpdateTechnique::prepare(float dt)
{
	if (!m_cachedAccelerationStructures)
	{
		m_raytracedaccelerationstructure        = accelerationStructureM->getElement(move(string("raytracedaccelerationstructure")));
		m_dynamicraytracedaccelerationstructure = accelerationStructureM->getElement(move(string("dynamicraytracedaccelerationstructure")));
		m_cachedAccelerationStructures          = true;
	}

	// TODO: Use real elapsed time and not the hardcoded 0.167 seconds
	SkinnedMeshRenderComponent* component = nullptr;
	int currentOffset = 0;
	forI(m_vectorSkinnedMeshComponent.size())
	{
		component               = m_vectorSkinnedMeshComponent[i];
		component->updateBones(dt);
		vectorFloat& vectorData = component->refVectorUsedBoneCurrentPoseFloat();
		int temp0 = vectorData.size();
		memcpy(m_vectorPoseMatrixData.data() + currentOffset, vectorData.data(), vectorData.size() * sizeof(float));
		currentOffset          += vectorData.size();
	}

	m_poseMatrixBuffer->setContentHostVisible(m_vectorPoseMatrixData.data());
}

/////////////////////////////////////////////////////////////////////////////////////////////

VkCommandBuffer* SkeletalAnimationUpdateTechnique::record(int currentImage, uint& commandBufferID, CommandBufferType& commandBufferType)
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

	VulkanStructInitializer::insertBufferMemoryBarrier(bufferM->getElement(move(string("poseMatrixBuffer"))),
													   VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT,
													   VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT,
													   VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
													   VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
													   commandBuffer);

	VulkanStructInitializer::insertBufferMemoryBarrier(bufferM->getElement(move(string("sceneLoadTransformBuffer"))),
													   VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT,
													   VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT,
													   VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
													   VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
													   commandBuffer);

	uint dynamicAllignment = materialM->getMaterialUBDynamicAllignment();
	vkCmdBindPipeline(*commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, m_material->getPipeline()->getPipeline());

	forI(m_numSkinnedMeshComponent)
	{
		uint32_t offsetData[2];
		uint32_t sceneDataBufferOffset = static_cast<uint32_t>(gpuPipelineM->getSceneUniformData()->getDynamicAllignment());
		uint32_t elementIndex          = sceneM->getElementIndex(m_vectorSkinnedMesh[i]);

		offsetData[0] = elementIndex * sceneDataBufferOffset;
		offsetData[1] = static_cast<uint32_t>(m_materialSkeletalAnimationUpdate->getMaterialUniformBufferIndex() * dynamicAllignment);
		vkCmdBindDescriptorSets(*commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, m_materialSkeletalAnimationUpdate->getPipelineLayout(), 0, 1, &m_materialSkeletalAnimationUpdate->refDescriptorSet(), 2, &offsetData[0]);

		m_materialSkeletalAnimationUpdate->setPushConstantData(vec4(m_boneMatrixOffset[i], m_vectorNumVertexToProcess[i], float(i), m_vectorVertexOffset[i]));
		m_materialSkeletalAnimationUpdate->updatePushConstantCPUBuffer();
		CPUBuffer& cpuBufferNewCenter = m_materialSkeletalAnimationUpdate->refShader()->refPushConstant().refCPUBuffer();

		vkCmdPushConstants(*commandBuffer,
			m_materialSkeletalAnimationUpdate->getPipelineLayout(),
			VK_SHADER_STAGE_COMPUTE_BIT,
			0,
			uint32_t(m_materialSkeletalAnimationUpdate->getPushConstantExposedStructFieldSize()),
			cpuBufferNewCenter.refUBHostMemory());

		vkCmdDispatch(*commandBuffer, m_vectorDispatchSize[i].x, m_vectorDispatchSize[i].y, 1); // Compute shader global workgroup https://www.khronos.org/registry/vulkan/specs/1.1-extensions/html/vkspec.html
	}

	// TODO: Avoid duplicating the vertex information on two buffers (vertexBuffer, vertexBufferSkinnedMesh and each RenderComponent::m_vertexBuffer for
	// ray tracing) by using for RenderComponent::m_vertexBuffer the memory from vertexBuffer and vertexBufferSkinnedMesh buffers already made.
	// Now, copy the results from vertexBufferSkinnedMeshData onto each vertex acceleration structure buffer

	VulkanStructInitializer::insertBufferMemoryBarrier(m_vertexBufferSkinnedMeshBuffer,
													   VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT,
													   VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT,
													   VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
													   VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
													   commandBuffer);

	forI(m_numSkinnedMeshComponent)
	{
		bufferM->copyBuffer(commandBuffer, m_vertexBufferSkinnedMeshBuffer, m_vectorSkinnedMeshComponent[i]->refVertexBuffer(), uint(m_vectorVertexOffset[i]) * gpuPipelineM->getVertexStrideBytes(), 0, uint(m_vectorNumVertexToProcess[i]) * gpuPipelineM->getVertexStrideBytes());

		VulkanStructInitializer::insertBufferMemoryBarrier(m_vectorSkinnedMeshComponent[i]->refVertexBuffer(),
													   VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT,
													   VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT,
													   VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
													   VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
													   commandBuffer);
	}

	m_raytracedaccelerationstructure->updateBLAS(*commandBuffer, m_vectorSkinnedMesh);
	m_dynamicraytracedaccelerationstructure->updateBLAS(*commandBuffer, m_vectorSkinnedMesh);	

#ifdef USE_TIMESTAMP
	vkCmdWriteTimestamp(*commandBuffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, coreM->getComputeQueueQueryPool(), m_queryIndex1);
#endif

	recordBarriersEnd(commandBuffer);

	coreM->endCommandBuffer(*commandBuffer);

	m_vectorCommand.push_back(commandBuffer);

	return commandBuffer;
}

/////////////////////////////////////////////////////////////////////////////////////////////

void SkeletalAnimationUpdateTechnique::postCommandSubmit()
{
	m_executeCommand = false;
	m_needsToRecord  = (m_vectorCommand.size() != m_usedCommandBufferNumber);
}

/////////////////////////////////////////////////////////////////////////////////////////////
