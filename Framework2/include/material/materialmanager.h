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

#ifndef _MATERIALMANAGER_H_
#define _MATERIALMANAGER_H_

// GLOBAL INCLUDES

// PROJECT INCLUDES
#include "../../include/headers.h"
#include "../../include/util/singleton.h"
#include "../../include/util/managertemplate.h"

// CLASS FORWARDING
class MultiTypeUnorderedMap;
class Material;
class UniformBuffer;

// NAMESPACE

// DEFINES
#define materialM s_pMaterialManager->instance()

/////////////////////////////////////////////////////////////////////////////////////////////

class MaterialManager: public ManagerTemplate<Material>, public Singleton<MaterialManager>
{
	friend class CoreManager;

public:
	/** Constructor
	* @return nothing */
	MaterialManager();

	/** Destructor
	* @return nothing */
	virtual ~MaterialManager();

	/** Builds a new framebuffer, a pointer to the framebuffer is returned, nullptr is returned if any errors while building it
	* @param className     [in] name of the class of the material to instantiate
	* @param instanceName  [in] name of the new instance (the m_sName member variable)
	* @param attributeData [in] attributes to configure the newly built pipeline
	* @return a pointer to the built framebuffer, nullptr otherwise */
	Material* buildMaterial(string&& className, string&& instanceName, MultiTypeUnorderedMap* attributeData);

	/** Destroys all elements in the manager
	* @return nothing */
	void destroyResources();

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

	/** Builds an uniform buffer which will hold the data for all the materials used
	* @return nothing */
	void buildMaterialUniformBuffer();

	/** Update the GPU buffer m_materialUniformData memory region given by the material parameter
	* @param materialToUpdate [in] material to update
	* @return nothing */
	void updateGPUBufferMaterialData(Material* materialToUpdate);

	/** Init all materials present in m_mapElement
	* @return nothing */
	void initAllMaterials();

	REF_PTR(UniformBuffer, m_materialUniformData, MaterialUniformData)
	GETCOPY(uint, m_materialUBDynamicAllignment, MaterialUBDynamicAllignment)

protected:
	UniformBuffer* m_materialUniformData;         //!< Uniform buffer with information for each one of the materials
	uint           m_materialUBDynamicAllignment; //!< Value of UniformBuffer::m_dynamicAllignment for m_materialUniformData
	bool           m_materialsInitialized;        //!< Flafg to know whether the call to initAllMaterials was done
};

static MaterialManager* s_pMaterialManager;

/////////////////////////////////////////////////////////////////////////////////////////////

#endif _MATERIALMANAGER_H_
