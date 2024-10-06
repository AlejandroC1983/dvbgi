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

#ifndef _FACTORYTEMPLATE_H_
#define _FACTORYTEMPLATE_H_

// GLOBAL INCLUDES

// PROJECT INCLUDES
#include "../../include/util/objectfactory.h"

// CLASS FORWARDING

// NAMESPACE
using namespace commonnamespace;

// DEFINES

/////////////////////////////////////////////////////////////////////////////////////////////

template <class T> class FactoryTemplate
{
public:
	/** Adds the pair <string, ObjectFactory>(name, buildMethod) if not present to m_mapFactory, allowing new elements of that Shader subclass
	* to be instantiated
	* @param name	     [in] name of the Shader subclass
	* @param buildMethod [in] class with a build method to generate new instances of this Shader subclass
	* @return true if added successfully and false otherwise */
	static bool registerSubClass(string &&name, ObjectFactory<T> *buildMethod);

	/** Uses the m_mapFactory to build an already registered class
	* @param name         [in] name of the class to build a new instance
	* @param instanceName [in] name of the instance to build
	* @return a pointer to the newly built class if success and nullptr otherwise */
	static T *buildElement(string &&name);

protected:
	static map<string, ObjectFactory<T>*> m_mapFactory; //!< Map used to generate the new instances of the classes from this factory
};

/////////////////////////////////////////////////////////////////////////////////////////////

template <class T> inline bool FactoryTemplate<T>::registerSubClass(string &&name, ObjectFactory<T> *buildMethod)
{
	bool result = addIfNoPresent(move(name), buildMethod, m_mapFactory);

	if (!result)
	{
		cout << "ERROR: failed trying to register a class " << name << endl;
	}

	return result;
}

/////////////////////////////////////////////////////////////////////////////////////////////

template <class T> inline T *FactoryTemplate<T>::buildElement(string &&name)
{
	auto it = m_mapFactory.find(name);

	if (it != m_mapFactory.end())
	{
		return it->second->create(move(name));
	}

	cout << "ERROR: trying to instantiate a non registered shader class " << name << endl;
	return nullptr;
}

/////////////////////////////////////////////////////////////////////////////////////////////

#endif _FACTORYTEMPLATE_H_
