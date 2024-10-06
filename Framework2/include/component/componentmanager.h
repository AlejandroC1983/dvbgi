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

#ifndef _COMPONENTMANAGER_H_
#define _COMPONENTMANAGER_H_

// GLOBAL INCLUDES

// PROJECT INCLUDES
#include "../../include/util/singleton.h"
#include "../../include/util/managertemplate.h"
#include "../headers.h"

// CLASS FORWARDING
class BaseComponent;

// NAMESPACE

// DEFINES
#define componentM s_pComponentManager->instance()

/////////////////////////////////////////////////////////////////////////////////////////////

class ComponentManager: public ManagerTemplate<BaseComponent>, public Singleton<ComponentManager>
{
public:
	/** Constructor
	* @param name [in] manager name
	* @return nothing */
	ComponentManager();

	/** Destructor
	* @return nothing */
	virtual ~ComponentManager();

	/** Builds a new framebuffer, a pointer to the framebuffer is returned, nullptr is returned if any errors while building it
	* @param instanceName [in] name of the new instance (the m_sName member variable)
	* @param resourceType [in] type of component to build
	* @return a pointer to the built component, nullptr otherwise */
	BaseComponent* buildComponent(string&& instanceName, GenericResourceType resourceType);

	/** Destroys all elements in the manager
	* @return nothing */
	void destroyResources();
};

static ComponentManager* s_pComponentManager;

/////////////////////////////////////////////////////////////////////////////////////////////

#endif _COMPONENTMANAGER_H_
