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
#include "../../include/math/transform.h"

// NAMESPACE

// DEFINES

// STATIC MEMBER INITIALIZATION

/////////////////////////////////////////////////////////////////////////////////////////////

Transform::Transform():
	m_traslation(vec3(.0f)),
	m_scaling(vec3(1.0f)),
	m_rotation(quat()),
	m_mat(mat4()),
	m_dirty(false)
{
}

/////////////////////////////////////////////////////////////////////////////////////////////

Transform::Transform(const vec3 traslation):
	m_traslation(traslation),
	m_scaling(vec3(1.0f)),
	m_rotation(quat()),
	m_mat(mat4()),
	m_dirty(false)
{
	updateMatrix();
}

/////////////////////////////////////////////////////////////////////////////////////////////

Transform::Transform(const vec3 traslation, const quat rotation):
	m_traslation(traslation),
	m_scaling(vec3(1.0f)),
	m_rotation(rotation),
	m_mat(mat4()),
	m_dirty(false)
{
	updateMatrix();
}

/////////////////////////////////////////////////////////////////////////////////////////////

Transform::Transform(const vec3 traslation, const quat rotation, const vec3 scaling):
	m_traslation(traslation),
	m_scaling(scaling),
	m_rotation(rotation),
	m_mat(mat4()),
	m_dirty(false)
{
	updateMatrix();
}

/////////////////////////////////////////////////////////////////////////////////////////////

void Transform::updateMatrix()
{
	m_mat   = translate(m_traslation) * scale(m_scaling) * mat4(m_rotation);
	m_dirty = true;
}

/////////////////////////////////////////////////////////////////////////////////////////////

void Transform::addRotationX(const float angle)
{
	m_rotation *= quat(vec3(angle, .0f, .0f));
	updateMatrix();
}

/////////////////////////////////////////////////////////////////////////////////////////////

void Transform::addRotationY(const float angle)
{
	m_rotation *= quat(vec3(.0f, angle, .0f));
	updateMatrix();
}

/////////////////////////////////////////////////////////////////////////////////////////////

void Transform::addRotationZ(const float angle)
{
	m_rotation *= quat(vec3(.0f, .0f, angle));
	updateMatrix();
}

/////////////////////////////////////////////////////////////////////////////////////////////

void Transform::addRotation(const vec3 angle)
{
	m_rotation *= quat(angle);
	updateMatrix();
}

/////////////////////////////////////////////////////////////////////////////////////////////

void Transform::addRotation(const quat q)
{
	m_rotation *= q;
	updateMatrix();
}

/////////////////////////////////////////////////////////////////////////////////////////////

void Transform::setRotation(const vec3 angle)
{
	m_rotation = quat(angle);
	updateMatrix();
}

/////////////////////////////////////////////////////////////////////////////////////////////

void Transform::setRotation(const quat q)
{
	m_rotation = q;
	updateMatrix();
}

/////////////////////////////////////////////////////////////////////////////////////////////

void Transform::addTraslation(const vec3 offset)
{
	m_traslation += offset;
	updateMatrix();
}

/////////////////////////////////////////////////////////////////////////////////////////////

void Transform::setTraslation(const vec3 translation)
{
	m_traslation = translation;
	updateMatrix();
}

/////////////////////////////////////////////////////////////////////////////////////////////

void Transform::addScalingX(const float value)
{
	m_scaling.x += value;
	updateMatrix();
}

/////////////////////////////////////////////////////////////////////////////////////////////

void Transform::addScalingY(const float value)
{
	m_scaling.y += value;
	updateMatrix();
}

/////////////////////////////////////////////////////////////////////////////////////////////

void Transform::addScalingZ(const float value)
{
	m_scaling.z += value;
	updateMatrix();
}

/////////////////////////////////////////////////////////////////////////////////////////////

void Transform::addScaling(const vec3 value)
{
	m_scaling += value;
	updateMatrix();
}

/////////////////////////////////////////////////////////////////////////////////////////////

void Transform::setScaling(const vec3 value)
{
	m_scaling = value;
	updateMatrix();
}

/////////////////////////////////////////////////////////////////////////////////////////////
