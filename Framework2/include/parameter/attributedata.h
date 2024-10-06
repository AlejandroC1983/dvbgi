/*
Copyright 2016 Alejandro Cosin

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

#ifndef _ATTRIBUTEDATA_H_
#define _ATTRIBUTEDATA_H_

// GLOBAL INCLUDES

// PROJECT INCLUDES
#include "../../include/commonnamespace.h"

// CLASS FORWARDING

// NAMESPACE
using namespace commonnamespace;

// DEFINES

/////////////////////////////////////////////////////////////////////////////////////////////

/** Basic struct to interface with MultiTypeUnorderedMap, adding and retrieving / removing elements from
* a MultiTypeUnorderedMap variable that constitutes the multy type container to store all needed parameters */

template <class T> struct AttributeData
{
	AttributeData(string&& name, T&& data):
		m_name(move(name)),
		m_hashedName(uint(hash<string>()(m_name))),
		m_data(move(data))
	{

	}

	string m_name;       //!< Name of this struct
	uint   m_hashedName; //!< Hashed version of the name of this struct
	T      m_data;       //!< Data represented by this struct
};

/////////////////////////////////////////////////////////////////////////////////////////////

#endif _ATTRIBUTEDATA_H_
