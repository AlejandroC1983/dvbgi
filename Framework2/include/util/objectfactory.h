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

#ifndef _OBJECTFACTORY_H_
#define _OBJECTFACTORY_H_

// GLOBAL INCLUDES

// PROJECT INCLUDES
#include "../../include/commonnamespace.h"

// CLASS FORWARDING

// NAMESPACE
using namespace commonnamespace;

// DEFINES
// To automatically register each class of the shader factory
//https://www.codeproject.com/Articles/363338/Factory-Pattern-in-Cplusplus
//http://stackoverflow.com/questions/9975672/c-automatic-factory-registration-of-derived-types
#define REGISTER_TYPE(baseClass, registerSubClassName) \
class registerSubClassName##IntoFactory : public ObjectFactory<baseClass> \
{ \
public: \
    registerSubClassName##IntoFactory() \
    { \
        baseClass##Factory::registerSubClass(string(#registerSubClassName), this); \
    } \
    virtual baseClass *create(string &&name) \
    { \
        return new registerSubClassName(move(name), move(string(#registerSubClassName))); \
    } \
}; \
static registerSubClassName##IntoFactory global_##registerSubClassName##IntoFactory;

// To allow each class built to automatically register new classes to be able to build new instances of the class
// (since the shader and subclass constructors are protected
#define DECLARE_FRIEND_REGISTERER(registerSubClassName) friend class registerSubClassName##IntoFactory;

/////////////////////////////////////////////////////////////////////////////////////////////

template <class T> class ObjectFactory
{
public:
	/** Builds a new element, method used to build new elements from the factories (Shader, Material, etc)
	* @return pointer to the new built instance */
	virtual T* create(string &&name) = 0;
};

/////////////////////////////////////////////////////////////////////////////////////////////

#endif _OBJECTFACTORY_H_
