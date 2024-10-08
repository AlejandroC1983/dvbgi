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

#ifndef _ATOMICCOUNTERUNIT_H_
#define _ATOMICCOUNTERUNIT_H_

// GLOBAL INCLUDES

// PROJECT INCLUDES
#include "../headers.h"
#include "../../include/util/getsetmacros.h"
#include "../../include/shader/resourceenum.h"
#include "../../include/commonnamespace.h"

// CLASS FORWARDING
class AtomicCounter;

// NAMESPACE
using namespace commonnamespace;
using namespace resourceenum;

// DEFINES

/////////////////////////////////////////////////////////////////////////////////////////////

/**
* Represents the atomic counter unit that can be used in a shader to bind atomic counter elements
* TODO: REFACTOR IMAGE, SAMPLER AND ATOMICCOUNTERUNIT CLASSES
*/

class AtomicCounterUnit
{
public:
	/** Parameter constructor
	* @param id                [in] id of the atomic counter
	* @param name              [in] atomic counter name
	* @param atomicCounterType [in] atomic counter type
	* @param shaderStage       [in] enumerated with value the shader stage the uniform being built is used
	* @return nothing */
	AtomicCounterUnit(const int id, string &&name, const ResourceInternalType atomicCounterType, VkShaderStageFlagBits shaderStage);

	/** Sets the atomic counter given as parameter as the one to be bound with this atomic counter unit
	* @param atomicCounter [in] atomic counter to set to this atomic counter unit
	* @return true if the atomic counter was successfully set as the one used by this atomic counter unit, and false otherwise */
	bool setAtomicCounter(AtomicCounter *atomicCounter);

	/** Sets the atomic counter name given as parameter as the one to be used with this atomic counter unit, asking the atomic counter
	* manager for it. If it's not available, the m_hasAssignedAtomicCounter flag will be used before rasterization for try to find an
	* atomic counter with name given by m_atomicCounterToBindName and set it to m_atomicCounter, setting the m_hasAssignedTexture flag as true
	* @param name [in] name of the atomic counter to use for this atomic counter unit (will be requested to the atomic counter manager when calling the method, and, if not found, again before rasterization)
	* @return true if the atomic counter was found and assigned to m_atomicCounter successfully and false otherwise */
	bool setAtomicCounter(string &&name);

	/** Default destructor */
	~AtomicCounterUnit();
	
	GET(int, m_id, Id)
	GET(ResourceInternalType, m_atomicCounterType, ResourceInternalType)
	GET(VkShaderStageFlagBits, m_shaderStage, ShaderStage)
	GET(bool, m_hasAssignedAtomicCounter, HasAssignedAtomicCounter)
	GET_PTR(AtomicCounter, m_atomicCounter, AtomicCounter)
	GET(string, m_atomicCounterToBindName, AtomicCounterToBindName)
	GET(string, m_name, Name)

protected:
	int                   m_id;                       //!< Id of the atomic counter unit
	ResourceInternalType  m_atomicCounterType;        //!< Atomic counter unit type (can only be unsiged int atomic counter right now)
	VkShaderStageFlagBits m_shaderStage;              //!< enumerated with value the shader stage the uniform being built is used
	bool                  m_hasAssignedAtomicCounter; //!< If false, this atomic counter unit still doesn't have a proper value for the m_atomicCounter to bind
	AtomicCounter*        m_atomicCounter;            //!< Atomic counter bound to this atomic counter unit
	string                m_atomicCounterToBindName;  //!< Name of the atomic counter to bind (for cases when the atomic counter is built after the initialization call of the shader containing this atomic counter unit, will be taken into account before rasterization)
	string                m_name;		              //!< Atomic counter unit name
};

/////////////////////////////////////////////////////////////////////////////////////////////

#endif _ATOMICCOUNTERUNIT_H_
