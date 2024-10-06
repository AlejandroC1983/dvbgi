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

#ifndef _BASECOMPONENT_H_
#define _BASECOMPONENT_H_

// GLOBAL INCLUDES

// PROJECT INCLUDES
#include "../../include/util/genericresource.h"

// CLASS FORWARDING

// NAMESPACE

// DEFINES

/////////////////////////////////////////////////////////////////////////////////////////////

class BaseComponent: public GenericResource
{
	friend class ComponentManager;

protected:
	/** Paremeter constructor
	* @param name         [in] component resource name
	* @param className    [in] component class name
	* @param resourceType [in] component resource type
	* @return nothing */
	BaseComponent(string&& name, string&& className, GenericResourceType resourceType);

public:
	/** Default destructor
	* @return nothing */
	virtual ~BaseComponent();

	/** Initialize the resource
	* @return nothing */
	virtual void init();
};

/////////////////////////////////////////////////////////////////////////////////////////////

#endif _BASECOMPONENT_H_
