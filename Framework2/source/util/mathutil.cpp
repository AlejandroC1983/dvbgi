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
#include "../../include/util/mathutil.h"
#include "../../include/util/loopMacroDefines.h"
#include "../../include/math/transform.h"

// NAMESPACE

// DEFINES

// STATIC MEMBER INITIALIZATION

/////////////////////////////////////////////////////////////////////////////////////////////

vec3 MathUtil::rotateVector(const float &fAngleRad, const vec3 &vAxis, const vec3 &vRotVec)
{
	float fSinAngle = sinf(fAngleRad / 2.0f);
	quat q          = quat(cosf(fAngleRad / 2.0f), vAxis.x * fSinAngle, vAxis.y * fSinAngle, vAxis.z * fSinAngle);
	quat qInv       = inverse(q);
	quat P          = quat(0.0f, vRotVec.x, vRotVec.y, vRotVec.z);
	P               = normalize(P);
	quat qPqInv     = q * P * qInv;
	return vec3(qPqInv.x, qPqInv.y, qPqInv.z);
}

/////////////////////////////////////////////////////////////////////////////////////////////

void MathUtil::transformPoint(const mat4 &tMat, vec3 &vP)
{
	vec4 vT = tMat * vec4(vP, 1.0f);
	vP		= vec3(vT.x, vT.y, vT.z);
}

/////////////////////////////////////////////////////////////////////////////////////////////

void MathUtil::transformXYZToMinusXZY(vectorVec3 &vecData)
{
	vec3 temp;
	forI(vecData.size())
	{
		temp = vecData[i];
		vecData[i] = vec3(-1.0f * temp.x, temp.z, temp.y);
	}
}

/////////////////////////////////////////////////////////////////////////////////////////////

void MathUtil::transformXYZToMinusXZY(vec3 &data)
{
	data = vec3(-1.0f * data.x, data.z, data.y);
}

/////////////////////////////////////////////////////////////////////////////////////////////

vec3 MathUtil::convertToModelSpace(vectorVec3& vertices)
{
	const uint maxIndex = uint(vertices.size());

	vec3 min = vec3(FLT_MAX);
	vec3 max = vec3(-1.0f * FLT_MAX);
	vec3 temp;

	forI(maxIndex)
	{
		temp  = vertices[i];
		max.x = glm::max(temp.x, max.x);
		max.y = glm::max(temp.y, max.y);
		max.z = glm::max(temp.z, max.z);
		min.x = glm::min(temp.x, min.x);
		min.y = glm::min(temp.y, min.y);
		min.z = glm::min(temp.z, min.z);
	}

	vec3 mid = (min + max) * 0.5f;

	forI(maxIndex)
	{
		vertices[i] -= mid;
	}

	return mid;
}

/////////////////////////////////////////////////////////////////////////////////////////////

vectorFloat MathUtil::buildGeometryBuffer(const vectorVec3& vectorVertex, const vectorVec3& vectorNormal, const vectorUint& vectorIndex)
{
	const uint maxIndex = uint(vectorIndex.size());

	vectorFloat result;
	result.resize((maxIndex / 3) * 12);

	float* pVector = result.data();
	uint counter   = 0;

	vec3 n0;
	vec3 n1;
	vec3 n2;
	vec3 normal;

	forIStep(3, maxIndex)
	{
		pVector[counter++] = vectorVertex[vectorIndex[i + 0]][0];
		pVector[counter++] = vectorVertex[vectorIndex[i + 0]][1];
		pVector[counter++] = vectorVertex[vectorIndex[i + 0]][2];
		pVector[counter++] = vectorVertex[vectorIndex[i + 1]][0];
		pVector[counter++] = vectorVertex[vectorIndex[i + 1]][1];
		pVector[counter++] = vectorVertex[vectorIndex[i + 1]][2];
		pVector[counter++] = vectorVertex[vectorIndex[i + 2]][0];
		pVector[counter++] = vectorVertex[vectorIndex[i + 2]][1];
		pVector[counter++] = vectorVertex[vectorIndex[i + 2]][2];
		n0                 = vectorNormal[vectorIndex[i + 0]];
		n1                 = vectorNormal[vectorIndex[i + 1]];
		n2                 = vectorNormal[vectorIndex[i + 2]];
		normal             = normalize(n0 + n1 + n2);
		pVector[counter++] = normal[0];
		pVector[counter++] = normal[1];
		pVector[counter++] = normal[2];
	}

	return result;
}

/////////////////////////////////////////////////////////////////////////////////////////////

mat4 MathUtil::assimpAIMatrix4x4ToGLM(const aiMatrix4x4* aiMatrix)
{
	glm::mat4 to;

	to[0][0] = (float)aiMatrix->a1; to[0][1] = (float)aiMatrix->b1;  to[0][2] = (float)aiMatrix->c1; to[0][3] = (float)aiMatrix->d1;
	to[1][0] = (float)aiMatrix->a2; to[1][1] = (float)aiMatrix->b2;  to[1][2] = (float)aiMatrix->c2; to[1][3] = (float)aiMatrix->d2;
	to[2][0] = (float)aiMatrix->a3; to[2][1] = (float)aiMatrix->b3;  to[2][2] = (float)aiMatrix->c3; to[2][3] = (float)aiMatrix->d3;
	to[3][0] = (float)aiMatrix->a4; to[3][1] = (float)aiMatrix->b4;  to[3][2] = (float)aiMatrix->c4; to[3][3] = (float)aiMatrix->d4;

	return to;
}

/////////////////////////////////////////////////////////////////////////////////////////////

int MathUtil::getNextPowerOfTwo(uint value)
{
	// https://graphics.stanford.edu/~seander/bithacks.html#RoundUpPowerOf2

	value--;
	value |= value >> 1;
	value |= value >> 2;
	value |= value >> 4;
	value |= value >> 8;
	value |= value >> 16;
	value++;
	return value;
}

/////////////////////////////////////////////////////////////////////////////////////////////

bool MathUtil::isPowerOfTwo(uint value)
{
	return ((value & (value - 1)) == 0);
}

/////////////////////////////////////////////////////////////////////////////////////////////

mat4 MathUtil::orthographicRH(float left, float right, float bottom, float top, float zNear, float zFar)
{
	mat4 Result(1);
	Result[0][0] =  2.0f / (right - left);
	Result[1][1] =  2.0f / (top - bottom);
	Result[3][0] = -(right + left) / (right - left);
	Result[3][1] = -(top + bottom) / (top - bottom);
	Result[2][2] = -1.0f / (zFar - zNear);
	Result[3][2] = -zNear / (zFar - zNear);

	return Result;
}

/////////////////////////////////////////////////////////////////////////////////////////////

bool MathUtil::transformAndVerifyWindingOrder(vectorUint& indices,
		                                      vectorVec3& vertices,
		                                      vectorVec3& normals,
		                                      vectorVec3& tangents,
		                                      mat4&       transform)
{
	uint numVertices  = indices.size();
	uint numTriangles = numVertices / 3;

	vec3 v0;
	vec3 v1;
	vec3 v2;
	vec3 v0v1;
	vec3 v0v2;
	vec3 crossResult;
	vec4 vPosition4D;
	vec3 vertexNormal;
	float dotResult;

	vectorFloat vectorDotResult(numTriangles);

	// For each triangle, compare the normal direction 
	forI(numTriangles)
	{
		v0                 = vertices[3 * i + 0];
		v1                 = vertices[3 * i + 1];
		v2                 = vertices[3 * i + 2];
		v0v1               = normalize(v1 - v0);
		v0v2               = normalize(v2 - v0);
		vertexNormal       = normalize(normals[3 * i + 0] + normals[3 * i + 1] + normals[3 * i + 1]); // Assuming a mean value normal
		crossResult        = normalize(cross(v0v1, v0v2));
		dotResult          = dot(crossResult, vertexNormal);
		vectorDotResult[i] = dotResult >= 0.0f ? 1.0f : -1.0f;
	}

	mat3 normalMatrix = mat3(transpose(inverse(transform)));
	forI(numVertices)
	{
		vec4 vPosition4D = transform * vec4(vertices[i], 1.0f);
		vertices[i]      = vec3(vPosition4D.x, vPosition4D.y, vPosition4D.z);
		normals[i]       = normalize(normalMatrix * normals[i]);
		tangents[i]      = normalize(normalMatrix * tangents[i]);
	}

	// Test the winding order according to the data sotred in vectorDotResult in the first run of the loop
	forI(numTriangles)
	{
		v0                 = vertices[3 * i + 0];
		v1                 = vertices[3 * i + 1];
		v2                 = vertices[3 * i + 2];
		v0v1               = normalize(v1 - v0);
		v0v2               = normalize(v2 - v0);
		vertexNormal       = normalize(normals[3 * i + 0] + normals[3 * i + 1] + normals[3 * i + 1]); // Assuming a mean value normal
		crossResult        = normalize(cross(v0v1, v0v2));
		dotResult          = dot(crossResult, vertexNormal);

		if (vectorDotResult[i] != dotResult)
		{
			cout << "ERROR: Different winding order for triangle " << i << endl;
		}
	}

	return false;
}

/////////////////////////////////////////////////////////////////////////////////////////////

vec4 MathUtil::convertRGBA8ToVec4(uint val)
{
	return vec4(float((val & 0x000000FF)), float((val & 0x0000FF00) >> 8U), float((val & 0x00FF0000) >> 16U), float((val & 0xFF000000) >> 24U));
}

/////////////////////////////////////////////////////////////////////////////////////////////

uint MathUtil::convertVec4ToRGBA8(vec4 val)
{
	return (uint(val.w) & 0x000000FF) << 24U | (uint(val.z) & 0x000000FF) << 16U | (uint(val.y) & 0x000000FF) << 8U | (uint(val.x) & 0x000000FF);
}

/////////////////////////////////////////////////////////////////////////////////////////////

ivec3 MathUtil::convertRGB8ToIvec3(uint val)
{
	return ivec3(val & 0x000000FF, (val & 0x0000FF00) >> 8U, (val & 0x00FF0000) >> 16U);
}

/////////////////////////////////////////////////////////////////////////////////////////////

uint MathUtil::convertIvec3ToRGB8(ivec3 val)
{
	return ((val.z & 0x000000FF) << 16U | (val.y & 0x000000FF) << 8U | (val.x & 0x000000FF));
}

/////////////////////////////////////////////////////////////////////////////////////////////
