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

layout (std140, binding = 0) uniform sceneElementTransform
{
    mat4 model;
} mySceneElementTransform;

/////////////////////////////////////////////////////////////////////////////////////////////

layout (std140, binding = 1) uniform sceneCamera
{
    mat4 view;
    mat4 projection;
    vec4 sceneOffset;
    vec4 sceneExtent;
} mySceneCamera;

/////////////////////////////////////////////////////////////////////////////////////////////

layout (std140, binding = 2) uniform materialData
{
    vec4 voxelSize;                   //!< Size of the voxelization texture
    vec4 sceneMin;                    //!< Scene min AABB value
    vec4 sceneExtent;                 //!< Scene extent
    vec4 lightPosition;               //!< Light position
    mat4 shadowViewProjection;        //!< shadow map view projection matrix
    vec4 offsetAndSize;               //!< Texture offset for rasterizing the omnidirectional distance maps (x, y coordinates), size of the patch (z coordinate) and size of the texture (w coordinate)
    vec4 zFar;                        //!< Shadow distance map values are divided by z far camera plane value (x coordinate) to set values approximately in the [0,1] interval, although values can be bigger than 1
    vec4 lightForwardEmitterRadiance; //!< Light forward direction in xyz components, emitter radiance in w component
    uint irradianceFieldGridDensity;  //!< Density of the irradiance field's grid (value n means one irradiance field each n units in the voxelization volume in all dimensions)
} myMaterialData;

/////////////////////////////////////////////////////////////////////////////////////////////

layout (location = 0) in vec3 position;
layout (location = 1) in uvec3 UVNormalTangentFlags;

// Instance attributes
layout (location = 2) in vec3 instancePos;
layout (location = 3) in float instanceScale;

/////////////////////////////////////////////////////////////////////////////////////////////

// TODO: Remove not needed
layout (location = 0) out vec2 outUV;
layout (location = 1) out vec3 positionVS;
layout (location = 2) out vec3 worldNormalVS;
layout (location = 3) out vec3 worldTangentVS;
layout (location = 4) out vec3 lightDirection;
layout (location = 5) flat out float isDynamicSceneElement;

vec3 convRGB8ToVec(uint val)
{
    return vec3(float((val&0x000000FF)), float((val&0x0000FF00)>>8U), float ((val&0x00FF0000)>>16U)); // TODO: Review offset, might be wrong and needs to be fixed at vertex level
}

/////////////////////////////////////////////////////////////////////////////////////////////

void main()
{
    vec3 normalW      = convRGB8ToVec(UVNormalTangentFlags.y);
    normalW          /= 255.0;
    normalW          -= vec3(0.5);
    normalW          *= 2.0;

    vec3 tangentW     = convRGB8ToVec(UVNormalTangentFlags.z);
    tangentW         /= 255.0;
    tangentW         -= vec3(0.5);
    tangentW         *= 2.0;

    outUV             = unpackHalf2x16(UVNormalTangentFlags.x);
    vec4 positionVS4D = mySceneElementTransform.model * vec4(position, 1.0);
    gl_Position       = mySceneCamera.projection * mySceneCamera.view * positionVS4D;
    positionVS        = positionVS4D.xyz;
    mat3 normalMatrix = mat3(transpose(inverse(mySceneElementTransform.model)));
    worldNormalVS     = normalMatrix * normalW;
    worldTangentVS    = normalMatrix * tangentW;
    lightDirection    = myMaterialData.lightPosition.xyz - position; // No need to normalize since normalization is mandatory on fragment shader

    isDynamicSceneElement = (((UVNormalTangentFlags.z & 0xFF000000) >>24U) > 0) ? 1.0 : 0.0;
}

/////////////////////////////////////////////////////////////////////////////////////////////
