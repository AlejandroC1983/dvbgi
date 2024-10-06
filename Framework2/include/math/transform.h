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

#ifndef _TRANSFORM_H_
#define _TRANSFORM_H_

// GLOBAL INCLUDES

// PROJECT INCLUDES
#include "../../include/headers.h"
#include "../../include/util/getsetmacros.h"

// CLASS FORWARDING
class Node;

// NAMESPACE

// DEFINES

/////////////////////////////////////////////////////////////////////////////////////////////

// Class used to manage rts data for a given node (rotation, traslation and scaling)
class Transform
{
public:
	/** Default constructor
	* @return nothing */
	Transform();

	/** Parameter constructor
	* @param translation [in] traslation to apply to the transform matrix
	* @return nothing */
	Transform(const vec3 traslation);

	/** Parameter constructor
	* @param traslation [in] traslation to apply to the transform matrix
	* @param rotation   [in] rotation to apply t the transform matrix
	* @return nothing */
	Transform(const vec3 traslation, const quat rotation);

	/** Parameter constructor
	* @param traslation [in] traslation to apply to the transform matrix
	* @param rotation   [in] rotation to apply t the transform matrix
	* @param scaling    [in] scaling to apply to the transform matrix
	* @return nothing */
	Transform(const vec3 traslation, const quat rotation, const vec3 scaling);

	/** Updates m_mat taking the values from m_traslation, m_scaling and m_rotation
	* @return nothing */
	void updateMatrix();

	/** Adds a rotation in the x axis of angle degrees to the current rotation present in m_rotation and m_mat
	* @param angle [in] angle rotation around the x axis to add to the current rotation
	* @return nothing */
	void addRotationX(const float angle);

	/** Adds a rotation in the y axis of angle degrees to the current rotation present in m_rotation and m_mat
	* @param angle [in] angle rotation around the y axis to add to the current rotation
	* @return nothing */
	void addRotationY(const float angle);

	/** Adds a rotation in the z axis of angle degrees to the current rotation present in m_rotation and m_mat
	* @param angle [in] angle rotation around the z axis to add to the current rotation
	* @return nothing */
	void addRotationZ(const float angle);

	/** Adds a rotation around the x, y and z axis given by the angle parameter in degrees to the current rotation
	* present in m_rotation and m_mat
	* @param angle [in] angle rotation around the x, y and z axis to add to the current rotation
	* @return nothing */
	void addRotation(const vec3 angle);

	void addRotation(const quat q);

	/** Sets the rotation around the x, y and z axis given by the angle parameter in degrees to the current rotation
	* present in m_rotation and m_mat
	* @param angle [in] angle rotation around the x, y and z axis to set
	* @return nothing */
	void setRotation(const vec3 angle);

	/** Sets the rotation around the x, y and z axis given by the quaternion given as parameter
	* @param q [in] quaternion describing the rotation to set
	* @return nothing */
	void setRotation(const quat q);

	/** Adds the traslation given by the offset parameter to the current translation, m_traslation and to m_mat
	* @param offset [in] traslation to add
	* @return nothing */
	void addTraslation(const vec3 offset);

	/** Sets the traslation given by the traslation parameter to the current translation, m_traslation and to m_mat
	* @param offset [in] traslation to set
	* @return nothing */
	void setTraslation(const vec3 traslation);

	/** Adds scaling in the x axis of value to the current scaling present in m_scaling and m_mat
	* @param value [in] scaling to add to the current x scaling
	* @return nothing */
	void addScalingX(const float value);

	/** Adds scaling in the y axis of value to the current scaling present in m_scaling and m_mat
	* @param value [in] scaling to add to the current y scaling
	* @return nothing */
	void addScalingY(const float value);

	/** Adds scaling in the z axis of value to the current scaling present in m_scaling and m_mat
	* @param value [in] scaling to add to the current z scaling
	* @return nothing */
	void addScalingZ(const float value);

	/** Adds the value given as parameter to the current scaling value present in m_scaling and m_mat
	* @param value [in] scaling to add to the current scaling
	* @return nothing */
	void addScaling(const vec3 value);

	/** Sets the scaling value to the current scaling present in m_scaling and m_mat
	* @param value [in] scaling to set as current
	* @return nothing */
	void setScaling(const vec3 value);

	GETCOPY(vec3, m_traslation, Traslation)
	GETCOPY(vec3, m_scaling, Scaling)
	GET(quat, m_rotation, Rotation)
	GET(mat4, m_mat, Mat)
	GETCOPY_SET(bool, m_dirty, Dirty)

protected:
	vec3 m_traslation; //!< traslation data
	vec3 m_scaling;    //!< scaling data
	quat m_rotation;   //!< rotation data
	mat4 m_mat;        //!< matrix containing all the transform info
	bool m_dirty;      //!< true whenever m_mat is modified, set to false when by the transform owner (a Node)
};

/////////////////////////////////////////////////////////////////////////////////////////////

#endif _TRANSFORM_H_
