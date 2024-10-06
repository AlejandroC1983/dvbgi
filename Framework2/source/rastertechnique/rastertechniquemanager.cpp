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
#include "../../include/rastertechnique/rastertechniquemanager.h"
#include "../../include/rastertechnique/rastertechniquefactory.h"
#include "../../include/rastertechnique/rastertechnique.h"
#include "../../include/parameter/attributedefines.h"
#include "../../include/material/material.h"
#include "../../include/material/materialmanager.h"
#include "../../include/core/coremanager.h"

// NAMESPACE

// DEFINES
using namespace attributedefines;

// STATIC MEMBER INITIALIZATION

/////////////////////////////////////////////////////////////////////////////////////////////

RasterTechniqueManager::RasterTechniqueManager()
{
	m_managerName = g_rasterTechniqueManager;
}

/////////////////////////////////////////////////////////////////////////////////////////////

RasterTechniqueManager::~RasterTechniqueManager()
{
	forIT(m_mapElement)
	{
		delete it->second;
		it->second = nullptr;
	}
}

/////////////////////////////////////////////////////////////////////////////////////////////

RasterTechnique *RasterTechniqueManager::buildNewRasterTechnique(string &&className, string &&instanceName, MultiTypeUnorderedMap* attributeData)
{
	if (existsElement(move(instanceName)))
	{
		return getElement(move(instanceName));
	}

	RasterTechnique *result = RasterTechniqueFactory::buildElement(move(className));

	if (result != nullptr)
	{
		result->setName(move(string(instanceName)));
		RasterTechniqueManager::addElement(move(instanceName), result);
		result->setParameterData(attributeData);
		result->init();
	}

	return result;
}

/////////////////////////////////////////////////////////////////////////////////////////////

void RasterTechniqueManager::assignSlots()
{
	materialM->refElementSignal().connect<RasterTechniqueManager, &RasterTechniqueManager::slotElement>(this);
}

/////////////////////////////////////////////////////////////////////////////////////////////

void RasterTechniqueManager::slotElement(const char* managerName, string&& elementName, ManagerNotificationType notificationType)
{
	if (!gpuPipelineM->getPipelineInitialized())
	{
		return;
	}

	if (managerName == g_materialManager)
	{
		map<string, RasterTechnique*>::iterator it = m_mapElement.begin();
		for (it; it != m_mapElement.end(); ++it)
		{
			if (it->second->materialResourceNotification(move(string(elementName)), notificationType))
			{
				if (gpuPipelineM->getPipelineInitialized())
				{
					emitSignalElement(move(string(elementName)), notificationType);
				}
			}
		}
	}
}

/////////////////////////////////////////////////////////////////////////////////////////////
