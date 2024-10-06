/*
Copyright 2021 Alejandro Cosin

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
#include "../../include/accelerationstructure/accelerationstructure.h"
#include "../../include/node/node.h"
#include "../../include/buffer/buffer.h"
#include "../../include/buffer/buffermanager.h"
#include "../../include/core/gpupipeline.h"
#include "../../include/core/functionpointer.h"
#include "../../include/core/coremanager.h"
#include "../../include/util/vulkanstructinitializer.h"
#include "../../include/component/basecomponent.h"
#include "../../include/component/rendercomponent.h"

// NAMESPACE

// DEFINES

// STATIC MEMBER INITIALIZATION
uint AccelerationStructure::m_instanceCounter = 0;

/////////////////////////////////////////////////////////////////////////////////////////////

AccelerationStructure::AccelerationStructure(string &&name, vectorNodePtr&& vectorNode) : GenericResource(move(name) , move(string("AccelerationStructure"))
	, GenericResourceType::GRT_ACCELERATIONSTRUCTURE)
	, m_vectorNode(move(vectorNode))
	, m_hostVisibleTLASBuffer(nullptr)
	, m_deviceLocalTLASBuffer(nullptr)
	, m_tlasScratchBuffer(nullptr)
	, m_usingDeviceLocalBuffer0(true)
{
	forI(m_vectorNode.size())
	{
		assert(true);
		auto result = m_mapNodeInternalIndex.insert(std::make_pair(m_vectorNode[i], i));
		if (result.second == false)
		{
			cout << "ERROR: Node already inserted in m_mapNodeInternalIndex in AccelerationStructure::AccelerationStructure" << endl;
		}
	}
}

/////////////////////////////////////////////////////////////////////////////////////////////

AccelerationStructure::~AccelerationStructure()
{
	destroyResources();
}

/////////////////////////////////////////////////////////////////////////////////////////////

void AccelerationStructure::updateTLAS(const vectorNodePtr* vectorUpdateNode)
{
	buildTLAS(true, vectorUpdateNode);
}

/////////////////////////////////////////////////////////////////////////////////////////////

void AccelerationStructure::updateBLAS(VkCommandBuffer commandBuffer, const vectorNodePtr& vectorNode)
{
	// TODO: Cache the index and vertex device address values

	// Do one element for now before generalisation
	if (m_mapNodeInternalIndex.find(vectorNode[0]) == m_mapNodeInternalIndex.end())
	{
		cout << "ERROR: Trying to update a non existing node in AccelerationStructure::updateBLAS" << endl;
	}

	int index;
	RenderComponent* component = nullptr;
	int numElement             = int(vectorNode.size());

	VkDeviceSize maximumScratchSize = 0;

	vector<VkAccelerationStructureGeometryKHR> vectorAccelerationStructureGeometry(numElement);
	vector<VkAccelerationStructureBuildGeometryInfoKHR> vectorAccelerationStructureBuildGeometryInfo(numElement);

	forI(numElement)
	{
		index     = m_mapNodeInternalIndex[vectorNode[i]];
		component = m_vectorNode[index]->refRenderComponent();

		VkAccelerationStructureGeometryTrianglesDataKHR accelerationStructureGeometryTrianglesData = { VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_TRIANGLES_DATA_KHR };
		accelerationStructureGeometryTrianglesData.vertexFormat  = VK_FORMAT_R32G32B32_SFLOAT;
		accelerationStructureGeometryTrianglesData.vertexData    = VkDeviceOrHostAddressConstKHR{ component->refVertexBuffer()->getBufferDeviceAddress() };
		accelerationStructureGeometryTrianglesData.vertexStride  = static_cast<VkDeviceSize>(gpuPipelineM->getVertexStrideBytes());
		accelerationStructureGeometryTrianglesData.indexType     = VK_INDEX_TYPE_UINT32;
		accelerationStructureGeometryTrianglesData.indexData     = VkDeviceOrHostAddressConstKHR{ component->refIndexBuffer()->getBufferDeviceAddress() };
		accelerationStructureGeometryTrianglesData.maxVertex     = static_cast<VkIndexType>(component->getVertices().size());
		accelerationStructureGeometryTrianglesData.transformData = {};

		vectorAccelerationStructureGeometry[i] = { VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR };
		vectorAccelerationStructureGeometry[i].geometryType       = VK_GEOMETRY_TYPE_TRIANGLES_KHR;
		vectorAccelerationStructureGeometry[i].flags              = VK_GEOMETRY_OPAQUE_BIT_KHR;
		vectorAccelerationStructureGeometry[i].geometry.triangles = accelerationStructureGeometryTrianglesData;

		vectorAccelerationStructureBuildGeometryInfo[i] = { VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR };
		vectorAccelerationStructureBuildGeometryInfo[i].flags                    = VK_BUILD_ACCELERATION_STRUCTURE_ALLOW_UPDATE_BIT_KHR | VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_BUILD_BIT_KHR;
		vectorAccelerationStructureBuildGeometryInfo[i].geometryCount            = 1;
		vectorAccelerationStructureBuildGeometryInfo[i].pGeometries              = &vectorAccelerationStructureGeometry[i];
		vectorAccelerationStructureBuildGeometryInfo[i].mode                     = VK_BUILD_ACCELERATION_STRUCTURE_MODE_UPDATE_KHR;
		vectorAccelerationStructureBuildGeometryInfo[i].type                     = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
		vectorAccelerationStructureBuildGeometryInfo[i].srcAccelerationStructure = m_blas[index];
		vectorAccelerationStructureBuildGeometryInfo[i].dstAccelerationStructure = m_blas[index];

		uint32_t maxPrimitiveCount         = static_cast<uint32_t>(component->refIndices().size());
		std::vector<uint32_t> maxPrimCount = { maxPrimitiveCount / 3 + (maxPrimitiveCount % 3 == 0 ? 0 : 1) };
		VkAccelerationStructureBuildSizesInfoKHR asBuildSizesInfo{ VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR };

		vkfpM->vkGetAccelerationStructureBuildSizesKHR(coreM->getLogicalDevice(),
													   VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR,
													   &vectorAccelerationStructureBuildGeometryInfo[i],
													   maxPrimCount.data(),
													   &asBuildSizesInfo);

		maximumScratchSize = std::max(maximumScratchSize, asBuildSizesInfo.buildScratchSize);
	}

	// TODO: Add code to make sure the size is not lower (add assert and cache the value)
	Buffer* scratchBuffer = bufferM->buildBuffer(move(m_name + string("UpdateScratchBuffer")),
												 nullptr,
												 maximumScratchSize,
												 VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
												 VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

	// Get the address of the scratch buffer.
	VkDeviceAddress scratchAddress = scratchBuffer->getBufferDeviceAddress();

	forI(numElement)
	{
		index                           = m_mapNodeInternalIndex[vectorNode[i]];
		uint32_t maxPrimitiveCountRange = static_cast<uint32_t>(m_vectorNode[index]->refRenderComponent()->refIndices().size());
		uint32_t maxPrimCountRange      = maxPrimitiveCountRange / 3 + (maxPrimitiveCountRange % 3 == 0 ? 0 : 1);

		VkAccelerationStructureBuildRangeInfoKHR accelerationStructureBuildOffset = { maxPrimCountRange, 0, 0, 0 };
		std::vector<const VkAccelerationStructureBuildRangeInfoKHR*> vectorAccelerationStructureBuildOffset(1);
		vectorAccelerationStructureBuildOffset[0] = &accelerationStructureBuildOffset;

		vectorAccelerationStructureBuildGeometryInfo[i].scratchData.deviceAddress = scratchAddress;

		vkfpM->vkCmdBuildAccelerationStructuresKHR(commandBuffer, 1, &vectorAccelerationStructureBuildGeometryInfo[i], vectorAccelerationStructureBuildOffset.data());

		VulkanStructInitializer::insertMemoryBarrier(VK_ACCESS_ACCELERATION_STRUCTURE_WRITE_BIT_KHR,
													 VK_ACCESS_ACCELERATION_STRUCTURE_READ_BIT_KHR,
													 VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_KHR,
													 VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_KHR,
													 &commandBuffer);
	}
}

/////////////////////////////////////////////////////////////////////////////////////////////

void AccelerationStructure::buildBLAS()
{
	// TODO: Delete all BLAS and TLAS when the AS is destroyed
	m_blas.resize(m_vectorNode.size());

	std::vector<VkAccelerationStructureGeometryKHR> vectorASGeometry(m_vectorNode.size());
	std::vector<VkAccelerationStructureBuildGeometryInfoKHR> vectorASBuildGeometryInfo(m_vectorNode.size());
	
	VkDeviceSize maximumScratchSize = 0;

	m_blasBuffer.resize(m_vectorNode.size());
	m_blasBufferDeviceAddres.resize(m_vectorNode.size());

	for (int i = 0; i < m_vectorNode.size(); ++i)
	{
		RenderComponent* renderComponent    = m_vectorNode[i]->refRenderComponent();
		VkDeviceAddress vertexBufferAddress = renderComponent->refVertexBuffer()->getBufferDeviceAddress();
		VkDeviceAddress indexBufferAddress  = renderComponent->refIndexBuffer()->getBufferDeviceAddress();

		VkAccelerationStructureGeometryTrianglesDataKHR triangleGeometryData = { VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_TRIANGLES_DATA_KHR };
		triangleGeometryData.vertexFormat  = VK_FORMAT_R32G32B32_SFLOAT;
		triangleGeometryData.vertexData    = VkDeviceOrHostAddressConstKHR{ vertexBufferAddress };
		triangleGeometryData.vertexStride  = static_cast<VkDeviceSize>(gpuPipelineM->getVertexStrideBytes());
		triangleGeometryData.indexType     = VK_INDEX_TYPE_UINT32;
		triangleGeometryData.indexData     = VkDeviceOrHostAddressConstKHR{ indexBufferAddress };
		triangleGeometryData.maxVertex     = static_cast<VkIndexType>(renderComponent->refVertices().size());
		triangleGeometryData.transformData = {};

		vectorASGeometry[i]                    = { VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR };
		vectorASGeometry[i].geometryType       = VK_GEOMETRY_TYPE_TRIANGLES_KHR;
		vectorASGeometry[i].flags              = VK_GEOMETRY_OPAQUE_BIT_KHR;
		vectorASGeometry[i].geometry.triangles = triangleGeometryData;

		vectorASBuildGeometryInfo[i] = {};

		if (renderComponent->getResourceType() == GenericResourceType::GRT_SKINNEDMESHRENDERCOMPONENT)
		{
			// Skeletal meshes will be updated per frame so prefer a more efficient BLAS build
			vectorASBuildGeometryInfo[i].flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_BUILD_BIT_KHR | VK_BUILD_ACCELERATION_STRUCTURE_ALLOW_UPDATE_BIT_KHR;
		}
		else
		{
			vectorASBuildGeometryInfo[i].flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR | VK_BUILD_ACCELERATION_STRUCTURE_ALLOW_UPDATE_BIT_KHR;
		}

		vectorASBuildGeometryInfo[i].sType                    = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
		vectorASBuildGeometryInfo[i].geometryCount            = 1;
		vectorASBuildGeometryInfo[i].pGeometries              = &vectorASGeometry[i];
		vectorASBuildGeometryInfo[i].mode                     = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
		vectorASBuildGeometryInfo[i].type                     = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
		vectorASBuildGeometryInfo[i].srcAccelerationStructure = VK_NULL_HANDLE;

		uint32_t maxPrimitiveCount         = static_cast<uint32_t>(renderComponent->refIndices().size());
		std::vector<uint32_t> maxPrimCount = { maxPrimitiveCount / 3 + (maxPrimitiveCount % 3 == 0 ? 0 : 1) };
		VkAccelerationStructureBuildSizesInfoKHR asBuildSizesInfo{ VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR };
		vkfpM->vkGetAccelerationStructureBuildSizesKHR(coreM->getLogicalDevice(),
			VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR, &vectorASBuildGeometryInfo[i], maxPrimCount.data(),
			&asBuildSizesInfo);

		string name = m_vectorNode[i]->getName() + "blasBuffer";
		m_blasBuffer[i] = bufferM->buildBuffer(move(name),
			nullptr,
			asBuildSizesInfo.accelerationStructureSize,
			VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

		m_blasBufferDeviceAddres[i] = m_blasBuffer[i]->getBufferDeviceAddress();

		VkAccelerationStructureCreateInfoKHR asCreateInfo = {};
		asCreateInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR;
		asCreateInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
		asCreateInfo.size = asBuildSizesInfo.accelerationStructureSize;
		asCreateInfo.buffer = m_blasBuffer[i]->getBuffer();

		vkfpM->vkCreateAccelerationStructureKHR(coreM->getLogicalDevice(), &asCreateInfo, nullptr, &m_blas[i]);

		maximumScratchSize = std::max(maximumScratchSize, asBuildSizesInfo.buildScratchSize);

		vectorASBuildGeometryInfo[i].dstAccelerationStructure = m_blas[i];
	}

	// A scratch buffer with the size of the biggest bottom level acceleration structure geometry element needs to be built and provided
	// to build the bottom level acceleration structure.
	Buffer* scratchBuffer = bufferM->buildBuffer(move(m_name + string("scratchBuffer") + to_string(m_instanceCounter)),
		nullptr,
		maximumScratchSize,
		VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

	// Get the address of the scratch buffer.
	VkDeviceAddress scratchAddress = scratchBuffer->getBufferDeviceAddress();

	VkCommandBuffer	commandBuffer; //!< Command buffer for vertex buffer - Triangle geometry
	coreM->allocCommandBuffer(&coreM->getLogicalDevice(), coreM->getGraphicsCommandPool(), &commandBuffer);
	coreM->beginCommandBuffer(commandBuffer);

	for (int i = 0; i < m_vectorNode.size(); ++i)
	{
		// The bottom level acceleration structure gets the handle to the bottom level acceleration structure, the
		// vector of AS structure geometries in _blas::_geometry and the address of the scratch buffer.
		vectorASBuildGeometryInfo[i].scratchData.deviceAddress = scratchAddress;

		uint32_t maxPrimitiveCount = static_cast<uint32_t>(m_vectorNode[i]->refRenderComponent()->refIndices().size());
		uint32_t maxPrimCount = maxPrimitiveCount / 3 + (maxPrimitiveCount % 3 == 0 ? 0 : 1);

		// Information describing each acceleration structure geometry.
		VkAccelerationStructureBuildRangeInfoKHR accelerationStructureBuildOffset = { maxPrimCount, 0, 0, 0 };
		std::vector<const VkAccelerationStructureBuildRangeInfoKHR*> vectorAccelerationStructureBuildOffset(1);
		vectorAccelerationStructureBuildOffset[0] = &accelerationStructureBuildOffset;

		vkfpM->vkCmdBuildAccelerationStructuresKHR(commandBuffer, 1, &vectorASBuildGeometryInfo[i], vectorAccelerationStructureBuildOffset.data());

		VulkanStructInitializer::insertMemoryBarrier(VK_ACCESS_ACCELERATION_STRUCTURE_WRITE_BIT_KHR,
													 VK_ACCESS_ACCELERATION_STRUCTURE_READ_BIT_KHR,
													 VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_KHR,
													 VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_KHR,
													 &commandBuffer);
	}

	coreM->endCommandBuffer(commandBuffer);
	coreM->submitCommandBuffer(coreM->getLogicalDeviceGraphicsQueue(), &commandBuffer);
	VkCommandBuffer cmdBufs[] = { commandBuffer };
	vkFreeCommandBuffers(coreM->getLogicalDevice(), coreM->getGraphicsCommandPool(), sizeof(cmdBufs) / sizeof(VkCommandBuffer), cmdBufs);
	
	bufferM->removeElement(move(string(scratchBuffer->getName())));
}

/////////////////////////////////////////////////////////////////////////////////////////////

void AccelerationStructure::buildTLAS(bool update, const vectorNodePtr* vectorUpdateNode)
{
	// TODO: For cases where only a few elements are being updated, kee the previous bufer and do a partial updae of the corresponding indices
	const vectorNodePtr* vectorNodeData = update ? vectorUpdateNode : &m_vectorNode;
	vector<VkAccelerationStructureInstanceKHR> vectorAccelerationStructureInstanceKHR(vectorNodeData->size());

	for (int i = 0; i < vectorNodeData->size(); ++i)
	{
		auto it = m_mapNodeInternalIndex.find((*vectorNodeData)[i]);
		if (it == m_mapNodeInternalIndex.end())
		{
			cout << "ERROR: Node with no internal index in AccelerationStructure::buildTLAS" << endl;
			assert(false);
		}
		uint nodeIndexInternal = it->second;

		mat4 transformTransposed = transpose((*vectorNodeData)[i]->getModelMat());
		memcpy(&vectorAccelerationStructureInstanceKHR[i].transform, &transformTransposed, sizeof(mat3x4));
		vectorAccelerationStructureInstanceKHR[i].instanceCustomIndex                    = nodeIndexInternal;
		vectorAccelerationStructureInstanceKHR[i].mask                                   = 0xFF;
		vectorAccelerationStructureInstanceKHR[i].instanceShaderBindingTableRecordOffset = 0; // TODO: This might need to be parameterised
		vectorAccelerationStructureInstanceKHR[i].flags                                  = VK_GEOMETRY_INSTANCE_TRIANGLE_FACING_CULL_DISABLE_BIT_KHR;	
		vectorAccelerationStructureInstanceKHR[i].accelerationStructureReference         = m_blasBufferDeviceAddres[nodeIndexInternal];
	}

	VkCommandBuffer	commandBuffer; //!< Command buffer for vertex buffer - Triangle geometry
	coreM->allocCommandBuffer(&coreM->getLogicalDevice(), coreM->getGraphicsCommandPool(), &commandBuffer);
	coreM->beginCommandBuffer(commandBuffer);

	if (!update)
	{
		m_hostVisibleTLASBuffer = bufferM->buildBuffer(move(string("hostVisibleASInstanceBuffer") + to_string(m_instanceCounter)),
			vectorAccelerationStructureInstanceKHR.data(),
			sizeof(VkAccelerationStructureInstanceKHR) * vectorAccelerationStructureInstanceKHR.size(),
			VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

		m_deviceLocalTLASBuffer = bufferM->buildBuffer(move(string("deviceLocalASInstanceBuffer") + to_string(m_instanceCounter)),
			m_hostVisibleTLASBuffer,
			VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);			
	}
	else
	{
		assert(m_hostVisibleTLASBuffer->getUsePersistentMapping());
		uint8_t* mappedPointer = m_hostVisibleTLASBuffer->refMappedPointer();

		forI(vectorNodeData->size())
		{
			auto it = m_mapNodeInternalIndex.find((*vectorNodeData)[i]);
			if (it == m_mapNodeInternalIndex.end())
			{
				cout << "ERROR: Node with no internal index in AccelerationStructure::buildTLAS" << endl;
				assert(false);
			}
			uint nodeIndexInternal = it->second;

			uint8_t* dataSource = (uint8_t*)(&vectorAccelerationStructureInstanceKHR[i]);

			mappedPointer += sizeof(VkAccelerationStructureInstanceKHR) * nodeIndexInternal;
			memcpy(mappedPointer, dataSource, sizeof(VkAccelerationStructureInstanceKHR));
		}

		VkResult result = vkFlushMappedMemoryRanges(coreM->getLogicalDevice(), 1, &m_hostVisibleTLASBuffer->getMappedRange());

		// TODO: Avoid a whole copy buffer operation and copy just he regions being modified
		bufferM->copyBuffer(&commandBuffer, m_hostVisibleTLASBuffer, m_deviceLocalTLASBuffer, 0, 0, m_hostVisibleTLASBuffer->m_dataSize);
	}
	
	VulkanStructInitializer::insertMemoryBarrier(VK_ACCESS_TRANSFER_WRITE_BIT,
												 VK_ACCESS_ACCELERATION_STRUCTURE_WRITE_BIT_KHR,
												 VK_PIPELINE_STAGE_TRANSFER_BIT,
												 VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_KHR,
												 &commandBuffer);

	VkAccelerationStructureGeometryInstancesDataKHR accelerationStructureGeometryInstancesData = { VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_INSTANCES_DATA_KHR };
	accelerationStructureGeometryInstancesData.arrayOfPointers    = VK_FALSE;
	accelerationStructureGeometryInstancesData.data.deviceAddress = m_deviceLocalTLASBuffer->getBufferDeviceAddress();

	VkAccelerationStructureGeometryKHR accelerationStructureGeometry = { VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR };
	accelerationStructureGeometry.geometryType       = VK_GEOMETRY_TYPE_INSTANCES_KHR;
	accelerationStructureGeometry.geometry.instances = accelerationStructureGeometryInstancesData;

	VkAccelerationStructureBuildGeometryInfoKHR accelerationStructureBuildGeometryInfo = { VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR };
	accelerationStructureBuildGeometryInfo.flags                    = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR | VK_BUILD_ACCELERATION_STRUCTURE_ALLOW_UPDATE_BIT_KHR;
	accelerationStructureBuildGeometryInfo.geometryCount            = 1;
	accelerationStructureBuildGeometryInfo.pGeometries              = &accelerationStructureGeometry;
	accelerationStructureBuildGeometryInfo.mode                     = update ? VK_BUILD_ACCELERATION_STRUCTURE_MODE_UPDATE_KHR : VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
	accelerationStructureBuildGeometryInfo.type                     = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;
	accelerationStructureBuildGeometryInfo.srcAccelerationStructure = VK_NULL_HANDLE;

	uint numElement = uint(m_vectorNode.size());
	VkAccelerationStructureBuildSizesInfoKHR accelerationStructureBuildSizesInfo = { VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR };
	vkfpM->vkGetAccelerationStructureBuildSizesKHR(coreM->getLogicalDevice(), VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR, &accelerationStructureBuildGeometryInfo, &numElement, &accelerationStructureBuildSizesInfo);

	if (!update)
	{
		Buffer* tlasBuffer = bufferM->buildBuffer(move(string("tlasInstanceBuffer") + to_string(m_instanceCounter)),
			nullptr,
			accelerationStructureBuildSizesInfo.accelerationStructureSize,
			VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
	
		VkAccelerationStructureCreateInfoKHR accelerationStructureCreateInfo = { VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR };
		accelerationStructureCreateInfo.type   = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;
		accelerationStructureCreateInfo.size   = accelerationStructureBuildSizesInfo.accelerationStructureSize;
		accelerationStructureCreateInfo.buffer = tlasBuffer->getBuffer();

		vkfpM->vkCreateAccelerationStructureKHR(coreM->getLogicalDevice(), &accelerationStructureCreateInfo, nullptr, &m_tlas);

		m_tlasScratchBuffer = bufferM->buildBuffer(move(string("tlasScratchBuffer") + to_string(m_instanceCounter)),
			nullptr,
			accelerationStructureBuildSizesInfo.buildScratchSize,
			VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
	}

	accelerationStructureBuildGeometryInfo.srcAccelerationStructure  = update ? m_tlas : VK_NULL_HANDLE;
	accelerationStructureBuildGeometryInfo.dstAccelerationStructure  = m_tlas;
	accelerationStructureBuildGeometryInfo.scratchData.deviceAddress = m_tlasScratchBuffer->getBufferDeviceAddress();

	VkAccelerationStructureBuildRangeInfoKHR accelerationStructureBuildRangeInfo = { numElement, 0, 0, 0 };
	VkAccelerationStructureBuildRangeInfoKHR* pAccelerationStructureBuildRangeInfo = &accelerationStructureBuildRangeInfo;

	vkfpM->vkCmdBuildAccelerationStructuresKHR(commandBuffer, 1, &accelerationStructureBuildGeometryInfo, &pAccelerationStructureBuildRangeInfo);

	coreM->endCommandBuffer(commandBuffer);
	coreM->submitCommandBuffer(coreM->getLogicalDeviceGraphicsQueue(), &commandBuffer);
	VkCommandBuffer cmdBufs[] = { commandBuffer };
	vkFreeCommandBuffers(coreM->getLogicalDevice(), coreM->getGraphicsCommandPool(), sizeof(cmdBufs) / sizeof(VkCommandBuffer), cmdBufs);
	m_usingDeviceLocalBuffer0 = !m_usingDeviceLocalBuffer0;
}

/////////////////////////////////////////////////////////////////////////////////////////////

void AccelerationStructure::destroyResources()
{

}

/////////////////////////////////////////////////////////////////////////////////////////////
