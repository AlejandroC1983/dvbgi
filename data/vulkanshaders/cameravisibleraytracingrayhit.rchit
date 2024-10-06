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
    vec3 hitPosition;
    int isDynamicGeometry;
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
    mat4 viewMatrixInverse;           //!< Inverse of the view matrix
    mat4 projectionMatrixInverse;     //!< Inverse of the projection matrix
    vec4 cameraPosition;              //!< Main camera position
    vec4 sceneMin;                    //!< Scene min AABB value
    vec4 sceneExtentVoxelizationSize; //!< Scene extent and voxel size in w field
    vec4 lightPosition;               //!< Light position in world coordinates
    vec4 lightForward;                //!< Light direction in world coordinates
} myMaterialData;

/////////////////////////////////////////////////////////////////////////////////////////////

layout(location = 0) rayPayloadInEXT hitPayload prd;
layout(binding = 18, scalar) buffer Vertices { Vertex v[]; } vertices[];
layout(binding = 19) buffer Indices { uint i[]; } indices[];

layout (binding = 20, scalar) buffer coherent sceneDescriptorBuffer
{
    SceneDescription sceneDescription[ ];
};

layout(binding = 21) uniform sampler2D textureArrayCombined[];

/////////////////////////////////////////////////////////////////////////////////////////////

// Taken from https://blog.yiningkarlli.com/2018/10/bidirectional-mipmap.html
vec4 calculateDifferentialSurfaceForTriangle(in vec3 p0, in vec3 p1, in vec3 p2, in vec2 uv0, vec2 uv1, vec2 uv2)
{
    vec2 duv02        = uv0 - uv2;
    vec2 duv12        = uv1 - uv2;
    float determinant = duv02.x * duv12.y - duv02.y * duv12.x;

    vec3 dpdu;
    vec3 dpdv;

    vec3 dp02 = p0 - p2;
    vec3 dp12 = p1 - p2;
    if (abs(determinant) == 0.0f)
    {
        vec3 ng = normalize(cross(p2 - p0, p1 - p0));

        if (abs(ng.x) > abs(ng.y))
        {
            dpdu = vec3(-ng.z, 0, ng.x) / sqrt(ng.x * ng.x + ng.z * ng.z);
        }
        else
        {
            dpdu = vec3(0, ng.z, -ng.y) / sqrt(ng.y * ng.y + ng.z * ng.z);
        }

        dpdv = cross(ng, dpdu);
    }
    else
    {
        float invdet = 1.0 / determinant;
        dpdu         = ( duv12.y * dp02 - duv02.y * dp12) * invdet;
        dpdv         = (-duv12.x * dp02 + duv02.x * dp12) * invdet;
    }

    return vec4(dpdu, dpdv);
}

/////////////////////////////////////////////////////////////////////////////////////////////
// Taken from "Texture Level of Detail Strategies for Real-Time Ray Tracing" https://link.springer.com/content/pdf/10.1007/978-1-4842-4427-2_20.pdf
// Explained as well in "Improved Shader and Texture Level of Detail Using Ray Cones" https://www.jcgt.org/published/0010/01/01/paper-lowres.pdf
float sampleConeMipMapComputation(vec3 v0, vec3 v1, vec3 v2, vec2 t0, vec2 t1, vec2 t2, float distanceToSample, vec3 triangleNormal, vec3 rayDirection)
{
    float textureResolutionWidth  = 1024.0; // TODO: Change
    float textureResolutionHeight = 1024.0; // TODO: Change
    float ta                      = textureResolutionWidth * textureResolutionHeight * abs( (t1.x - t0.x) * (t2.y - t0.y) - (t2.x - t0.x) * (t1.y - t0.y) );
    float pa                      = length(cross(vec3(v1 - v0), vec3(v2 - v0)));
    float delta                   = 0.5 * log2(ta / pa);
    float mipLevel                = delta + log2(distanceToSample) - log2(abs(dot(triangleNormal, -1.0 * rayDirection))); // TODO clamp log2(dot());

    prd.value0 = delta;
    prd.value1 = log2(distanceToSample);
    prd.value2 = -1.0 * log2(abs(dot(triangleNormal, -1.0 * rayDirection)));
    prd.value3 = mipLevel;

    return mipLevel;
}

/////////////////////////////////////////////////////////////////////////////////////////////

float computeTextureLOD(vec3 v0, vec3 v1, vec3 v2, vec2 t0, vec2 t1, vec2 t2, float distanceToSample, vec3 triangleNormal, vec3 rayDirection)
{
    float textureResolutionWidth  = 1024.0; // TODO: Change
    float textureResolutionHeight = 1024.0; // TODO: Change
    float ta     = textureResolutionWidth * textureResolutionHeight * abs( (t1.x - t0.x) * (t2.y - t0.y) - (t2.x - t0.x) * (t1.y - t0.y) );
    float pa     = length(cross(vec3(v1 - v0), vec3(v2 - v0))); // World space computation equivalent
    float delta  = 0.5 * log2(ta / pa);
    float lambda = delta + 0.5 * log2(textureResolutionWidth * textureResolutionHeight) - log2(abs(dot(rayDirection, triangleNormal)));

    prd.value0 = t0.x;
    prd.value1 = t0.y;
    prd.value2 = t1.x;
    prd.value3 = t1.y;

    return lambda;
}

/////////////////////////////////////////////////////////////////////////////////////////////

vec3 convRGB8ToVec(uint val)
{
    return vec3(float((val&0x000000FF)), float((val&0x0000FF00)>>8U), float ((val&0x00FF0000)>>16U)); // TODO: Review offset, might be wrong and needs to be fixed at vertex level
}

/////////////////////////////////////////////////////////////////////////////////////////////

/** Sample a BC5 texture at the coordinates given as parameter for the texture index given as
* parameter and reconstruct the normal direction
* @param uv           [in] texture coordinates
* @param textureIndex [in] texture index
* @param mipLevel     [in] mip level to sample
* @return normal value */
vec3 sampleBC5Normal(vec2 uv, uint textureIndex, float mipLevel)
{
    vec2 normalSampled  = textureLod(textureArrayCombined[textureIndex], uv, mipLevel).xy;
    vec3 normalUnpacked = vec3((normalSampled.xy * 2.0) - 1.0, 0.0);
    normalUnpacked.z    = sqrt(1.0 - dot(normalUnpacked.xy, normalUnpacked.xy));
    return normalize(normalUnpacked);
}

/////////////////////////////////////////////////////////////////////////////////////////////

// Taken from https://blog.yiningkarlli.com/2018/10/bidirectional-mipmap.html and https://www.cs.cornell.edu/courses/cs4620/2018sp/slides/22antialiasing.pdf
float mipLevelFromDifferentialSurface(vec2 dFdx, vec2 dFdy)
{
    float width = max(max(dFdx.x, dFdy.x), max(dFdx.y, dFdy.y));
    float level = log2(1.0 + sqrt(dFdx.x * dFdx.x + dFdx.y * dFdx.y + dFdy.x * dFdy.x + dFdy.y + dFdy.y));

    if(isnan(level) || (level < 0.0))
    {
        return 0.0;
    }

    return level;
}

/////////////////////////////////////////////////////////////////////////////////////////////

void main()
{
    // Object of this instance
    uint objId = sceneDescription[gl_InstanceID].objId;

    Vertex v0;
    Vertex v1;
    Vertex v2;

    // Indices of the triangle
    ivec3 ind = ivec3(indices[nonuniformEXT(objId)].i[3 * gl_PrimitiveID + 0],   //
                      indices[nonuniformEXT(objId)].i[3 * gl_PrimitiveID + 1],   //
                      indices[nonuniformEXT(objId)].i[3 * gl_PrimitiveID + 2]);  //

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

    vec3 tangentV0     = convRGB8ToVec(v0.UVNormalTangentFlags.z);
    tangentV0         /= 255.0;
    tangentV0         -= vec3(0.5);
    tangentV0         *= 2.0;

    vec3 tangentV1     = convRGB8ToVec(v1.UVNormalTangentFlags.z);
    tangentV1         /= 255.0;
    tangentV1         -= vec3(0.5);
    tangentV1         *= 2.0;

    vec3 tangentV2     = convRGB8ToVec(v2.UVNormalTangentFlags.z);
    tangentV2         /= 255.0;
    tangentV2         -= vec3(0.5);
    tangentV2         *= 2.0;

    vec3 barycentrics            = vec3(1.0 - attribs.x - attribs.y, attribs.x, attribs.y);

    // Compute the coordinates of the hit position
    vec3 worldPos                = v0.position * barycentrics.x + v1.position * barycentrics.y + v2.position * barycentrics.z;
    worldPos                     = vec3(gl_ObjectToWorldEXT * vec4(worldPos, 1.0));
    prd.hitPosition              = worldPos;
    vec3 normalWorld             = normalV0    * barycentrics.x + normalV1    * barycentrics.y + normalV2    * barycentrics.z;
    vec3 tangentWorld            = tangentV0   * barycentrics.x + tangentV1   * barycentrics.y + tangentV2   * barycentrics.z;
    vec2 uv0                     = unpackHalf2x16(v0.UVNormalTangentFlags.x);
    vec2 uv1                     = unpackHalf2x16(v1.UVNormalTangentFlags.x);
    vec2 uv2                     = unpackHalf2x16(v2.UVNormalTangentFlags.x);    
    vec2 uv                      = uv0 * barycentrics.x + uv1 * barycentrics.y + uv2 * barycentrics.z;

    uint flags                   = sceneDescription[gl_InstanceID].flags;
    uint textureIndexReflectance = 2 * (flags >> 16);
    uint textureIndexNormal      = textureIndexReflectance + 1;

    normalWorld                  = transpose(inverse(mat3(gl_ObjectToWorldEXT))) * normalWorld;
    normalWorld                  = normalize(normalWorld);
    tangentWorld                 = transpose(inverse(mat3(gl_ObjectToWorldEXT))) * tangentWorld;
    tangentWorld                 = normalize(tangentWorld);

    vec3 binormalWorld           = normalize(cross(normalWorld,tangentWorld));
    mat3 rotMat                  = mat3(tangentWorld, binormalWorld, normalWorld); //Construct the texture-space matrix and rotate the normal into the world space

    float distanceToSample       = length(prd.rayOrigin - worldPos);
    vec3 rayDirection            = normalize(prd.rayOrigin - worldPos);

    float mipLevel               = computeTextureLOD(v0.position, v1.position, v2.position, uv0, uv1, uv2, distanceToSample, normalWorld /*Should this be the normal map normal*/, rayDirection);

    prd.mipLevelValue            = mipLevel;

    vec3 normalSampled           = sampleBC5Normal(uv, textureIndexNormal, mipLevel);
    vec3 normalSampledWorld      = normalize(rotMat * normalSampled);

    // Compress normal to integer with eight bits per channel
    normalSampledWorld          *= 0.5;
    normalSampledWorld          += vec3(0.5);
    prd.normalWorld              = normalSampledWorld;
    prd.reflectance              = textureLod(textureArrayCombined[textureIndexReflectance], uv, mipLevel).xyz;

    // Transform the position to world space
    
    prd.anyHit      = 1;
    prd.isDynamicGeometry = (sceneDescription[gl_InstanceID].flags & 0x00000001) > 0 ? 1 : 0;
}

/////////////////////////////////////////////////////////////////////////////////////////////
