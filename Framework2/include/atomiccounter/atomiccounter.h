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

#ifndef _ATOMICCOUNTER_H_
#define _ATOMICCOUNTER_H_

// GLOBAL INCLUDES

// PROJECT INCLUDES
#include "../../include/util/genericresource.h"

// CLASS FORWARDING

// NAMESPACE

// DEFINES


/////////////////////////////////////////////////////////////////////////////////////////////

/** Atomic counter class wrapper */
class AtomicCounter : public GenericResource
{
protected:
	/** Parameter constructor
	* @param [in] atomic counter's name
	* @return nothing */
	AtomicCounter(string &&name);

	/** Default destructor
	* @return nothing */
	virtual ~AtomicCounter();

public:
	/** Init resource
	* @return nothing */
	void init();

	/** Binds this atomic counter
	* @return nothing */
	void bind() const;

	/** Unbinds the atomic counter
	* @return nothing */
	void unbind() const;

	/** Recovers from GPU memory the value of this atomic counter
	* @return nothing */
	uint getValue();

	/** Sets to GPU memory the value of this atomic counter
	* @param value [in] value to set
	* @return nothing */
	void setValue( uint value );

	GET(int, m_id, Id)

protected:
	SET(int, m_id, Id)
	
	int m_id; //!< Id of the resource
};

/////////////////////////////////////////////////////////////////////////////////////////////

#endif _ATOMICCOUNTER_H_
