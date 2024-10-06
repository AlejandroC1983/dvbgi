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

// GLOBAL INCLUDES

// PROJECT INCLUDES
#include "../../include/geometry/triangle2d.h"

// NAMESPACE

// DEFINES

// STATIC MEMBER INITIALIZATION

/////////////////////////////////////////////////////////////////////////////////////////////

Triangle2D::Triangle2D():
	m_v0(vec2(.0f)),
	m_v1(vec2(.0f)),
	m_v2(vec2(.0f))
{
}

/////////////////////////////////////////////////////////////////////////////////////////////

Triangle2D::Triangle2D(const vec2 &v0, const vec2 &v1, const vec2 &v2):
	m_v0(v0),
	m_v1(v1),
	m_v2(v2)
{
}

/////////////////////////////////////////////////////////////////////////////////////////////

vectorVec2 Triangle2D::getVertices()
{
	vectorVec2 vVerts;
	vVerts.clear();
	vVerts.push_back(m_v0);
	vVerts.push_back(m_v1);
	vVerts.push_back(m_v2);
	return vVerts;
}

/////////////////////////////////////////////////////////////////////////////////////////////

void Triangle2D::setVertices(const vec2 &v0, const vec2 &v1, const vec2 &v2)
{
	m_v0 = v0;
	m_v1 = v1;
	m_v2 = v2;
}

/////////////////////////////////////////////////////////////////////////////////////////////
