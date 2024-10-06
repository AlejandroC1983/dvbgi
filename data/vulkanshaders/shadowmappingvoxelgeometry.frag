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

layout (location = 0) in vec3 worldPosition;

/////////////////////////////////////////////////////////////////////////////////////////////

layout(location = 0) out float color;

/////////////////////////////////////////////////////////////////////////////////////////////

layout (std140, binding = 1) uniform materialData
{
    mat4 viewProjection; //!< Shadow mapping view projection matrix
    vec4 lightPosition;  //!< Shadow mapping camera position
} myMaterialData;

/////////////////////////////////////////////////////////////////////////////////////////////

void main()
{
    color = length(worldPosition - myMaterialData.lightPosition.xyz);
}

/////////////////////////////////////////////////////////////////////////////////////////////
