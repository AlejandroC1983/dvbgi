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
#extension GL_EXT_nonuniform_qualifier : enable
#extension GL_EXT_scalar_block_layout : enable

/////////////////////////////////////////////////////////////////////////////////////////////

hitAttributeEXT vec2 attribs;

struct hitPayload
{
    int anyHit;
};

struct Vertex
{
    vec3 position;
    uvec3 UVNormalTangentFlags;
};

struct SceneDescription
{
    int objId;
	uint flags;
    mat4 transform;
    mat4 transformIT;
};

layout (std140, binding = 0) uniform materialData
{
    vec4 sceneMin;                    //!< Scene min AABB value
    vec4 sceneExtentVoxelizationSize; //!< Scene extent and voxel size in w field
    vec4 lightPosition;               //!< Light position in world coordinates
} myMaterialData;

/////////////////////////////////////////////////////////////////////////////////////////////

layout(location = 0) rayPayloadInEXT hitPayload prd;
layout(binding = 18, scalar) buffer Vertices { Vertex v[]; } vertices[];
layout(binding = 19) buffer Indices { uint i[]; } indices[];

layout (binding = 20, scalar) buffer coherent sceneDescriptorBuffer
{
    SceneDescription sceneDescription[ ];
};

/////////////////////////////////////////////////////////////////////////////////////////////

void main()
{
    prd.anyHit = 1;
}

/////////////////////////////////////////////////////////////////////////////////////////////
