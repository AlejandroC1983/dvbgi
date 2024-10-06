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
#include "../../include/rastertechnique/computefrustumcullingtechnique.h"
#include "../../include/buffer/buffer.h"
#include "../../include/buffer/buffermanager.h"
#include "../../include/material/materialcomputefrustumculling.h"
#include "../../include/material/materialmanager.h"
#include "../../include/parameter/attributedefines.h"
#include "../../include/parameter/attributedata.h"
#include "../../include/camera/cameramanager.h"
#include "../../include/camera/camera.h"
#include "../../include/util/vulkanstructinitializer.h"
#include "../../include/component/basecomponent.h"
#include "../../include/component/rendercomponent.h"

// NAMESPACE
using namespace attributedefines;

// DEFINES

// STATIC MEMBER INITIALIZATION

/////////////////////////////////////////////////////////////////////////////////////////////

ComputeFrustumCullingTechnique::ComputeFrustumCullingTechnique(string&& name, string&& className) : BufferProcessTechnique(move(name), move(className))
	, m_instanceDataBuffer(nullptr)
	, m_frustumDebugBuffer(nullptr)
	, m_frustumElementCounterMainCameraBuffer(nullptr)
	, m_frustumElementCounterEmitterCameraBuffer(nullptr)
	, m_indirectCommandBufferMainCamera(nullptr)
	, m_indirectCommandBufferEmitterCamera(nullptr)
	, m_frustumElementMainCameraCounter(0)
	, m_frustumElementEmitterCameraCounter(0)
	, m_materialComputeFrustumCulling(nullptr)
{
	m_numElementPerLocalWorkgroupThread = 1;
	m_numThreadPerLocalWorkgroup = 64;
	m_rasterTechniqueType = RasterTechniqueType::RTT_COMPUTE;
	m_computeHostSynchronize = true;
	m_active = true;
	m_needsToRecord = true;
}

/////////////////////////////////////////////////////////////////////////////////////////////

void ComputeFrustumCullingTechnique::init()
{
	// TODO: Update instanceDataBuffer data for dynamic elements

	m_arrayNode = sceneM->getByMeshType(eMeshType::E_MT_RENDER_MODEL);
	uint instanceNumElement = uint(m_arrayNode.size());
	m_vectorInstanceData.resize(instanceNumElement);

	forI(m_vectorInstanceData.size())
	{
		m_vectorInstanceData[i].m_position = m_arrayNode[i]->refTransform().getTraslation();
		m_vectorInstanceData[i].m_radius   = m_arrayNode[i]->getBBox().getMaxDist();
	}

	m_instanceDataBuffer = bufferM->buildBuffer(
		move(string("instanceDataBuffer")),
		(void*)(m_vectorInstanceData.data()),
		instanceNumElement * sizeof(InstanceData),
		VK_BUFFER_USAGE_STORAGE_BUFFER_BIT  | VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

	// TODO: Remove debug buffer once everything has been tested properly
	m_frustumDebugBuffer = bufferM->buildBuffer(
		move(string("frustumDebugBuffer")),
		nullptr,
		10240,
		VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

	m_frustumElementCounterMainCameraBuffer = bufferM->buildBuffer(
		move(string("frustumElementCounterMainCameraBuffer")),
		(void*)(&m_frustumElementMainCameraCounter),
		4,
		VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

	m_frustumElementCounterEmitterCameraBuffer = bufferM->buildBuffer(
		move(string("frustumElementCounterEmitterCameraBuffer")),
		(void*)(&m_frustumElementEmitterCameraCounter),
		4,
		VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

	m_arrayIndirectCommand.resize(m_arrayNode.size());

	forI(m_arrayIndirectCommand.size())
	{
		RenderComponent* renderComponent        = m_arrayNode[i]->refRenderComponent();
		m_arrayIndirectCommand[i].instanceCount = 1;
		m_arrayIndirectCommand[i].firstInstance = i;
		m_arrayIndirectCommand[i].firstIndex    = renderComponent->getStartIndex();
		m_arrayIndirectCommand[i].indexCount    = renderComponent->getIndexSize();
		m_arrayIndirectCommand[i].vertexOffset  = 0;
	}

	m_indirectCommandBufferMainCamera = bufferM->buildBuffer(
		move(string("indirectCommandBufferMainCamera")),
		(void*)(m_arrayIndirectCommand.data()),
		sizeof(VkDrawIndexedIndirectCommand) * m_arrayIndirectCommand.size(),
		VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

	m_indirectCommandBufferEmitterCamera = bufferM->buildBuffer(
		move(string("indirectCommandBufferEmitterCamera")),
		(void*)(m_arrayIndirectCommand.data()),
		sizeof(VkDrawIndexedIndirectCommand) * m_arrayIndirectCommand.size(),
		VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

	// Assuming each thread will take care of a whole row / column
	buildShaderThreadMapping();

	m_bufferNumElement = instanceNumElement;

	obtainDispatchWorkGroupCount();

	MultiTypeUnorderedMap* attributeMaterial = new MultiTypeUnorderedMap();
	attributeMaterial->newElement<AttributeData<string>*>(new AttributeData<string>(string(g_computeFrustumCullingCodeChunk), string(m_computeShaderThreadMapping)));
	m_material = materialM->buildMaterial(move(string("MaterialComputeFrustumCulling")), move(string("MaterialComputeFrustumCulling")), attributeMaterial);
	m_materialComputeFrustumCulling = static_cast<MaterialComputeFrustumCulling*>(m_material);
	m_vectorMaterialName.push_back("MaterialComputeFrustumCulling");
	m_vectorMaterial.push_back(m_material);

	m_materialComputeFrustumCulling = static_cast<MaterialComputeFrustumCulling*>(m_vectorMaterial[0]);
	m_materialComputeFrustumCulling->setLocalWorkGroupsXDimension(m_localWorkGroupsXDimension);
	m_materialComputeFrustumCulling->setLocalWorkGroupsYDimension(m_localWorkGroupsYDimension);
	m_materialComputeFrustumCulling->setNumThreadExecuted(m_numThreadExecuted);
}

/////////////////////////////////////////////////////////////////////////////////////////////

void ComputeFrustumCullingTechnique::recordBarriers(VkCommandBuffer* commandBuffer)
{
	VulkanStructInitializer::insertBufferMemoryBarrier(m_indirectCommandBufferMainCamera,
		VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT,
		VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT,
		VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
		VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
		commandBuffer);

	VulkanStructInitializer::insertBufferMemoryBarrier(m_indirectCommandBufferEmitterCamera,
		VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT,
		VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT,
		VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
		VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
		commandBuffer);
}

/////////////////////////////////////////////////////////////////////////////////////////////

void ComputeFrustumCullingTechnique::prepare(float dt)
{
	m_materialComputeFrustumCulling = static_cast<MaterialComputeFrustumCulling*>(m_vectorMaterial[0]);

	const vec4* frustumPlanesMainCamera = cameraM->getElement(move(string("maincamera")))->getArrayFrustumPlane();

	m_materialComputeFrustumCulling->setFrustumPlaneLeftMainCamera(frustumPlanesMainCamera[0]);
	m_materialComputeFrustumCulling->setFrustumPlaneRightMainCamera(frustumPlanesMainCamera[1]);
	m_materialComputeFrustumCulling->setFrustumPlaneTopMainCamera(frustumPlanesMainCamera[2]);
	m_materialComputeFrustumCulling->setFrustumPlaneBottomMainCamera(frustumPlanesMainCamera[3]);
	m_materialComputeFrustumCulling->setFrustumPlaneBackMainCamera(frustumPlanesMainCamera[4]);
	m_materialComputeFrustumCulling->setFrustumPlaneFrontMainCamera(frustumPlanesMainCamera[5]);

	const Camera* emitterCamera = cameraM->getElement(move(string("emitter")));

	if (emitterCamera != nullptr)
	{
		const vec4* frustumPlanesEmitterCamera = cameraM->getElement(move(string("emitter")))->getArrayFrustumPlane();

		m_materialComputeFrustumCulling->setFrustumPlaneLeftEmitterCamera(frustumPlanesEmitterCamera[0]);
		m_materialComputeFrustumCulling->setFrustumPlaneRightEmitterCamera(frustumPlanesEmitterCamera[1]);
		m_materialComputeFrustumCulling->setFrustumPlaneTopEmitterCamera(frustumPlanesEmitterCamera[2]);
		m_materialComputeFrustumCulling->setFrustumPlaneBottomEmitterCamera(frustumPlanesEmitterCamera[3]);
		m_materialComputeFrustumCulling->setFrustumPlaneBackEmitterCamera(frustumPlanesEmitterCamera[4]);
		m_materialComputeFrustumCulling->setFrustumPlaneFrontEmitterCamera(frustumPlanesEmitterCamera[5]);
	}
}

/////////////////////////////////////////////////////////////////////////////////////////////

void ComputeFrustumCullingTechnique::postCommandSubmit()
{
	m_executeCommand = false;
	m_needsToRecord = false;
}

/////////////////////////////////////////////////////////////////////////////////////////////
