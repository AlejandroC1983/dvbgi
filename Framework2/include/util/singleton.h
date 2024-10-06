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

#ifndef _SINGLETON_H_
#define _SINGLETON_H_

// GLOBAL INCLUDES
#include <string>

// PROJECT INCLUDES

// CLASS FORWARDING

// NAMESPACE

// DEFINES

/////////////////////////////////////////////////////////////////////////////////////////////

template <class T> class Singleton
{
public:
	/** Return singleton instance's pointer
	* @return singleton instance's pointer */
	static T* instance();

	/** Build singleton instance
	* @return singleton instance's pointer */
	static T* init();

protected:
	/** Default constructor
	* @return nothing */
	Singleton() {}

	/** Default destructor
	* @return nothing */
	virtual ~Singleton() {}

private:
	/** Avoid implementation of reference parameter constructor
	* @return nothing */
	Singleton(Singleton const&);

	/** Avoid implementation of copy constructor
	* @return nothing */
	Singleton &operator=(Singleton const&);

	static T* m_instance; //!< Static variable, singleton instance
};

template <class T> T* Singleton<T>::m_instance = nullptr; // single initialization, in main.cpp now

/////////////////////////////////////////////////////////////////////////////////////////////

// INLINE METHODS

/////////////////////////////////////////////////////////////////////////////////////////////

template <class T> inline T *Singleton<T>::instance()
{
	return m_instance;
}

/////////////////////////////////////////////////////////////////////////////////////////////

template <class T> inline T *Singleton<T>::init()
{
	if (!m_instance)
	{
		m_instance = new T;
	}
	return m_instance;
}

/////////////////////////////////////////////////////////////////////////////////////////////

#endif _SINGLETON_H_
