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

#extension GL_EXT_ray_tracing : require

struct hitPayload
{
    vec3 hitPosition;
    int isStaticGeometry;
    int anyHit;
    vec3 reflectance;
    vec3 normalWorld;
    vec3 rayOrigin;
    float mipLevelValue;
    float value0;
    float value1;
    float value2;
    float value3;
};

/////////////////////////////////////////////////////////////////////////////////////////////

layout(location = 0) rayPayloadInEXT hitPayload prd;

/////////////////////////////////////////////////////////////////////////////////////////////

void main()
{

}

/////////////////////////////////////////////////////////////////////////////////////////////
