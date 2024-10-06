/*
Copyright 2018 Alejandro Cosin

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

#ifndef _RENDERPASSMANAGER_H_
#define _RENDERPASSMANAGER_H_

// GLOBAL INCLUDES

// PROJECT INCLUDES
#include "../../include/util/singleton.h"
#include "../../include/util/managertemplate.h"

// CLASS FORWARDING
class MultiTypeUnorderedMap;
class RenderPass;

// NAMESPACE

// DEFINES
#define renderPassM s_pRenderPassManager->instance()

/////////////////////////////////////////////////////////////////////////////////////////////

class RenderPassManager: public ManagerTemplate<RenderPass>, public Singleton<RenderPassManager>
{
	friend class CoreManager;

public:
	/** Constructor
	* @return nothing */
	RenderPassManager();

	/** Destructor
	* @return nothing */
	virtual ~RenderPassManager();

	/** Builds a new render pass, a pointer to the render pass is returned, nullptr is returned if any errors while building it
	* @param instanceName  [in] name of the new instance (the m_sName member variable)
	* @param attributeData [in] attributes to configure the newly built render pass
	* @return a pointer to the built render pass, nullptr otherwise */
	RenderPass* buildRenderPass(string&& instanceName, MultiTypeUnorderedMap* attributeData);

	/** Destroys all elements in the manager
	* @return nothing */
	void destroyResources();
};

static RenderPassManager *s_pRenderPassManager;

/////////////////////////////////////////////////////////////////////////////////////////////

#endif _RENDERPASSMANAGER_H_
