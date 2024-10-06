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

#ifndef _MULTITYPEIMPLEMENTATION_H_
#define _MULTITYPEIMPLEMENTATION_H_

// GLOBAL INCLUDES

// PROJECT INCLUDES
#include "../../include/parameter/multitypebase.h"

// CLASS FORWARDING

// NAMESPACE

// DEFINES

/////////////////////////////////////////////////////////////////////////////////////////////

/** Templatized class to store elements in the MultiUnorderedMap */

template <typename T> class MultiTypeImplementation: public MultiTypeBase
{
public:
	virtual ~MultiTypeImplementation() = default;

	/** Assigns element to m_data
	* @return nothing */
	template <typename T> void assignData(T element)
	{
		m_data = element;
	}

	/** Getter of m_data
	* @return copy of m_data */
	T getData()
	{
		return m_data;
	}

	/** Getter of m_data
	* @return copy of m_data */
	const T getData() const
	{
		return m_data;
	}

protected:
	T m_data; //!< Templatized data element to be added to the MultiTypeUnorderedMap
};

/////////////////////////////////////////////////////////////////////////////////////////////

#endif _MULTITYPEIMPLEMENTATION_H_
