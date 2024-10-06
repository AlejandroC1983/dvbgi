/*
Copyright 2024 Alejandro Cosin

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

/////////////////////////////////////////////////////////////////////////////////////////////

layout (location = 0) in uint hashedPosition;

/////////////////////////////////////////////////////////////////////////////////////////////

layout (location = 0) out vec3 positionTextureCoordinatesVS;
layout (location = 1) out uint hashedPositionVS;

/////////////////////////////////////////////////////////////////////////////////////////////

layout (std140, binding = 0) uniform sceneCamera
{
    mat4 view;
    mat4 projection;
    vec4 sceneOffset;
    vec4 sceneExtent;
} mySceneCamera;

/////////////////////////////////////////////////////////////////////////////////////////////

layout (std140, binding = 1) uniform materialData
{
	mat4 viewProjection;
	vec4 voxelizationSize;
	vec4 sceneMin;
	vec4 sceneExtent;
} myMaterialData;

/////////////////////////////////////////////////////////////////////////////////////////////

/** Computes the unhashed 3D coordinates for the position given with hashed value as parameter, for a uniform
* voxelization volume given by voxelizationWidth
* @param value [in] value to unhash
* @return unhashed value for parameter value */
vec3 unhashValue(uint value)
{
	float sizeFloat = float(myMaterialData.voxelizationSize.x);
	float number    = float(value) / sizeFloat;

	uvec3 result;
	float integerPart;
	float fractional = modf(number, integerPart);
	result.z         = uint(fractional * sizeFloat);
	number          /= sizeFloat;
	fractional       = modf(number, integerPart);
	result.y         = uint(fractional * sizeFloat);
	result.x         = uint(integerPart);

	return vec3(float(result.x), float(result.y), float(result.z));
}

/////////////////////////////////////////////////////////////////////////////////////////////

void main()
{
	vec3 voxelCoordinatesFloat   = unhashValue(hashedPosition);
   	gl_Position                  = vec4(voxelCoordinatesFloat.xyz, 1.0);
	positionTextureCoordinatesVS = voxelCoordinatesFloat;
	hashedPositionVS             = hashedPosition;
}

/////////////////////////////////////////////////////////////////////////////////////////////
