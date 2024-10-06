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

// GLOBAL INCLUDES

// PROJECT INCLUDES
#include "../../include/shader/atomiccounterunit.h"
#include "../../include/atomiccounter/atomiccounter.h"

// NAMESPACE

// DEFINES

// STATIC MEMBER INITIALIZATION

/////////////////////////////////////////////////////////////////////////////////////////////

AtomicCounterUnit::AtomicCounterUnit(const int id, string &&name, const ResourceInternalType atomicCounterType, VkShaderStageFlagBits shaderStage):
	m_id(id),
	m_atomicCounterType(atomicCounterType),
	m_shaderStage(shaderStage),
	m_hasAssignedAtomicCounter(false),
	m_atomicCounter(nullptr),
	m_name(move(name)),
	m_atomicCounterToBindName(string(""))
{

}

/////////////////////////////////////////////////////////////////////////////////////////////

bool AtomicCounterUnit::setAtomicCounter(AtomicCounter *atomicCounter)
{
	return true;
}

/////////////////////////////////////////////////////////////////////////////////////////////

bool AtomicCounterUnit::setAtomicCounter(string &&name)
{
	return m_hasAssignedAtomicCounter;
}

/////////////////////////////////////////////////////////////////////////////////////////////

AtomicCounterUnit::~AtomicCounterUnit()
{

}

/////////////////////////////////////////////////////////////////////////////////////////////
