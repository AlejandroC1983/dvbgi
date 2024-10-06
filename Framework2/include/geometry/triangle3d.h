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

#ifndef _TRIANGLE3D_H_
#define _TRIANGLE3D_H_

// GLOBAL INCLUDES

// PROJECT INCLUDES
#include "../../include/geometry/bbox.h"

// CLASS FORWARDING
class Node;

// NAMESPACE

// DEFINES

/////////////////////////////////////////////////////////////////////////////////////////////

// Class used to manage the mesh containment test
class Triangle3D
{
public:
	/** Default constructor
	* @return nothing */
	Triangle3D();

	/** Builds a new CTriangle3D with v0, v1 and v2 as vertices
	* @param v0 [in] one of the three verties of the triangle
	* @param v1 [in] one of the three verties of the triangle
	* @param v2 [in] one of the three verties of the triangle
	* @return nothing */
	Triangle3D(const vec3 &v0, const vec3 &v1, const vec3 &v2);

	/** Computes the bounding box of the triangle
	* @return the bounding box of the triangle */
	BBox3D getBBox();

	/** Getter of m_v0, m_v1 and m_v2
	* @return a vector with the three vertices of the triangle */
	vectorVec3 getVertices();

	/** Setter of m_v0, m_v1 and m_v2
	* @param v0 [in] one of the three verties of the triangle
	* @param v1 [in] one of the three verties of the triangle
	* @param v2 [in] one of the three verties of the triangle
	* @return nothing */
	void setVertices(const vec3 &v0, const vec3 &v1, const vec3 &v2);

	GET_SET(vec3, m_v0, Vert0)
	GET_SET(vec3, m_v1, Vert1)
	GET_SET(vec3, m_v2, Vert2)

protected:
	vec3 m_v0; //!< One of the three vertices of the triangle
	vec3 m_v1; //!< One of the three vertices of the triangle
	vec3 m_v2; //!< One of the three vertices of the triangle
};

/////////////////////////////////////////////////////////////////////////////////////////////

#endif _TRIANGLE3D_H_
