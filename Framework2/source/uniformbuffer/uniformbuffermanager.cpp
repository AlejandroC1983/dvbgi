/*
Copyright 2017 Alejandro Cosin

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
#include "../../include/uniformbuffer/uniformbuffermanager.h"
#include "../../include/uniformbuffer/uniformbuffer.h"
#include "../../include/core/coremanager.h"
#include "../../include/parameter/attributedefines.h"

// NAMESPACE
using namespace attributedefines;

// DEFINES

// STATIC MEMBER INITIALIZATION

/////////////////////////////////////////////////////////////////////////////////////////////

UniformBufferManager::UniformBufferManager()
{
	m_managerName = g_uniformBufferManager;
}

/////////////////////////////////////////////////////////////////////////////////////////////

UniformBufferManager::~UniformBufferManager()
{
	destroyResources();
}

/////////////////////////////////////////////////////////////////////////////////////////////

void UniformBufferManager::destroyResources()
{
	forIT(m_mapElement)
	{
		delete it->second;
		it->second = nullptr;
	}
}

/////////////////////////////////////////////////////////////////////////////////////////////

bool UniformBufferManager::resize(string&& instanceName, int cellSize, int numCells)
{
	UniformBuffer* uniformBuffer = getElement(move(string(instanceName)));

	if (uniformBuffer == nullptr)
	{
		return false;
	}

	uniformBuffer->m_ready       = false;
	uniformBuffer->destroyResources();
	uniformBuffer->setMinCellSize(cellSize);
	uniformBuffer->m_CPUBuffer.setNumCells(numCells);
	uniformBuffer->m_CPUBuffer.setMinCellSize(cellSize);
	buildUniformBufferResources(uniformBuffer);
	uniformBuffer->m_ready = true;

	if (gpuPipelineM->getPipelineInitialized())
	{
		emitSignalElement(move(instanceName), ManagerNotificationType::MNT_CHANGED);
	}

	return true;
}

/////////////////////////////////////////////////////////////////////////////////////////////

UniformBuffer* UniformBufferManager::buildUniformBuffer(string&& instanceName, int cellSize, int numCells)
{
	return buildUniformBuffer(move(instanceName), cellSize, numCells, false);
}

/////////////////////////////////////////////////////////////////////////////////////////////

UniformBuffer* UniformBufferManager::buildUniformBuffer(string&& instanceName, int cellSize, int numCells, bool usePersistentMapping)
{
	if (existsElement(move(instanceName)))
	{
		return getElement(move(instanceName));
	}

	UniformBuffer* uniformBuffer = new UniformBuffer(move(string(instanceName)));
	uniformBuffer->setMinCellSize(cellSize);
	uniformBuffer->m_CPUBuffer.setNumCells(numCells);
	uniformBuffer->m_CPUBuffer.setMinCellSize(cellSize);

	uniformBuffer->m_usePersistentMapping = usePersistentMapping;

	buildUniformBufferResources(uniformBuffer);

	addElement(move(string(instanceName)), uniformBuffer);
	uniformBuffer->m_name  = move(instanceName);
	uniformBuffer->m_ready = true;

	return uniformBuffer;
}

/////////////////////////////////////////////////////////////////////////////////////////////

void UniformBufferManager::buildUniformBufferResources(UniformBuffer* uniformBuffer)
{
	uniformBuffer->m_CPUBuffer.buildCPUBuffer();
	uniformBuffer->buildGPUBufer();
}

/////////////////////////////////////////////////////////////////////////////////////////////
