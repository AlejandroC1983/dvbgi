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

#ifndef _RASTERTECHNIQUEMANAGER_H_
#define _RASTERTECHNIQUEMANAGER_H_

// GLOBAL INCLUDES

// PROJECT INCLUDES
#include "../../include/util/singleton.h"
#include "../../include/util/managertemplate.h"

// CLASS FORWARDING
class RasterTechnique;
class MultiTypeUnorderedMap;

// NAMESPACE

// DEFINES
#define rasterTechniqueM s_pRasterTechniqueManager->instance()

/////////////////////////////////////////////////////////////////////////////////////////////

class RasterTechniqueManager: public ManagerTemplate<RasterTechnique>, public Singleton<RasterTechniqueManager>
{
	friend class CoreManager;

public:
	/** Constructor
	* @return nothing */
	RasterTechniqueManager();

	/** Destructor
	* @return nothing */
	virtual ~RasterTechniqueManager();

	/** Builds a new raster technique, instancing a class of type className and with name instance Name
	* @param className    [in] name of the class to instance by this factory
	* @param instanceName [in] name of the new instance (the m_sName member variable)
	* @return if the built was successfull, a pointer to the raster technique is returned, nullptr is returned otherwise */
	RasterTechnique *buildNewRasterTechnique(string &&className, string &&instanceName, MultiTypeUnorderedMap* attributeData);

protected:
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

static RasterTechniqueManager* s_pRasterTechniqueManager;

/////////////////////////////////////////////////////////////////////////////////////////////

#endif _RASTERTECHNIQUEMANAGER_H_
