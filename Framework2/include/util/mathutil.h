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

#ifndef _MATHUTIL_H_
#define _MATHUTIL_H_

// GLOBAL INCLUDES
#include <assimp/scene.h>

// PROJECT INCLUDES
#include "../../include/headers.h"
#include "../../include/geometry/triangle3d.h"
#include "../../include/geometry/bbox.h"

// CLASS FORWARDING

// NAMESPACE

// DEFINES

/////////////////////////////////////////////////////////////////////////////////////////////

class MathUtil
{
public:
	/** To build a quaternion which rotates fAngleStep rads around an A axis, see 
	* Mathematics for 3D Game Programming & Computer Graphics (Charles River Media, 2002) chapter 3.2
	* @param fAngleRad [in] angle to rotate vRotVec around vAxis
	* @param vAxis     [in] axis to rotate vRotVec fAngleRad radians
	* @param vRotVec   [in] vector to rotate around vAxis fAngleRad radians
	* @return rotated vector */
	static vec3 rotateVector(const float &fAngleRad, const vec3 &vAxis, const vec3 &vRotVec);

	/** Transforms the point vP by the matrix tMat
	* @param tMat [in]	   matrix used to transform vP
	* @param vP   [in / out] point to be transformed
	* @return nothing */
	static void transformPoint(const mat4 &tMat, vec3 &vP);

	/** Transforms the given vector (x,y,z) coordinates into OpenGL coordinates (-x,z,y), used for info from modelas loaded with
	* z up, x front and y right coordinate systems (like 3ds studio max)
	* @param vertices [in / out] vector to transform each position with the change (x,y,z) -> (-x,z,y)
	* @return nothing */
	static void transformXYZToMinusXZY(vectorVec3 &vecData);

	/** Transforms the given vec3 (x,y,z) coordinates into OpenGL coordinates (-x,z,y), used for info from modelas loaded with
	* z up, x front and y right coordinate systems (like 3ds studio max)
	* @param data [in / out] data to transform each position with the change (x,y,z) -> (-x,z,y)
	* @return nothing */
	static void transformXYZToMinusXZY(vec3 &data);

	/** Will compute the model space matrix and modify the vertices parameter to center the geometry in model space
	* @param vertices [inout] vector with the vertices forming the geometry
	* @return traslation to put the model in world coordinates as it was defined by the content of vertices before the call */
	static vec3 convertToModelSpace(vectorVec3& vertices);

	/** Using the indices in vectorIndex to index in vectorVertex and vectorNormal, builds a vector
	* of float elements with the final triangle information interleaved (V0 V1 V2 N0 V4 V5 V6 N1 ...)
	* @param vectorVertex [in] vector with the vertex information
	* @param vectorNormal [in] vector with the normal information
	* @param vectorIndex  [in] vector with the index information
	* @return geometry buffer */
	static vectorFloat buildGeometryBuffer(const vectorVec3& vectorVertex, const vectorVec3& vectorNormal, const vectorUint& vectorIndex);

	/** Convert a aiMatrix4x4 matrix in assimp to glm mat4
	* @param aiMatrix [in] matrix to convert
	* @return converted matrix */
	static mat4 assimpAIMatrix4x4ToGLM(const aiMatrix4x4* aiMatrix);

	/** Get the next power of two value for the parameter provided
	* @param value [in] value to compute the next power of two
	* @return next power of two value computed */
	static int getNextPowerOfTwo(uint value);

	/** Compute whether the value provided is a power of two or not
	* @param value [in] value to compute if it is a power of two
	* @return true in value is power of two, false otherwise */
	static bool isPowerOfTwo(uint value);

	/** Compute a right-handed ortographic projection matrix
	* @param left   [in] left plane
	* @param right  [in] right plane
	* @param bottom [in] bottom plane
	* @param top    [in] top plane
	* @param zNear  [in] zNear plane
	* @param zFar   [in] zFar plane
	* @return projection matrix computed */
	static mat4 orthographicRH(float left, float right, float bottom, float top, float zNear, float zFar);

	/** Transform the vertices, normal and tangent data given as parameter by the transform matrix given as parameter
	* verifying the winding order for the transformed triangles is kept
	* @param indices   [in] vector with the mesh indices
	* @param vertices  [in] vector with the mesh vertices
	* @param normals   [in] vector with the mesh normals
	* @param tangents  [in] vector with the mesh tangents
	* @param transform [in] vector with the mesh transform
	* @return true if any changes were applied, false otherwise */
	static bool transformAndVerifyWindingOrder(vectorUint& indices,
		                                       vectorVec3& vertices,
		                                       vectorVec3& normals,
		                                       vectorVec3& tangents,
		                                       mat4&       transform);

	/** Decode 8-bit per channel RGBA value in val
	* @param val [in] value to decode
	* @return decoded 8-bit per channel RGBA color value */
	static vec4 convertRGBA8ToVec4(uint val);

	/** Encode 8-bit per channel RGBA value given as parameter in a single unsigned it
	* @param value [in] value to encode
	* @return unsigned int with encoded values */
	static uint convertVec4ToRGBA8(vec4 val);

	/** Decode 8-bit per channel RGB value in val
	* @param val [in] value to decode
	* @return decoded 8-bit per channel RGB color value */
	static ivec3 convertRGB8ToIvec3(uint val);

	/** Encode 8-bit per channel RGB value given as parameter in a single unsigned it
	* @param value [in] value to encode
	* @return unsigned int with encoded values */
	static uint convertIvec3ToRGB8(ivec3 val);
};

/////////////////////////////////////////////////////////////////////////////////////////////

#endif _MATHUTIL_H_
