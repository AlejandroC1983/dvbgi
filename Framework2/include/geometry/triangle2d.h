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

#ifndef _TRIANGLE2D_H_
#define _TRIANGLE2D_H_

// GLOBAL INCLUDES

// PROJECT INCLUDES
#include "../../include/geometry/bbox.h"

// CLASS FORWARDING
class Node;

// NAMESPACE

// DEFINES

/////////////////////////////////////////////////////////////////////////////////////////////

// Class used to manage the mesh containment test
class Triangle2D
{
public:
	/** Default constructor
	* @return nothing */
	Triangle2D();

	/** Builds a new Triangle2D with v0, v1 and v2 as vertices
	* @param v0 [in] one of the three verties of the triangle
	* @param v1 [in] one of the three verties of the triangle
	* @param v2 [in] one of the three verties of the triangle
	* @return nothing */
	Triangle2D(const vec2 &v0, const vec2 &v1, const vec2 &v2);

	/** Getter of m_v0, m_v1 and m_v2
	* @return a vector with the three vertices of the triangle */
	vectorVec2 getVertices();

	/** Setter of m_v0, m_v1 and m_v2
	* @param v0 [in] one of the three verties of the triangle
	* @param v1 [in] one of the three verties of the triangle
	* @param v2 [in] one of the three verties of the triangle
	* @return nothing */
	void setVertices(const vec2 &v0, const vec2 &v1, const vec2 &v2);

	GET_SET(vec2, m_v0, Vert0)
	GET_SET(vec2, m_v1, Vert1)
	GET_SET(vec2, m_v2, Vert2)

protected:
	vec2 m_v0; //!< One of the three vertices of the triangle
	vec2 m_v1; //!< One of the three vertices of the triangle
	vec2 m_v2; //!< One of the three vertices of the triangle
};

/////////////////////////////////////////////////////////////////////////////////////////////

#endif _TRIANGLE2D_H_
