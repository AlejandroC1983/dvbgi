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
#extension GL_EXT_shader_16bit_storage : require

/////////////////////////////////////////////////////////////////////////////////////////////

hitAttributeEXT vec2 attribs;

struct hitPayload
{
    vec3 hitPosition;
    vec3 hitNormal;
    int isDynamicGeometry;
    int anyHit;
    int isShadowRay;
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
    // TRY ADDING A NEW FIELD "FLAGS" TO ENCODE ALL KIND OF INFORMATION LIKE THE ELEMENT BEING STATIC / DYNAMIC
};

/////////////////////////////////////////////////////////////////////////////////////////////

layout(location = 0) rayPayloadInEXT hitPayload prd;

layout(binding = 6, scalar) buffer Vertices { Vertex v[]; } vertices[];
layout(binding = 7) buffer Indices { uint i[]; } indices[];

layout (binding = 8, scalar) buffer coherent sceneDescriptorBuffer
{
    SceneDescription sceneDescription[ ];
};

/////////////////////////////////////////////////////////////////////////////////////////////

vec3 convRGB8ToVec(uint val)
{
    return vec3(float((val&0x000000FF)), float((val&0x0000FF00)>>8U), float ((val&0x00FF0000)>>16U)); // TODO: Review offset, might be wrong and needs to be fixed at vertex level
}

/////////////////////////////////////////////////////////////////////////////////////////////

void main()
{
    if(prd.isShadowRay == 1)
    {
        prd.anyHit = 1;
        return;
    }

    // Object of this instance
    uint objId = sceneDescription[gl_InstanceID].objId;

    Vertex v0;
    Vertex v1;
    Vertex v2;

    // Indices of the triangle
    ivec3 ind = ivec3(indices[nonuniformEXT(objId)].i[3 * gl_PrimitiveID + 0], 
                      indices[nonuniformEXT(objId)].i[3 * gl_PrimitiveID + 1], 
                      indices[nonuniformEXT(objId)].i[3 * gl_PrimitiveID + 2]);

    // Vertex of the triangle
    v0 = vertices[nonuniformEXT(objId)].v[ind.x];
    v1 = vertices[nonuniformEXT(objId)].v[ind.y];
    v2 = vertices[nonuniformEXT(objId)].v[ind.z];

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
    
    vec3 barycentrics = vec3(1.0 - attribs.x - attribs.y, attribs.x, attribs.y);

    // Computing the coordinates of the hit position
    vec3 worldPos            = v0.position * barycentrics.x + v1.position * barycentrics.y + v2.position * barycentrics.z;
    vec3 normal0Uncompressed = normalize(normalV0);
    vec3 normal1Uncompressed = normalize(normalV1);
    vec3 normal2Uncompressed = normalize(normalV2);
    vec3 worldNormal         = normal0Uncompressed * barycentrics.x + normal1Uncompressed * barycentrics.y + normal2Uncompressed* barycentrics.z;

    // Transforming the position to world space
    worldPos                 = vec3(gl_ObjectToWorldEXT * vec4(worldPos, 1.0));
    prd.hitPosition          = worldPos;
    prd.anyHit               = 1;
    mat3 temp                = mat3(gl_ObjectToWorldEXT);
    mat3 normalMatrix        = mat3(transpose(inverse(temp)));
    prd.hitNormal            = normalize(normalMatrix * worldNormal);
    prd.isDynamicGeometry    = (sceneDescription[gl_InstanceID].flags & 0x00000001) > 0 ? 1 : 0;
}

/////////////////////////////////////////////////////////////////////////////////////////////
