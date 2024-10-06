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

#ifndef _ACCELERATIONSTRUCTUREMANAGER_H_
#define _ACCELERATIONSTRUCTUREMANAGER_H_

// GLOBAL INCLUDES

// PROJECT INCLUDES
#include "../../include/util/singleton.h"
#include "../../include/util/managertemplate.h"
#include "../../include/headers.h"

// CLASS FORWARDING
class AccelerationStructure;

// NAMESPACE

// DEFINES
#define accelerationStructureM s_pAccelerationStructureManager->instance()

/////////////////////////////////////////////////////////////////////////////////////////////

class AccelerationStructureManager: public ManagerTemplate<AccelerationStructure>, public Singleton<AccelerationStructureManager>
{
	friend class CoreManager;

public:
	/** Constructor
	* @param name [in] manager name
	* @return nothing */
	AccelerationStructureManager();

	/** Destructor
	* @return nothing */
	virtual ~AccelerationStructureManager();

	/** Destroys all elements in the manager
	* @return nothing */
	void destroyResources();

	/** Builds a new acceleration structure, a pointer to the acceleration structure is returned, nullptr is returned if
	* any errors while building it
	* @param instanceName [in] name of the new instance (the m_sName member variable)
	* @param vectorNode   [in] vector with the nodes that will form this acceleration structure
	* @return true if the acceleration strcuture was built properly, false otherwise */
	AccelerationStructure* buildAccelerationStructure(string&& instanceName, vectorNodePtr&& vectorNode);

protected:
	/** Builds all the resources used by the acceleration structure given by parameter
	* @param accelerationStructure [in] acceleration structure to build resources for
	* @return true if the acceleration structure resources were built properly, false otherwise */
	bool buildAccelerationStructureResources(AccelerationStructure* accelerationStructure);

	// TODO: use crtp to avoid this virtual method call
	/** Assigns the corresponding slots to listen to signals affecting the resources
	* managed by this manager */
	virtual void assignSlots();

	// TODO: use crtp to avoid this virtual method call
	/** Slot for managing added, elements signal
	* @param managerName      [in] name of the manager performing the notification
	* @param elementName      [in] name of the element added
	* @param notificationType [in] enum describing the type of notification
	* @return nothing */
	virtual void slotElement(const char* managerName, string&& elementName, ManagerNotificationType notificationType);
};

static AccelerationStructureManager* s_pAccelerationStructureManager;

/////////////////////////////////////////////////////////////////////////////////////////////

#endif _ACCELERATIONSTRUCTUREMANAGER_H_
