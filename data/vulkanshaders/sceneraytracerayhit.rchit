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

hitAttributeEXT vec2 attribs;

struct hitPayload
{
    vec3 cameraPosition;
    float distance;
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

layout(location = 0) rayPayloadInEXT hitPayload prd;

layout(binding = 3, scalar) buffer Vertices { Vertex v[]; } vertices[];
layout(binding = 4) buffer Indices { uint i[]; } indices[];

/** Buffer used to store the index of the visible voxels for each voxel face */
layout (binding = 5, scalar) buffer coherent sceneDescriptionBufferCamera
{
    SceneDescription scnDesc[ ];
};

void main()
{
    // Object of this instance
    uint objId = scnDesc[gl_InstanceID].objId;

    // Indices of the triangle
    ivec3 ind = ivec3(indices[nonuniformEXT(objId)].i[3 * gl_PrimitiveID + 0],   //
                      indices[nonuniformEXT(objId)].i[3 * gl_PrimitiveID + 1],   //
                      indices[nonuniformEXT(objId)].i[3 * gl_PrimitiveID + 2]);  //

    // Vertex of the triangle
    Vertex v0 = vertices[nonuniformEXT(objId)].v[ind.x];
    Vertex v1 = vertices[nonuniformEXT(objId)].v[ind.y];
    Vertex v2 = vertices[nonuniformEXT(objId)].v[ind.z];

    const vec3 barycentrics = vec3(1.0 - attribs.x - attribs.y, attribs.x, attribs.y);

    // Computing the coordinates of the hit position
    vec3 worldPos = v0.position * barycentrics.x + v1.position * barycentrics.y + v2.position * barycentrics.z;
    
    // Transforming the position to world space
    vec4 transformResult = scnDesc[gl_InstanceCustomIndexEXT].transform * vec4(worldPos, 1.0);
    worldPos = transformResult.xyz;

    prd.distance = distance(prd.cameraPosition, worldPos);
}
