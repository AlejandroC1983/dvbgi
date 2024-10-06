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

#ifndef _FRAMEBUFFERMANAGER_H_
#define _FRAMEBUFFERMANAGER_H_

// GLOBAL INCLUDES

// PROJECT INCLUDES
#include "../../include/util/singleton.h"
#include "../../include/util/managertemplate.h"
#include "../headers.h"

// CLASS FORWARDING
class Framebuffer;

// NAMESPACE

// DEFINES
#define framebufferM s_pFramebufferManager->instance()

/////////////////////////////////////////////////////////////////////////////////////////////

class FramebufferManager: public ManagerTemplate<Framebuffer>, public Singleton<FramebufferManager>
{
	friend class CoreManager;

public:
	/** Constructor
	* @param name [in] manager name
	* @return nothing */
	FramebufferManager();

	/** Destructor
	* @return nothing */
	virtual ~FramebufferManager();

	/** Builds a new framebuffer, a pointer to the framebuffer is returned, nullptr is returned if any errors while building it
	* @param instanceName        [in] name of the new instance (the m_sName member variable)
	* @param width               [in] framebuffer width
	* @param height              [in] framebuffer height
	* @param renderPass          [in] name of the render pass to use to build this framebuffer
	* @param arrayAttachmentName [in] vector with the names of the attachments for this framebuffer
	* @return a pointer to the built framebuffer, nullptr otherwise */
	Framebuffer* buildFramebuffer(string&& instanceName, uint32_t width, uint32_t height, string&& renderPassName, vector<string>&& m_arrayAttachmentName);

	/** Destroys all elements in the manager
	* @return nothing */
	void destroyResources();

protected:
	/** Verifies if the resources needed to build the framebuffer are available
	*@param framebuffer[in] framebuffer to verify
	* @return true if the resources are available, false otherwise */
	bool verifyCanBeBuilt(Framebuffer* framebuffer);

	/** Builds the actual framebuffer as a resource
	* @param framebuffer [in] framebuffer to build
	* @return true if the resource was built successfully, false otherwise */
	bool buildFramebufferResources(Framebuffer* framebuffer);

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

	/** Computes which framebuffers are affected by notifications from the shader and render pass managers,
	returning them in a vector
	* @param managerName      [in] name of the manager performing the notification
	* @param elementName      [in] name of the element added
	* @param notificationType [in] enum describing the type of notification
	* @return affected framebuffers */
	vector<Framebuffer*> computeNotifyAffectedElements(const char* managerName, string&& elementName, ManagerNotificationType notificationType);
};

static FramebufferManager* s_pFramebufferManager;

/////////////////////////////////////////////////////////////////////////////////////////////

#endif _FRAMEBUFFERMANAGER_H_
