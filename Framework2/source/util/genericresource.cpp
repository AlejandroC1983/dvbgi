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

// GLOBAL INCLUDES

// PROJECT INCLUDES
#include "../../include/util/genericresource.h"

// NAMESPACE

// DEFINES

// STATIC MEMBER INITIALIZATION

/////////////////////////////////////////////////////////////////////////////////////////////

GenericResource::GenericResource() :
	  m_name("")
	, m_className("")
	, m_hashedName(0)
	, m_parameterData(nullptr)
	, m_dirty(false)
	, m_ready(false)
	, m_resourceType(GenericResourceType::GRT_UNDEFINED)
{

}

/////////////////////////////////////////////////////////////////////////////////////////////

GenericResource::GenericResource(string&& name, string&& className, GenericResourceType resourceType) :
	  m_name(move(name))
	, m_className(move(className))
	, m_dirty(false)
	, m_ready(false)
	, m_resourceType(resourceType)
{
	m_hashedName = uint(hash<string>()(m_name));
}

/////////////////////////////////////////////////////////////////////////////////////////////

GenericResource::~GenericResource()
{
	//if (m_parameterData != nullptr)
	//{
	//	delete m_parameterData;
	//	m_parameterData = nullptr;
	//}
}

/////////////////////////////////////////////////////////////////////////////////////////////

void GenericResource::init()
{

}

/////////////////////////////////////////////////////////////////////////////////////////////

void GenericResource::setName(string&& name)
{
	m_name       = move(name);
	m_hashedName = uint(hash<string>()(m_name));
}

/////////////////////////////////////////////////////////////////////////////////////////////

void GenericResource::slotElement(const char* managerName, string&& elementName, ManagerNotificationType notificationType)
{

}

/////////////////////////////////////////////////////////////////////////////////////////////
