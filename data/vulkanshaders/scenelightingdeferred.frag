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

#extension GL_EXT_shader_8bit_storage : enable

/////////////////////////////////////////////////////////////////////////////////////////////

layout (std140, binding = 0) uniform materialData
{
    vec4 lightPosition;                //!< Light position
    vec4 lightForward;                 //!< Light direction (x,y and z fields)
    vec4 sceneMinEmitterRadiance;      //!< Scene min AABB value (x,y and z fields) and emitter radiance (w field)
    vec4 sceneExtentVoxelSize;         //!< Scene extent (x,y and z fields) and voxelization resolution (w field)
    float irradianceMultiplier;        //!< Equivalent to IRRADIANCE_MULTIPLIER
    float directIrradianceMultiplier;  //!< Equivalent to DIRECT_IRRADIANCE_MULTIPLIER
    float brightness;                  //!< Brightness value to be applied in the shaders used in this technique
    float contrast;                    //!< Contrast value to be applied in the shaders used in this technique
    float saturation;                  //!< Saturation value to be applied in the shaders used in this technique
    float intensity;                   //!< Intensity value to be added to the final color
} myMaterialData;

/////////////////////////////////////////////////////////////////////////////////////////////

/** For debug */
layout (binding = 16) buffer coherent sceneLightingDeferredDebugBuffer
{
   float sceneLightingDeferredDebug[ ];
};

/////////////////////////////////////////////////////////////////////////////////////////////

/** Buffer with the information about what neighbour voxels are lit and the voxel itself
* Encodes whether there is an occupied static and lit voxel neighbour of the one being analysed
* at bit level, with information on each neighbour at bit index:
* Index 0:  offset (-1, -1, -1)
* Index 1:  offset (-1, -1,  0)
* Index 2:  offset (-1, -1,  1)
* Index 3:  offset (-1,  0, -1)
* Index 4:  offset (-1,  0,  0)
* Index 5:  offset (-1,  0,  1)
* Index 6:  offset (-1,  1, -1)
* Index 7:  offset (-1,  1,  0)
* Index 8:  offset (-1,  1,  1)
* Index 9:  offset ( 0, -1, -1)
* Index 10: offset ( 0, -1,  0)
* Index 11: offset ( 0, -1,  1)
* Index 12: offset ( 0,  0, -1)
* Index 13: offset ( 0,  0,  0)
* Index 14: offset ( 0,  0,  1)
* Index 15: offset ( 0,  1, -1)
* Index 16: offset ( 0,  1,  0)
* Index 17: offset ( 0,  1,  1)
* Index 18: offset ( 1, -1, -1)
* Index 19: offset ( 1, -1,  0)
* Index 20: offset ( 1, -1,  1)
* Index 21: offset ( 1,  0, -1)
* Index 22: offset ( 1,  0,  0)
* Index 23: offset ( 1,  0,  1)
* Index 24: offset ( 1, -1, -1)
* Index 25: offset ( 1, -1,  0)
* Index 26: offset ( 1, -1,  1) */
layout (binding = 17) buffer coherent neighbourLitVoxelInformationBuffer
{
    uint neighbourLitVoxelInformation[ ];
};

/////////////////////////////////////////////////////////////////////////////////////////////

/** Buffer of lit test voxels having information on a per-byte approach (each byte has information on whether any 
* voxel face is lit and what voxel faces are lit at bit level, with info [no info][-x][+x][-y][+y][-z][+z][AnyLitVoxelFace]) */
layout (binding = 18) buffer coherent litTestVoxelPerByteBuffer
{
    uint8_t litTestVoxelPerByte[ ];
};

/////////////////////////////////////////////////////////////////////////////////////////////

/** Buffer to know, for a given 3D voxelization volume texture (whose indices are hashed to a unique uint 32-bit)
* if there are fragments at that position or that 3D voxelization volume is empty. */
layout (binding = 20) buffer coherent occupiedStaticVoxelTileBuffer
{
    uint occupiedStaticVoxelTile[ ];
};

/////////////////////////////////////////////////////////////////////////////////////////////

layout(binding = 1)  uniform sampler2D GBufferNormal;
layout(binding = 2)  uniform sampler2D GBufferReflectance;
layout(binding = 3)  uniform sampler2D GBufferPosition;
layout(binding = 4)  uniform sampler3D irradiance3DStaticNegativeX;
layout(binding = 5)  uniform sampler3D irradiance3DStaticPositiveX;
layout(binding = 6)  uniform sampler3D irradiance3DStaticNegativeY;
layout(binding = 7)  uniform sampler3D irradiance3DStaticPositiveY;
layout(binding = 8)  uniform sampler3D irradiance3DStaticNegativeZ;
layout(binding = 9)  uniform sampler3D irradiance3DStaticPositiveZ;
layout(binding = 10) uniform sampler3D irradiance3DDynamicNegativeX;
layout(binding = 11) uniform sampler3D irradiance3DDynamicPositiveX;
layout(binding = 12) uniform sampler3D irradiance3DDynamicNegativeY;
layout(binding = 13) uniform sampler3D irradiance3DDynamicPositiveY;
layout(binding = 14) uniform sampler3D irradiance3DDynamicNegativeZ;
layout(binding = 15) uniform sampler3D irradiance3DDynamicPositiveZ;

layout(binding = 19) uniform usampler3D staticVoxelIndexTexture;

/////////////////////////////////////////////////////////////////////////////////////////////

layout (location = 0) in vec2 inUV;

/////////////////////////////////////////////////////////////////////////////////////////////

layout(location = 0) out vec4 outColor;

/////////////////////////////////////////////////////////////////////////////////////////////

// DEFINES
#define PI 3.14159265359

/////////////////////////////////////////////////////////////////////////////////////////////

// GLOBAL VARIABLES
vec3  g_sceneMin;                //!< Scene min
vec3  g_sceneExtent;             //!< Scene extent
vec3  g_voxelSizeThreeDimension; //!< Size of the voxelization texture
float g_voxelSize;               //!< Size of the voxelization texture
uint  g_voxelSizeUint;           //!< Size of the voxelization texture (unsigned int)
int   g_voxelSizeInt;            //!< Size of the voxelization texture (int)
float g_irradianceMultiplier;    //!< Value taken from raster flag IRRADIANCE_MULTIPLIER and divided by 100000

// TODO: Refactor g_irradianceMultiplier

vec3 arrayDirection[6] =
{
    vec3(-1.0,  0.0,  0.0),
    vec3( 1.0,  0.0,  0.0),
    vec3( 0.0, -1.0,  0.0),
    vec3( 0.0,  1.0,  0.0),
    vec3( 0.0,  0.0, -1.0),
    vec3( 0.0,  0.0,  1.0),
};

/////////////////////////////////////////////////////////////////////////////////////////////

/** Computes the index in arrayDirection of the most similar direction compared with the direction parameter
* @param direction [in] direction to compute index of most similar direction
* @return index in arrayDirection of closest direction */
int getArrayDirectionIndex(vec3 direction)
{
    int indexClosest   = -1;
    float minimumAngle = -1.0;
    float minimumAngleTemp;

    for(int i = 0; i < 6; ++i)
    {
        minimumAngleTemp = dot(arrayDirection[i], direction);

        if(minimumAngleTemp > minimumAngle)
        {
            minimumAngle = minimumAngleTemp;
            indexClosest = i;
        }
    }

    return indexClosest;
}

/////////////////////////////////////////////////////////////////////////////////////////////
 
/** Computes texture space coordinates of the world coordinates given as parameter
* @param worldCoordinates [in] world coordinates
* @return texture space coordinates of the world coordinates given as parameter */
 uvec3 worldToVoxelSpace(vec3 worldCoordinates)
 {
     vec3 result = worldCoordinates;
     result     -= g_sceneMin;
     result     /= g_sceneExtent;
     result     *= g_voxelSizeThreeDimension;
 
     return uvec3(uint(result.x), uint(result.y), uint(result.z));
 }

/////////////////////////////////////////////////////////////////////////////////////////////

vec3 interpolateIrradiance8VoxelTest(vec3 fragmentPosition, vec3 normalDirection, bool dynamicSceneElement)
{
    //vec3 voxelCoordinatesFloat = worldToVoxelSpaceFloat(fragmentPosition) / g_voxelSize;
    vec3 voxelCoordinatesFloat = worldToVoxelSpace(fragmentPosition) / g_voxelSize;
    int index                  = getArrayDirectionIndex(normalDirection);

    if(!dynamicSceneElement)
    {
        switch (index)
        {
            case 0:
            {
                return texelFetch(irradiance3DStaticNegativeX, ivec3(voxelCoordinatesFloat * g_voxelSize), 0).xyz;
                break;
            }
            case 1:
            {
                return texelFetch(irradiance3DStaticPositiveX, ivec3(voxelCoordinatesFloat * g_voxelSize), 0).xyz;
                break;
            }
            case 2:
            {
                return texelFetch(irradiance3DStaticNegativeY, ivec3(voxelCoordinatesFloat * g_voxelSize), 0).xyz;
                break;
            }
            case 3:
            {
                return texelFetch(irradiance3DStaticPositiveY, ivec3(voxelCoordinatesFloat * g_voxelSize), 0).xyz;
                break;
            }
            case 4:
            {
                return texelFetch(irradiance3DStaticNegativeZ, ivec3(voxelCoordinatesFloat * g_voxelSize), 0).xyz;
                break;
            }
            case 5:
            {
                return texelFetch(irradiance3DStaticPositiveZ, ivec3(voxelCoordinatesFloat * g_voxelSize), 0).xyz;
                break;
            }
        };
    }
    else
    {
        switch (index)
        {
            case 0:
            {
                return texelFetch(irradiance3DDynamicNegativeX, ivec3(voxelCoordinatesFloat * g_voxelSize), 0).xyz;
                break;
            }
            case 1:
            {
                return texelFetch(irradiance3DDynamicPositiveX, ivec3(voxelCoordinatesFloat * g_voxelSize), 0).xyz;
                break;
            }
            case 2:
            {
                return texelFetch(irradiance3DDynamicNegativeY, ivec3(voxelCoordinatesFloat * g_voxelSize), 0).xyz;
                break;
            }
            case 3:
            {
                return texelFetch(irradiance3DDynamicPositiveY, ivec3(voxelCoordinatesFloat * g_voxelSize), 0).xyz;
                break;
            }
            case 4:
            {
                return texelFetch(irradiance3DDynamicNegativeZ, ivec3(voxelCoordinatesFloat * g_voxelSize), 0).xyz;
                break;
            }
            case 5:
            {
                return texelFetch(irradiance3DDynamicPositiveZ, ivec3(voxelCoordinatesFloat * g_voxelSize), 0).xyz;
                break;
            }
        };
    }
}

/////////////////////////////////////////////////////////////////////////////////////////////
 
/** Computes texture space coordinates of the world coordinates given as parameter as float values
* @param worldCoordinates [in] world coordinates
* @return texture space coordinates of the world coordinates given as parameter */
vec3 worldToVoxelSpaceFloat(vec3 worldCoordinates)
{
    vec3 result = worldCoordinates;
    result     -= g_sceneMin;
    result     /= g_sceneExtent;
    result     *= g_voxelSizeThreeDimension;
 
    return vec3(result.x, result.y, result.z);
}

/////////////////////////////////////////////////////////////////////////////////////////////

/** Interplate the irradiance of the world coordinates given by worldPosition parameter
* with normal direction worldNormal with the irradiance information per voxel face available in the 
* irradiance textures for static and dynamic irradiance
* @param worldPosition       [in] world coordinates to interpolate irradiance
* @param worldNormal         [in] normal direction to interpolate irradiance
* @param dynamicSceneElement [in] flag to know whether to do the computations for dynamic or static irradiance information
* @return interpolated irradiance */
vec3 interpolateDirectionIrradiance(vec3 worldPosition, vec3 worldNormal, bool dynamicSceneElement)
{
    vec3 voxelCoordinatesFloat = worldToVoxelSpaceFloat(worldPosition) / g_voxelSize;

    float xAxisDotResult;
    float yAxisDotResult;
    float zAxisDotResult;

    vec3 irradianceX;
    vec3 irradianceY;
    vec3 irradianceZ;

    if(dot(worldNormal, vec3(-1.0, 0.0, 0.0)) > 0.0)
    {
        // Direction facing -x

        if(dot(worldNormal, vec3(0.0, -1.0, 0.0)) > 0.0)
        {
            // Direction facing -x and -y

            if(dot(worldNormal, vec3(0.0, 0.0, -1.0)) > 0.0)
            {
                // Direction facing -x, -y and -z
                xAxisDotResult = dot(worldNormal, vec3(-1.0, 0.0, 0.0));
                yAxisDotResult = dot(worldNormal, vec3( 0.0,-1.0, 0.0));
                zAxisDotResult = dot(worldNormal, vec3( 0.0, 0.0,-1.0));

                irradianceX = dynamicSceneElement ? texture(irradiance3DDynamicNegativeX, voxelCoordinatesFloat).xyz : texture(irradiance3DStaticNegativeX, voxelCoordinatesFloat).xyz;
                irradianceY = dynamicSceneElement ? texture(irradiance3DDynamicNegativeY, voxelCoordinatesFloat).xyz : texture(irradiance3DStaticNegativeY, voxelCoordinatesFloat).xyz;
                irradianceZ = dynamicSceneElement ? texture(irradiance3DDynamicNegativeZ, voxelCoordinatesFloat).xyz : texture(irradiance3DStaticNegativeZ, voxelCoordinatesFloat).xyz;
            }
            else
            {
                // Direction facing -x, -y and +z
                xAxisDotResult = dot(worldNormal, vec3(-1.0, 0.0, 0.0));
                yAxisDotResult = dot(worldNormal, vec3( 0.0,-1.0, 0.0));
                zAxisDotResult = dot(worldNormal, vec3( 0.0, 0.0, 1.0));

                irradianceX = dynamicSceneElement ? texture(irradiance3DDynamicNegativeX, voxelCoordinatesFloat).xyz : texture(irradiance3DStaticNegativeX, voxelCoordinatesFloat).xyz;
                irradianceY = dynamicSceneElement ? texture(irradiance3DDynamicNegativeY, voxelCoordinatesFloat).xyz : texture(irradiance3DStaticNegativeY, voxelCoordinatesFloat).xyz;
                irradianceZ = dynamicSceneElement ? texture(irradiance3DDynamicPositiveZ, voxelCoordinatesFloat).xyz : texture(irradiance3DStaticPositiveZ, voxelCoordinatesFloat).xyz;
            }
        }
        else
        {
            // Direction facing -x and +y

            if(dot(worldNormal, vec3(0.0, 0.0, -1.0)) > 0.0)
            {
                // Direction facing -x, +y and -z
                xAxisDotResult = dot(worldNormal, vec3(-1.0, 0.0, 0.0));
                yAxisDotResult = dot(worldNormal, vec3( 0.0, 1.0, 0.0));
                zAxisDotResult = dot(worldNormal, vec3( 0.0, 0.0,-1.0));

                irradianceX = dynamicSceneElement ? texture(irradiance3DDynamicNegativeX, voxelCoordinatesFloat).xyz : texture(irradiance3DStaticNegativeX, voxelCoordinatesFloat).xyz;
                irradianceY = dynamicSceneElement ? texture(irradiance3DDynamicPositiveY, voxelCoordinatesFloat).xyz : texture(irradiance3DStaticPositiveY, voxelCoordinatesFloat).xyz;
                irradianceZ = dynamicSceneElement ? texture(irradiance3DDynamicNegativeZ, voxelCoordinatesFloat).xyz : texture(irradiance3DStaticNegativeZ, voxelCoordinatesFloat).xyz;
            }
            else
            {
                // Direction facing -x, +y and +z
                xAxisDotResult = dot(worldNormal, vec3(-1.0, 0.0, 0.0));
                yAxisDotResult = dot(worldNormal, vec3( 0.0, 1.0, 0.0));
                zAxisDotResult = dot(worldNormal, vec3( 0.0, 0.0, 1.0));

                irradianceX = dynamicSceneElement ? texture(irradiance3DDynamicNegativeX, voxelCoordinatesFloat).xyz : texture(irradiance3DStaticNegativeX, voxelCoordinatesFloat).xyz;
                irradianceY = dynamicSceneElement ? texture(irradiance3DDynamicPositiveY, voxelCoordinatesFloat).xyz : texture(irradiance3DStaticPositiveY, voxelCoordinatesFloat).xyz;
                irradianceZ = dynamicSceneElement ? texture(irradiance3DDynamicPositiveZ, voxelCoordinatesFloat).xyz : texture(irradiance3DStaticPositiveZ, voxelCoordinatesFloat).xyz;
            }
        }
    }
    else
    {
        // Direction facing +x

        if(dot(worldNormal, vec3(0.0, -1.0, 0.0)) > 0.0)
        {
            // Direction facing +x and -y

            if(dot(worldNormal, vec3(0.0, 0.0, -1.0)) > 0.0)
            {
                // Direction facing +x, -y and -z
                xAxisDotResult = dot(worldNormal, vec3( 1.0, 0.0, 0.0));
                yAxisDotResult = dot(worldNormal, vec3( 0.0,-1.0, 0.0));
                zAxisDotResult = dot(worldNormal, vec3( 0.0, 0.0,-1.0));

                irradianceX = dynamicSceneElement ? texture(irradiance3DDynamicPositiveX, voxelCoordinatesFloat).xyz : texture(irradiance3DStaticPositiveX, voxelCoordinatesFloat).xyz;
                irradianceY = dynamicSceneElement ? texture(irradiance3DDynamicNegativeY, voxelCoordinatesFloat).xyz : texture(irradiance3DStaticNegativeY, voxelCoordinatesFloat).xyz;
                irradianceZ = dynamicSceneElement ? texture(irradiance3DDynamicNegativeZ, voxelCoordinatesFloat).xyz : texture(irradiance3DStaticNegativeZ, voxelCoordinatesFloat).xyz;
            }
            else
            {
                // Direction facing +x, -y and +z
                xAxisDotResult = dot(worldNormal, vec3( 1.0, 0.0, 0.0));
                yAxisDotResult = dot(worldNormal, vec3( 0.0,-1.0, 0.0));
                zAxisDotResult = dot(worldNormal, vec3( 0.0, 0.0, 1.0));

                irradianceX = dynamicSceneElement ? texture(irradiance3DDynamicPositiveX, voxelCoordinatesFloat).xyz : texture(irradiance3DStaticPositiveX, voxelCoordinatesFloat).xyz;
                irradianceY = dynamicSceneElement ? texture(irradiance3DDynamicNegativeY, voxelCoordinatesFloat).xyz : texture(irradiance3DStaticNegativeY, voxelCoordinatesFloat).xyz;
                irradianceZ = dynamicSceneElement ? texture(irradiance3DDynamicPositiveZ, voxelCoordinatesFloat).xyz : texture(irradiance3DStaticPositiveZ, voxelCoordinatesFloat).xyz;
            }
        }
        else
        {
            // Direction facing +x and +y

            if(dot(worldNormal, vec3(0.0, 0.0, -1.0)) > 0.0)
            {
                // Direction facing +x, +y and -z
                xAxisDotResult = dot(worldNormal, vec3( 1.0, 0.0, 0.0));
                yAxisDotResult = dot(worldNormal, vec3( 0.0, 1.0, 0.0));
                zAxisDotResult = dot(worldNormal, vec3( 0.0, 0.0,-1.0));

                irradianceX = dynamicSceneElement ? texture(irradiance3DDynamicPositiveX, voxelCoordinatesFloat).xyz : texture(irradiance3DStaticPositiveX, voxelCoordinatesFloat).xyz;
                irradianceY = dynamicSceneElement ? texture(irradiance3DDynamicPositiveY, voxelCoordinatesFloat).xyz : texture(irradiance3DStaticPositiveY, voxelCoordinatesFloat).xyz;
                irradianceZ = dynamicSceneElement ? texture(irradiance3DDynamicNegativeZ, voxelCoordinatesFloat).xyz : texture(irradiance3DStaticNegativeZ, voxelCoordinatesFloat).xyz;
            }
            else
            {
                // Direction facing +x, +y and +z
                xAxisDotResult = dot(worldNormal, vec3( 1.0, 0.0, 0.0));
                yAxisDotResult = dot(worldNormal, vec3( 0.0, 1.0, 0.0));
                zAxisDotResult = dot(worldNormal, vec3( 0.0, 0.0, 1.0));

                irradianceX = dynamicSceneElement ? texture(irradiance3DDynamicPositiveX, voxelCoordinatesFloat).xyz : texture(irradiance3DStaticPositiveX, voxelCoordinatesFloat).xyz;
                irradianceY = dynamicSceneElement ? texture(irradiance3DDynamicPositiveY, voxelCoordinatesFloat).xyz : texture(irradiance3DStaticPositiveY, voxelCoordinatesFloat).xyz;
                irradianceZ = dynamicSceneElement ? texture(irradiance3DDynamicPositiveZ, voxelCoordinatesFloat).xyz : texture(irradiance3DStaticPositiveZ, voxelCoordinatesFloat).xyz;
            }
        }
    }

    // Normalize to interval [0, 1], with 1.0 for the direction with angle 0 radians and 1.0 for those directions with angle PI / 2.0 radians
    xAxisDotResult = 1.0 - (acos(xAxisDotResult) / (PI * 0.5));
    yAxisDotResult = 1.0 - (acos(yAxisDotResult) / (PI * 0.5));
    zAxisDotResult = 1.0 - (acos(zAxisDotResult) / (PI * 0.5));

    vec3 weight = normalize(vec3(xAxisDotResult, yAxisDotResult, zAxisDotResult));

    vec3 interpolatedIrradiance = weight.x * irradianceX + weight.y * irradianceY + weight.z * irradianceZ;

    return interpolatedIrradiance;
}

/////////////////////////////////////////////////////////////////////////////////////////////

/** Compute the reflected irradiance from a directional emitter with direction lightDirection (from the surface being analysed)
* for a lambertian surface with normal direction surfaceNormal
* https://bheisler.github.io/post/writing-raytracer-in-rust-part-2/
* https://www.scratchapixel.com/lessons/3d-basic-rendering/introduction-to-shading/diffuse-lambertian-shading.html
* @param lightDirection  [in] direction towards the light
* @param surfaceNormal   [in] normal direction of the surface being analysed
* @param emitterRadiance [in] emitter radiance
* @return form factor value */
float reflectedIrradianceDirectionalLight(vec3 lightDirection, vec3 surfaceNormal, float emitterRadiance)
{
    float reflectedIrradiance = dot(lightDirection, surfaceNormal);
    reflectedIrradiance      *= emitterRadiance;
    reflectedIrradiance      /= PI;
    
    return reflectedIrradiance;
}

/////////////////////////////////////////////////////////////////////////////////////////////

/** Returns the form factor from a differential area with normal normalDA and to a
* differential area with position positionDB and normal normalDB
* NOTE: taken from J. R. Wallace, K. A. Elmquist, and E. A. Haines. 1989. A Ray tracing algorithm for progressive radiosity
* @param normalDA   [in] Normal of the differential area A
* @param positionDA [in] Point position of the differential area A
* @param normalDB   [in] Normal of the differential area B
* @param positionDB [in] Point position of the differential area B
* @param nSamples   [in] Hack to avoid close point reach too high values
* @return form factor value */
float differentialAreaFormFactor(vec3 normalDA, vec3 positionDA, vec3 normalDB, vec3 positionDB)
{
  vec3 dAtodBDirection = positionDB - positionDA;
  float distanceSq     = dot(dAtodBDirection, dAtodBDirection);
  dAtodBDirection      = normalize(dAtodBDirection);
  float cosTheta1      = dot(normalDA, dAtodBDirection);
  cosTheta1            = clamp(cosTheta1, 0.0, 1.0);
  float cosTheta2      = dot(normalDB, -1.0 * dAtodBDirection);
  cosTheta2            = clamp(cosTheta2, 0.0, 1.0);
  return (cosTheta1 * cosTheta2) / (PI * distanceSq);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////

/** Test whether the voxel at index indexHashed has any face set as lit
* @param indexHashed [in] voxel index to test if it has any voxel face as lit
* @return true if any voxel face as been set as lit, false otherwise */
bool isVoxelLit(uint indexHashed)
{
    return (uint(litTestVoxelPerByte[indexHashed]) > 0);
}

/////////////////////////////////////////////////////////////////////////////////////////////

/** hashed value for texture coordinates given as parameter in a voxelization of size given as parameter
* @param texcoord  [in] texture coordinates
* @param voxelSize [in] voxelization size
* @return hashed value */
uint getHashedIndex(uvec3 texcoord)
{
    return texcoord.x * g_voxelSizeUint * g_voxelSizeUint + texcoord.y * g_voxelSizeUint + texcoord.z;
}

/////////////////////////////////////////////////////////////////////////////////////////////

/** Search in voxelHashedPositionCompactedBuffer for the index of the hashed value
* of the texcoord parameter.
* @param texcoord       [in] 3D texture coordinates to find index for
* @param hashedPosition [in] texcoord parameter hashed texture coordinates
* @return uvec2 with index in .x field and test result in .y field */
uvec2 findHashedCompactedPositionIndex(uvec3 texcoord, uint hashedPosition)
{
    uint index = texelFetch(staticVoxelIndexTexture, ivec3(texcoord), 0).r;

    if(index != 4294967295)
    {
        return uvec2(index, 1);
    }

    return uvec2(0, 0);
}

/////////////////////////////////////////////////////////////////////////////////////////////

/** Compute the tile coordinates for the ones given as paramete
* @param coordinates [in] coordinates to compute tile coordinates from
* @return tile coordinates for the ones given as parameter */
ivec3 getTileCoordinates(ivec3 coordinates)
{
    return coordinates / ivec3(TILE_SIDE); // Currently tiles have size 2^3
}

/////////////////////////////////////////////////////////////////////////////////////////////

/** Hash the given tile coordinates, to map to the irradiancePaddingTagTilesBuffer buffer
* @param coordinates [in] coordinates to compute tile coordinates from
* @return hashed index of the tile coordinates given as parameter */
uint getTileHashedIndex(ivec3 tileCoordinates)
{
    int tileSize = g_voxelSizeInt / TILE_SIDE; // Currently tiles have size 2^
    return tileCoordinates.x * tileSize * tileSize + tileCoordinates.y * tileSize + tileCoordinates.z;
}

/////////////////////////////////////////////////////////////////////////////////////////////

// https://www.shadertoy.com/view/XdcXzn
mat4 brightnessMatrix( float brightness )
{
    return mat4( 1, 0, 0, 0,
                 0, 1, 0, 0,
                 0, 0, 1, 0,
                 brightness, brightness, brightness, 1 );
}

/////////////////////////////////////////////////////////////////////////////////////////////

mat4 contrastMatrix( float contrast )
{
    float t = ( 1.0 - contrast ) / 2.0;
    
    return mat4( contrast, 0, 0, 0,
                 0, contrast, 0, 0,
                 0, 0, contrast, 0,
                 t, t, t, 1 );
}

/////////////////////////////////////////////////////////////////////////////////////////////

mat4 saturationMatrix( float saturation )
{
    vec3 luminance    = vec3( 0.3086, 0.6094, 0.0820 );    
    float oneMinusSat = 1.0 - saturation;
    vec3 red          = vec3( luminance.x * oneMinusSat );
    red              += vec3( saturation, 0, 0 );
    vec3 green        = vec3( luminance.y * oneMinusSat );
    green            += vec3( 0, saturation, 0 );
    vec3 blue         = vec3( luminance.z * oneMinusSat );
    blue             += vec3( 0, 0, saturation );
    
    return mat4( red,     0,
                 green,   0,
                 blue,    0,
                 0, 0, 0, 1 );
}

/////////////////////////////////////////////////////////////////////////////////////////////

vec3 contrastSaturationBrightness(vec3 color, float brt, float sat, float con)
{
    // Increase or decrease theese values to adjust r, g and b color channels seperately
    const float AvgLumR = 0.5;
    const float AvgLumG = 0.5;
    const float AvgLumB = 0.5;
    
    const vec3 LumCoeff = vec3(0.2125, 0.7154, 0.0721);
    
    vec3 AvgLumin  = vec3(AvgLumR, AvgLumG, AvgLumB);
    vec3 brtColor  = color * brt;
    vec3 intensity = vec3(dot(brtColor, LumCoeff));
    vec3 satColor  = mix(intensity, brtColor, sat);
    vec3 conColor  = mix(AvgLumin, satColor, con);
    
    return conColor;
}

/////////////////////////////////////////////////////////////////////////////////////////////

void main(void)
{
    vec4 positionWorldIsDynamic = texture(GBufferPosition, inUV);
    vec4 reflectanceShadow      = texture(GBufferReflectance, inUV);
    vec4 normalCompressedShadow = texture(GBufferNormal, inUV);
    float shadowResult          = (reflectanceShadow.w != 0.0) ? 0.0 : 1.0; // .w field has value 1.0 if there was any intersection during the shadow ray being casted
    vec3 normalWorld            = normalize(2.0 * (normalCompressedShadow.xyz - vec3(0.5))); // Uncompress normal

    g_sceneMin                 = myMaterialData.sceneMinEmitterRadiance.xyz;
    g_sceneExtent              = myMaterialData.sceneExtentVoxelSize.xyz;
    g_voxelSizeThreeDimension  = vec3(myMaterialData.sceneExtentVoxelSize.w);
    g_voxelSize                = g_voxelSizeThreeDimension.x;
    g_voxelSizeUint            = uint(g_voxelSize);
    g_voxelSizeInt             = int(g_voxelSizeUint);
    g_irradianceMultiplier     = (positionWorldIsDynamic.w == 1.0) ? (float(DYNAMIC_IRRADIANCE_MULTIPLIER) + float(myMaterialData.irradianceMultiplier)) / 100000.0 : float(IRRADIANCE_MULTIPLIER) / 100000.0;

    float directMultiplier     = myMaterialData.directIrradianceMultiplier / 1000.0;

    vec3 lightDirection        = normalize(myMaterialData.lightPosition.xyz); // Light direction is currently a vectro from the origin of coordinates towards the position of the emitter
    float reflectedIrradiance  = reflectedIrradianceDirectionalLight(lightDirection, normalWorld, myMaterialData.sceneMinEmitterRadiance.w);
    reflectedIrradiance        = max(0.0, reflectedIrradiance);
    vec3 irradiance            = interpolateDirectionIrradiance(positionWorldIsDynamic.xyz, normalWorld, (positionWorldIsDynamic.w == 1.0) ? true : false);

    outColor.xyz               = shadowResult * reflectedIrradiance * reflectanceShadow.xyz * directMultiplier + reflectanceShadow.xyz * irradiance * g_irradianceMultiplier;

    uvec3 voxelCoordinatesTex  = worldToVoxelSpace(positionWorldIsDynamic.xyz);

    outColor.xyz = contrastSaturationBrightness(outColor.xyz, myMaterialData.brightness, myMaterialData.saturation, myMaterialData.contrast);

    outColor.x = pow(outColor.x, 1.0 / 2.2);
    outColor.y = pow(outColor.y, 1.0 / 2.2);
    outColor.z = pow(outColor.z, 1.0 / 2.2);
    outColor.w = 1.0;
}

/////////////////////////////////////////////////////////////////////////////////////////////
