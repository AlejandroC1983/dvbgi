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

layout (std140, binding = 2) uniform materialData
{
    vec3 color;
} myMaterialData;

/////////////////////////////////////////////////////////////////////////////////////////////

layout(binding = 3) uniform sampler2D reflectance;
layout(binding = 4) uniform sampler2D normal;

/////////////////////////////////////////////////////////////////////////////////////////////

layout(location = 0) in vec2 uv;
layout(location = 1) in vec3 positionVS;

/////////////////////////////////////////////////////////////////////////////////////////////

layout(location = 0) out vec4 outColor;

/////////////////////////////////////////////////////////////////////////////////////////////

void main()
{
	vec4 reflectance = texture(reflectance, uv);
    
#ifdef MATERIAL_TYPE_ALPHATESTED
    if(reflectance.w == 0.0)
    {
        discard;
    }
#endif

    outColor.x = pow(reflectance.x, 1.0 / 2.2);
    outColor.y = pow(reflectance.y, 1.0 / 2.2);
    outColor.z = pow(reflectance.z, 1.0 / 2.2);
    outColor.a = 1.0;
}

/////////////////////////////////////////////////////////////////////////////////////////////
