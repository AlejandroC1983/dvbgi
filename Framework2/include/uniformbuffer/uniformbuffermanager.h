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

#ifndef _UNIFORMBUFFERMANAGER_H_
#define _UNIFORMBUFFERMANAGER_H_

// GLOBAL INCLUDES

// PROJECT INCLUDES
#include "../../include/util/singleton.h"
#include "../../include/util/managertemplate.h"
#include "../headers.h"

// CLASS FORWARDING
class MultiTypeUnorderedMap;
class UniformBuffer;

// NAMESPACE

// DEFINES
#define uniformBufferM s_pUniformBufferManager->instance()

/////////////////////////////////////////////////////////////////////////////////////////////

class UniformBufferManager: public ManagerTemplate<UniformBuffer>, public Singleton<UniformBufferManager>
{
	friend class CoreManager;

public:
	/** Constructor
	* @return nothing */
	UniformBufferManager();

	/** Destructor
	* @return nothing */
	virtual ~UniformBufferManager();

	/** Builds a new framebuffer, a pointer to the framebuffer is returned, nullptr is returned if any errors while building it
	* @param instanceName [in] name of the new instance (the m_sName member variable)
	* @param cellSize     [in] size of each cell in the buffer in bytes
	* @param numCells     [in] number of cells in the buffer
	* @return a pointer to the built uniform buffer, nullptr otherwise */
	UniformBuffer* buildUniformBuffer(string&& instanceName, int cellSize, int numCells);

	/** Builds a new framebuffer, a pointer to the framebuffer is returned, nullptr is returned if any errors while building it
	* @param instanceName         [in] name of the new instance (the m_sName member variable)
	* @param cellSize             [in] size of each cell in the buffer in bytes
	* @param numCells             [in] number of cells in the buffer
	* @param usePersistentMapping [in] flag to use persistent mapping for the UniformBuffer::m_bufferInstance buffer (not fully suppported, just for host visible buffers that need their content to be updated towards the device side)
	* @return a pointer to the built uniform buffer, nullptr otherwise */
	UniformBuffer* buildUniformBuffer(string&& instanceName, int cellSize, int numCells, bool usePersistentMapping);

	/** Destroys all elements in the manager
	* @return nothing */
	void destroyResources();

	/** Resizes the uniform buffer with name given by instanceName if present in the manager's resources
	* @param instanceName [in] name of the instance to resize
	* @param cellSize     [in] size of each cell in the buffer in bytes
	* @param numCells     [in] number of cells in the buffer
	* @return true if thre resize operation was performed successfully, false otherwise */
	bool resize(string&& instanceName, int cellSize, int numCells);

protected:
	/** Builds the resources for the UniformBuffer given as parameter
	* @param uniformBuffer [in] uniform buffer to build resources for
	* @return nothing */
	void buildUniformBufferResources(UniformBuffer* uniformBuffer);
};

static UniformBufferManager* s_pUniformBufferManager;

/////////////////////////////////////////////////////////////////////////////////////////////

#endif _UNIFORMBUFFERMANAGER_H_
