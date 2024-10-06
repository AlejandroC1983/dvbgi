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
    vec3 hitPosition;
    vec3 hitNormal;
    float distance;
    vec3 v0;
    vec3 v1;
    vec3 v2;
    vec3 barycentrics;
    vec3 worldPos;
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

layout(binding = 12, scalar) buffer Vertices { Vertex v[]; } vertices[];
layout(binding = 13) buffer Indices { uint i[]; } indices[];

/** Buffer used to store the index of the visible voxels for each voxel face */
layout (binding = 14, scalar) buffer coherent sceneDescriptorStaticBuffer
{
    SceneDescription scnDesc[ ];
};

/////////////////////////////////////////////////////////////////////////////////////////////

vec3 convRGB8ToVec(uint val)
{
    return vec3(float((val&0x000000FF)), float((val&0x0000FF00)>>8U), float ((val&0x00FF0000)>>16U)); // TODO: Review offset, might be wrong and needs to be fixed at vertex level
}

/////////////////////////////////////////////////////////////////////////////////////////////

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

    vec3 normalV0      = convRGB8ToVec(v0.UVNormalTangentFlags.y);
    normalV0          /= 255.0;
    normalV0          -= vec3(0.5);
    normalV0          *= 2.0;

    vec3 normalV1      = convRGB8ToVec(v1.UVNormalTangentFlags.y);
    normalV1          /= 255.0;
    normalV1          -= vec3(0.5);
    normalV1          *= 2.0;

    vec3 normalV2      = convRGB8ToVec(v2.UVNormalTangentFlags.y);
    normalV2          /= 255.0;
    normalV2          -= vec3(0.5);
    normalV2          *= 2.0;

    // Computing the coordinates of the hit position
    vec3 worldPos            = v0.position    * barycentrics.x + v1.position    * barycentrics.y + v2.position    * barycentrics.z;
    vec3 normal0Uncompressed = normalV0;
    vec3 normal1Uncompressed = normalV1;
    vec3 normal2Uncompressed = normalV2;
    normal0Uncompressed      = normalize(normal0Uncompressed);
    normal1Uncompressed      = normalize(normal1Uncompressed);
    normal2Uncompressed      = normalize(normal2Uncompressed);
    vec3 worldNormal         = normal0Uncompressed * barycentrics.x + normal1Uncompressed * barycentrics.y + normal2Uncompressed* barycentrics.z;

    // Transforming the position to world space
    worldPos                 = vec3(gl_ObjectToWorldEXT * vec4(worldPos, 1.0));
    mat3 temp                = mat3(gl_ObjectToWorldEXT);
    mat3 normalMatrix        = mat3(transpose(inverse(temp)));
    prd.distance             = distance(gl_WorldRayOriginEXT, worldPos);
    prd.hitPosition          = worldPos;
    prd.hitNormal            = normalize(normalMatrix * worldNormal);

    prd.v0 = v0.position;
    prd.v1 = v1.position;
    prd.v2 = v2.position;
}
