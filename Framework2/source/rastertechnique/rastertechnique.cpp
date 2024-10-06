/*
Copyright 2014 Alejandro Cosin

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
#include "../../include/util/loopmacrodefines.h"
#include "../../include/material/material.h"
#include "../../include/material/materialmanager.h"
#include "../../include/rastertechnique/rastertechnique.h"
#include "../../include/core/coremanager.h"
#include "../../include/shader/shader.h"
#include "../../include/shader/shadermanager.h"

// NAMESPACE

// DEFINES

// STATIC MEMBER INITIALIZATION

/////////////////////////////////////////////////////////////////////////////////////////////

RasterTechnique::RasterTechnique(string &&name, string&& className) : GenericResource(move(name), move(className), GenericResourceType::GRT_RASTERTECHNIQUE)
	, m_active(true)
	, m_needsToRecord(true)
	, m_executeCommand(true)
	, m_recordPolicy(CommandRecordPolicy::CRP_EVERY_SWAPCHAIN_IMAGE)
	, m_minExecutionTime(FLT_MAX)
	, m_maxExecutiontime(-1.0f * FLT_MAX)
	, m_lastExecutionTime(0.0f)
	, m_meanExecutionTime(0.0f)
	, m_accumulatedExecutionTime(0.0f)
	, m_numExecution(0.0f)
	, m_queryIndex0(UINT_MAX)
	, m_queryIndex1(UINT_MAX)
	, m_isLastPipelineTechnique(false)
	, m_usedCommandBufferNumber(1)
	, m_neededSemaphoreNumber(1)
	, m_rasterTechniqueType(RasterTechniqueType::RTT_GRAPHICS)
	, m_computeHostSynchronize(false)
{
	VkSemaphoreCreateInfo semaphoreCreateInfo;
	semaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
	semaphoreCreateInfo.pNext = NULL;
	semaphoreCreateInfo.flags = 0;
}

/////////////////////////////////////////////////////////////////////////////////////////////

RasterTechnique::~RasterTechnique()
{
	shutdown();
}

/////////////////////////////////////////////////////////////////////////////////////////////

void RasterTechnique::init()
{

}

/////////////////////////////////////////////////////////////////////////////////////////////

void RasterTechnique::generateSempahore()
{
	m_vectorSemaphore.resize(m_neededSemaphoreNumber);

	VkSemaphoreCreateInfo semaphoreCreateInfo;
	semaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
	semaphoreCreateInfo.pNext = NULL;
	semaphoreCreateInfo.flags = 0;

	forI(m_neededSemaphoreNumber)
	{
		vkCreateSemaphore(coreM->getLogicalDevice(), &semaphoreCreateInfo, NULL, &m_vectorSemaphore[i]);
	}
}

/////////////////////////////////////////////////////////////////////////////////////////////

void RasterTechnique::prepare(float dt)
{

}

/////////////////////////////////////////////////////////////////////////////////////////////

void RasterTechnique::preRecordLoop()
{
	m_executeCommand = true;
	m_needsToRecord  = (m_vectorCommand.size() != m_usedCommandBufferNumber);
}

/////////////////////////////////////////////////////////////////////////////////////////////

void RasterTechnique::updateMaterial()
{
	const uint maxIndex = uint(m_vectorMaterial.size());

	Material* material;
	Shader* shader;

	forI(maxIndex)
	{
		material = m_vectorMaterial[i];
		if (!material->getReady())
		{
			shader = shaderM->getElement(move(string(material->getShaderResourceName())));
			if (shader != nullptr)
			{
				material->destroyPipelineResource();
				material->refShader()->destroySamplers();
				material->clearExposedFieldData();
				material->buildMaterialResources();
				material->destroyDescriptorSetResource();
				material->destroyDescriptorLayout();
				material->destroyPipelineLayouts();
				material->destroyDescriptorSet();
				material->destroyDescriptorPool();
				material->buildPipeline(); // add some control in case of errors?
				material->setReady(true);
				m_vectorCommand.clear(); // Clear any recorded command buffer, new material makes it mandatory to record again
			}
		}

		material->updateExposedResources();
		if (material->getExposedStructFieldDirty())
		{
			material->writeExposedDataToMaterialUB();
			material->setExposedStructFieldDirty(false);
			materialM->updateGPUBufferMaterialData(material);
		}
	}
}

/////////////////////////////////////////////////////////////////////////////////////////////

void RasterTechnique::postCommandSubmit()
{

}

/////////////////////////////////////////////////////////////////////////////////////////////

VkCommandBuffer* RasterTechnique::record(int currentImage, uint& commandBufferID, CommandBufferType& commandBufferType)
{
	commandBufferID = 0;
	commandBufferType = CommandBufferType::CBT_SIZE;
	return nullptr;
}

/////////////////////////////////////////////////////////////////////////////////////////////

void RasterTechnique::shutdown()
{
	forI(m_neededSemaphoreNumber)
	{
		vkDestroySemaphore(coreM->getLogicalDevice(), m_vectorSemaphore[i], NULL);
	}

	m_vectorSemaphore.clear();

	destroyCommandBuffers();
}

/////////////////////////////////////////////////////////////////////////////////////////////

bool RasterTechnique::materialResourceNotification(string&& materialResourceName, ManagerNotificationType notificationType)
{
	bool result = false;
	const uint maxIndex = uint(m_vectorMaterialName.size());

	forI(maxIndex)
	{
		if (m_vectorMaterialName[i] == materialResourceName)
		{
			switch (notificationType)
			{
				case ManagerNotificationType::MNT_REMOVED:
				{
					Material* material = m_vectorMaterial[i];
					if ((material != nullptr) && (material->getName() == materialResourceName))
					{
						material = nullptr;
						setReady(false);
						result = true;
					}
					break;
				}
				case ManagerNotificationType::MNT_ADDED:
				{
					Material* material = materialM->getElement(move(string(materialResourceName)));
					if (material != nullptr)
					{
						m_vectorMaterial[i] = material;
						result = true;
					}
					break;
				}
				case ManagerNotificationType::MNT_CHANGED:
				{
					setReady(false);
					result = true;
					break;
				}
			}
		}
	}

	return result;
}

/////////////////////////////////////////////////////////////////////////////////////////////

VkCommandBuffer* RasterTechnique::addRecordedCommandBuffer(uint& commandBufferId)
{
	uint newId                    = coreM->getNextCommandBufferIndex();
	commandBufferId               = newId;
	VkCommandBuffer commandBuffer = VK_NULL_HANDLE;
	bool result                   = addIfNoPresent(move(newId), commandBuffer, m_mapIdCommandBuffer);

	if (!result)
	{
		cout << "ERROR in RasterTechnique::addRecordedCommandBuffer, addIfNoPresent returned false" << endl;
		return nullptr;
	}

	return &m_mapIdCommandBuffer[newId];
}

/////////////////////////////////////////////////////////////////////////////////////////////

bool RasterTechnique::addCommandBufferQueueType(uint commandBufferId, CommandBufferType commandBufferType)
{
	bool result = addIfNoPresent(move(uint(commandBufferId)), commandBufferType, m_mapUintCommandBufferType);

	if (!result)
	{
		cout << "ERROR in RasterTechnique::addCommandBufferQueueType, addIfNoPresent returned false" << endl;
	}
	
	return result;
}

/////////////////////////////////////////////////////////////////////////////////////////////

void RasterTechnique::destroyCommandBuffers()
{
	mapUintCommandBuffer::const_iterator it;
	for (it = m_mapIdCommandBuffer.begin(); it != m_mapIdCommandBuffer.end(); ++it)
	{
		vkFreeCommandBuffers(coreM->getLogicalDevice(), (m_rasterTechniqueType == RasterTechniqueType::RTT_GRAPHICS) ? coreM->getGraphicsCommandPool() : coreM->getComputeCommandPool(), 1, &it->second);
	}

	m_mapIdCommandBuffer.clear();
}

/////////////////////////////////////////////////////////////////////////////////////////////

void RasterTechnique::addExecutionTime()
{
#ifndef USE_TIMESTAMP
	return;
#endif

	if ((m_queryIndex0 == UINT_MAX) || (m_queryIndex1 == UINT_MAX))
	{
		return;
	}

	uint32_t startRecordData;
	uint32_t endRecordData;

	if (m_mapUintCommandBufferType.rbegin()->second == CommandBufferType::CBT_GRAPHICS_QUEUE)
	{
		vkGetQueryPoolResults(coreM->getLogicalDevice(), coreM->getGraphicsQueueQueryPool(), m_queryIndex0, 1, sizeof(uint32_t), &startRecordData, 0, VK_QUERY_RESULT_WAIT_BIT);
		vkGetQueryPoolResults(coreM->getLogicalDevice(), coreM->getGraphicsQueueQueryPool(), m_queryIndex1, 1, sizeof(uint32_t), &endRecordData,   0, VK_QUERY_RESULT_WAIT_BIT);
	}
	else
	{
		vkGetQueryPoolResults(coreM->getLogicalDevice(), coreM->getComputeQueueQueryPool(), m_queryIndex0, 1, sizeof(uint32_t), &startRecordData, 0, VK_QUERY_RESULT_WAIT_BIT);
		vkGetQueryPoolResults(coreM->getLogicalDevice(), coreM->getComputeQueueQueryPool(), m_queryIndex1, 1, sizeof(uint32_t), &endRecordData,   0, VK_QUERY_RESULT_WAIT_BIT);
	}

	float executionTime         = float(endRecordData - startRecordData) / (float)1e6;
	m_minExecutionTime          = std::min(m_minExecutionTime, executionTime);
	m_maxExecutiontime          = std::max(m_minExecutionTime, executionTime);
	m_lastExecutionTime         = executionTime;
	m_accumulatedExecutionTime += executionTime;
	m_meanExecutionTime         = (m_numExecution * m_meanExecutionTime + executionTime) / (m_numExecution + 1.0f);
	m_numExecution             += 1.0f;
}

/////////////////////////////////////////////////////////////////////////////////////////////
