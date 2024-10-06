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
#include "../../include/accelerationstructure/accelerationstructuremanager.h"
#include "../../include/accelerationstructure/accelerationstructure.h"
#include "../../include/parameter/attributedefines.h"
#include "../../include/core/coremanager.h"

// NAMESPACE
using namespace attributedefines;

// DEFINES

// STATIC MEMBER INITIALIZATION

/////////////////////////////////////////////////////////////////////////////////////////////

AccelerationStructureManager::AccelerationStructureManager()
{
	m_managerName = g_accelerationStructureManager;
}

/////////////////////////////////////////////////////////////////////////////////////////////

AccelerationStructureManager::~AccelerationStructureManager()
{
	destroyResources();
}

/////////////////////////////////////////////////////////////////////////////////////////////

void AccelerationStructureManager::destroyResources()
{
	forIT(m_mapElement)
	{
		delete it->second;
		it->second = nullptr;
	}
}

/////////////////////////////////////////////////////////////////////////////////////////////

AccelerationStructure* AccelerationStructureManager::buildAccelerationStructure(string&& instanceName, vectorNodePtr&& vectorNode)
{
	if (existsElement(move(string(instanceName))))
	{
		return getElement(move(string(instanceName)));
	}

	AccelerationStructure* accelerationStructure = new AccelerationStructure(move(string(instanceName)), move(vectorNode));
	accelerationStructure->setReady(true);
	bool result = buildAccelerationStructureResources(accelerationStructure);

	coreM->setObjectName(uint64_t(accelerationStructure->m_tlas), VK_DEBUG_REPORT_OBJECT_TYPE_ACCELERATION_STRUCTURE_KHR_EXT, accelerationStructure->getName());
	
	addElement(move(instanceName), accelerationStructure);
	return accelerationStructure;
}

/////////////////////////////////////////////////////////////////////////////////////////////

bool AccelerationStructureManager::buildAccelerationStructureResources(AccelerationStructure* accelerationStructure)
{
	// TODO: BUILD THE SCENE DESC BUFFER

	accelerationStructure->buildBLAS();
	accelerationStructure->buildTLAS(false, nullptr);
	AccelerationStructure::m_instanceCounter++;

	return true;
}

/////////////////////////////////////////////////////////////////////////////////////////////

void AccelerationStructureManager::assignSlots()
{

}

/////////////////////////////////////////////////////////////////////////////////////////////

void AccelerationStructureManager::slotElement(const char* managerName, string&& elementName, ManagerNotificationType notificationType)
{

}

/////////////////////////////////////////////////////////////////////////////////////////////
