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

layout (location = 0) in vec3 position;
layout (location = 1) in uvec3 UVNormalTangentFlags;

/////////////////////////////////////////////////////////////////////////////////////////////

layout (location = 1) out vec2 vsUV;

/////////////////////////////////////////////////////////////////////////////////////////////

void main()
{   
    gl_Position = mySceneElementTransform.model * vec4(position, 1.0);
    vsUV        = unpackHalf2x16(UVNormalTangentFlags.x);
}

/////////////////////////////////////////////////////////////////////////////////////////////
