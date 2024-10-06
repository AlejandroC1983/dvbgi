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
#include "../../include/component/componentmanager.h"
#include "../../include/component/basecomponent.h"
#include "../../include/component/rendercomponent.h"
#include "../../include/component/dynamiccomponent.h"
#include "../../include/component/skinnedmeshrendercomponent.h"
#include "../../include/component/sceneelementcontrolcomponent.h"
#include "../../include/parameter/attributedefines.h"

// NAMESPACE
using namespace attributedefines;

// DEFINES

// STATIC MEMBER INITIALIZATION

/////////////////////////////////////////////////////////////////////////////////////////////

ComponentManager::ComponentManager()
{
	m_managerName = g_componentManager;
}

/////////////////////////////////////////////////////////////////////////////////////////////

ComponentManager::~ComponentManager()
{
	destroyResources();
}

/////////////////////////////////////////////////////////////////////////////////////////////

BaseComponent* ComponentManager::buildComponent(string&& instanceName, GenericResourceType resourceType)
{
	if (existsElement(move(string(instanceName))))
	{
		return getElement(move(instanceName));
	}

	BaseComponent* newComponent = nullptr;
	
	switch (resourceType)
	{
		case GenericResourceType::GRT_RENDERCOMPONENT:
		{
			newComponent                 = new RenderComponent(move(string(instanceName)), move(string("RenderComponent")), resourceType);
			newComponent->m_resourceType = GenericResourceType::GRT_RENDERCOMPONENT;
			break;
		}
		case GenericResourceType::GRT_DYNAMICCOMPONENT:
		{
			newComponent                 = new DynamicComponent(move(string(instanceName)));
			newComponent->m_resourceType = GenericResourceType::GRT_DYNAMICCOMPONENT;
			break;
		}
		case GenericResourceType::GRT_SCENEELEMENTCONTROLCOMPONENT:
		{
			newComponent                 = new SceneElementControlComponent(move(string(instanceName)));
			newComponent->m_resourceType = GenericResourceType::GRT_SCENEELEMENTCONTROLCOMPONENT;
			break;
		}
		case GenericResourceType::GRT_SKINNEDMESHRENDERCOMPONENT:
		{
			newComponent                 = new SkinnedMeshRenderComponent(move(string(instanceName)), move(string("SkinnedMeshRenderComponent")), resourceType);
			newComponent->m_resourceType = GenericResourceType::GRT_SKINNEDMESHRENDERCOMPONENT;
			break;
		}
	}

	addElement(move(instanceName), newComponent);

	return newComponent;
}

/////////////////////////////////////////////////////////////////////////////////////////////

void ComponentManager::destroyResources()
{
	forIT(m_mapElement)
	{
		delete it->second;
		it->second = nullptr;
	}
}

/////////////////////////////////////////////////////////////////////////////////////////////
