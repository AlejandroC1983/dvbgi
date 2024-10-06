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
#include "../../include/rastertechnique/resetlitvoxeldatatechnique.h"
#include "../../include/material/materialmanager.h"
#include "../../include/core/gpupipeline.h"
#include "../../include/scene/scene.h"
#include "../../include/rastertechnique/staticscenevoxelizationtechnique.h"
#include "../../include/camera/camera.h"
#include "../../include/camera/cameramanager.h"
#include "../../include/rastertechnique/bufferprefixsumtechnique.h"
#include "../../include/buffer/buffermanager.h"
#include "../../include/util/vulkanstructinitializer.h"
#include "../../include/buffer/buffer.h"
#include "../../include/buffer/buffermanager.h"

// NAMESPACE

// DEFINES

// STATIC MEMBER INITIALIZATION

/////////////////////////////////////////////////////////////////////////////////////////////

ResetLitvoxelData::ResetLitvoxelData(string &&name, string&& className) : RasterTechnique(move(name), move(className))
	, m_litTestVoxelPerByteBuffer(nullptr)
	, m_litTestDynamicVoxelPerByteBuffer(nullptr)
	, m_cameraVisibleVoxelPerByteBuffer(nullptr)
	, m_cameraVisibleDynamicVoxelPerByteBuffer(nullptr)
	, m_cameraVisibleDynamicVoxelPerByteTagProcessBuffer(nullptr)
	, m_resetLitVoxelDataDebugBuffer(nullptr)
	, m_irradiancePaddingTagTilesBuffer(nullptr)
	, m_irradianceFilteringTagTilesBuffer(nullptr)
	, m_dynamicVoxelVisibilityBuffer(nullptr)
	, m_bufferPrefixSumTechnique(nullptr)
	, m_prefixSumCompleted(false)
	, m_techniqueLock(false)
	, m_voxelizationWidth(0)
	, m_voxelOccupiedDynamicBuffer(nullptr)
	, m_mainCamera(nullptr)
	, m_emitterCamera(nullptr)
{
	m_active                 = false;
	m_needsToRecord          = false;
	m_executeCommand         = false;
	m_rasterTechniqueType    = RasterTechniqueType::RTT_GRAPHICS;
	m_computeHostSynchronize = true;
}

/////////////////////////////////////////////////////////////////////////////////////////////

void ResetLitvoxelData::init()
{
	StaticSceneVoxelizationTechnique* technique = static_cast<StaticSceneVoxelizationTechnique*>(gpuPipelineM->getRasterTechniqueByName(move(string("StaticSceneVoxelizationTechnique"))));
	m_voxelizationWidth = technique->getVoxelizedSceneWidth();

	VkDeviceSize bufferSize = (m_voxelizationWidth * m_voxelizationWidth * m_voxelizationWidth);

	m_cameraVisibleDynamicVoxelPerByteBuffer = bufferM->buildBuffer(
		move(string("cameraVisibleDynamicVoxelPerByteBuffer")),
		nullptr,
		bufferSize, // One bit per voxelization volume element
		VK_BUFFER_USAGE_STORAGE_BUFFER_BIT  | VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

	m_cameraVisibleDynamicVoxelPerByteTagProcessBuffer = bufferM->buildBuffer(
		move(string("cameraVisibleDynamicVoxelPerByteTagProcessBuffer")),
		nullptr,
		bufferSize, // One bit per voxelization volume element
		VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

	m_resetLitVoxelDataDebugBuffer = bufferM->buildBuffer(
		move(string("resetLitVoxelDataDebugBuffer")),
		nullptr,
		819200,
		VK_BUFFER_USAGE_STORAGE_BUFFER_BIT  | VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

	// For the buffes below, one byte is needed per voxel, bits {0,1,2,3,4,5} are
	// used to tag as lit specific dynamic voxel faces as lit, in particular{ -x,+x,-y,+y,-z,+z }. Bit 6 is to tag
	// whether thre is any lit voxel face, the last bit, 7th, is unused.

	m_litTestVoxelPerByteBuffer = bufferM->buildBuffer(
		move(string("litTestVoxelPerByteBuffer")),
		nullptr,
		bufferSize, // One byte per voxelization volume element
		VK_BUFFER_USAGE_STORAGE_BUFFER_BIT  | VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

	m_litTestDynamicVoxelPerByteBuffer = bufferM->buildBuffer(
		move(string("litTestDynamicVoxelPerByteBuffer")),
		nullptr,
		bufferSize, // One byte per voxelization volume element
		VK_BUFFER_USAGE_STORAGE_BUFFER_BIT  | VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

	vector<uint> vectorVoxelOccupied;
	vectorVoxelOccupied.resize(m_voxelizationWidth * m_voxelizationWidth * m_voxelizationWidth / 8);
	memset(vectorVoxelOccupied.data(), 0, vectorVoxelOccupied.size() * size_t(sizeof(uint)));

	m_voxelOccupiedDynamicBuffer = bufferM->buildBuffer(
		move(string("voxelOccupiedDynamicBuffer")),
		vectorVoxelOccupied.data(),
		vectorVoxelOccupied.size(),
		VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

	m_cameraVisibleVoxelPerByteBuffer = bufferM->buildBuffer(
		move(string("cameraVisibleVoxelPerByteBuffer")),
		nullptr,
		bufferSize, // One bit per voxelization volume element
		VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

	bufferSize = (m_voxelizationWidth * m_voxelizationWidth * m_voxelizationWidth) / 8; // 1 byte per tile, each tile has size 2^3 voxels

	m_irradiancePaddingTagTilesBuffer = bufferM->buildBuffer(
		move(string("irradiancePaddingTagTilesBuffer")),
		nullptr,
		bufferSize,
		VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

	m_irradianceFilteringTagTilesBuffer = bufferM->buildBuffer(
		move(string("irradianceFilteringTagTilesBuffer")),
		nullptr,
		bufferSize,
		VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

	m_dynamicVoxelVisibilityBuffer = bufferM->getElement(move(string("dynamicVoxelVisibilityBuffer")));

	m_bufferPrefixSumTechnique = static_cast<BufferPrefixSumTechnique*>(gpuPipelineM->getRasterTechniqueByName(move(string("BufferPrefixSumTechnique"))));
	m_bufferPrefixSumTechnique->refPrefixSumComplete().connect<ResetLitvoxelData, &ResetLitvoxelData::slotPrefixSumComplete>(this);

	sceneM->refSignalSceneDirtyNotification().connect<ResetLitvoxelData, &ResetLitvoxelData::slotSceneDirty>(this);

	m_mainCamera = cameraM->getElement(move(string("maincamera")));
	m_mainCamera->refCameraDirtySignal().connect<ResetLitvoxelData, &ResetLitvoxelData::slotCameraDirty>(this);

	m_emitterCamera = cameraM->getElement(move(string("emitter")));
	m_emitterCamera->refCameraDirtySignal().connect<ResetLitvoxelData, &ResetLitvoxelData::slotCameraDirty>(this);
}

/////////////////////////////////////////////////////////////////////////////////////////////

VkCommandBuffer* ResetLitvoxelData::record(int currentImage, uint& commandBufferID, CommandBufferType& commandBufferType)
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
	vkCmdWriteTimestamp(*commandBuffer, VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT, coreM->getGraphicsQueueQueryPool(), m_queryIndex0);
#endif

	vkCmdFillBuffer(*commandBuffer, m_cameraVisibleDynamicVoxelPerByteBuffer->getBuffer(),           0, m_cameraVisibleDynamicVoxelPerByteBuffer->getMappingSize(),                    0);
	vkCmdFillBuffer(*commandBuffer, m_cameraVisibleDynamicVoxelPerByteTagProcessBuffer->getBuffer(), 0, m_cameraVisibleDynamicVoxelPerByteTagProcessBuffer->getMappingSize(),          0);
	vkCmdFillBuffer(*commandBuffer, m_litTestVoxelPerByteBuffer->getBuffer(),                        0, m_litTestVoxelPerByteBuffer->getMappingSize(),                                 0);
	vkCmdFillBuffer(*commandBuffer, m_litTestDynamicVoxelPerByteBuffer->getBuffer(),                 0, m_litTestDynamicVoxelPerByteBuffer->getMappingSize(),                          0);
	vkCmdFillBuffer(*commandBuffer, m_voxelOccupiedDynamicBuffer->getBuffer(),                       0, m_voxelOccupiedDynamicBuffer->getMappingSize(),                                0);
	vkCmdFillBuffer(*commandBuffer, m_cameraVisibleVoxelPerByteBuffer->getBuffer(),                  0, m_cameraVisibleVoxelPerByteBuffer->getMappingSize(),                           0);
	vkCmdFillBuffer(*commandBuffer, m_irradiancePaddingTagTilesBuffer->getBuffer(),                  0, m_irradiancePaddingTagTilesBuffer->getMappingSize(),                           0);
	vkCmdFillBuffer(*commandBuffer, m_irradianceFilteringTagTilesBuffer->getBuffer(),                0, m_irradianceFilteringTagTilesBuffer->getMappingSize(),                         0);
	vkCmdFillBuffer(*commandBuffer, m_dynamicVoxelVisibilityBuffer->getBuffer(),                     0, m_dynamicVoxelVisibilityBuffer->getMappingSize(),                     4294967295);
	
#ifdef USE_TIMESTAMP
	vkCmdWriteTimestamp(*commandBuffer, VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT, coreM->getGraphicsQueueQueryPool(), m_queryIndex1);
#endif

	coreM->endCommandBuffer(*commandBuffer);
	
	m_vectorCommand.push_back(commandBuffer);

	return commandBuffer;
}

/////////////////////////////////////////////////////////////////////////////////////////////

void ResetLitvoxelData::postCommandSubmit()
{
	m_executeCommand = false;
	m_active         = false;
	m_needsToRecord  = false;

	m_signalResetLitvoxelDataCompletion.emit();
}

/////////////////////////////////////////////////////////////////////////////////////////////

void ResetLitvoxelData::slotPrefixSumComplete()
{
	m_prefixSumCompleted = true;
	m_active             = true;
	m_needsToRecord      = false;
}

/////////////////////////////////////////////////////////////////////////////////////////////

void ResetLitvoxelData::slotCameraDirty()
{
	slotDirtyUpdateValues();
}

/////////////////////////////////////////////////////////////////////////////////////////////

void ResetLitvoxelData::slotSceneDirty()
{
	slotDirtyUpdateValues();
}

/////////////////////////////////////////////////////////////////////////////////////////////

void ResetLitvoxelData::slotDirtyUpdateValues()
{
	if(m_techniqueLock)
	{
		return;
	}
	// This callback is for the shadow map camera, execute as well if the scene is dirty (ideally, if any dynamic scene element
	// can affect the irradiance in camera, maybe just in case any dynamic scene element is in camera or quite close to the frustum?
	if (m_prefixSumCompleted)
	{
		m_techniqueLock  = true;
		m_active         = true;
		m_executeCommand = true;
	}
}

/////////////////////////////////////////////////////////////////////////////////////////////
