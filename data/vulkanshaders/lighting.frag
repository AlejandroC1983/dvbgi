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

#extension GL_EXT_ray_query : enable
#extension GL_ARB_separate_shader_objects : enable
#extension GL_EXT_shader_8bit_storage : enable
#extension GL_EXT_shader_explicit_arithmetic_types_int8 : enable

/////////////////////////////////////////////////////////////////////////////////////////////

// TODO: Remove not needed
layout(location = 0) in vec2 uv;
layout(location = 1) in vec3 positionVS;
layout(location = 2) in vec3 worldNormalVS;
layout(location = 3) in vec3 worldTangentVS;
layout(location = 4) in vec3 lightDirection;
layout(location = 5) flat in float isDynamicSceneElement;

layout(location = 0) out vec4 outColor;

/////////////////////////////////////////////////////////////////////////////////////////////

layout (std140, binding = 2) uniform materialData
{
    vec4 voxelSize;                   //!< Size of the voxelization texture
    vec4 sceneMin;                    //!< Scene min AABB value
    vec4 sceneExtent;                 //!< Scene extent
    vec4 lightPosition;               //!< Light position
    mat4 shadowViewProjection;        //!< shadow map view projection matrix
    vec4 offsetAndSize;               //!< Texture offset for rasterizing the omnidirectional distance maps (x, y coordinates), size of the patch (z coordinate) and size of the texture (w coordinate)
    vec4 zFar;                        //!< Shadow distance map values are divided by z far camera plane value (x coordinate) to set values approximately in the [0,1] interval, although values can be bigger than 1
    vec4 lightForwardEmitterRadiance; //!< Light forward direction in xyz components, emitter radiance in w component
    uint irradianceFieldGridDensity;  //!< Density of the irradiance field's grid (value n means one irradiance field each n units in the voxelization volume in all dimensions)
    float irradianceMultiplier;       //!< Equivalent to IRRADIANCE_MULTIPLIER
    float directIrradianceMultiplier;  //!< Equivalent to DIRECT_IRRADIANCE_MULTIPLIER
} myMaterialData;

/////////////////////////////////////////////////////////////////////////////////////////////

layout(binding = 3)  uniform sampler2D reflectance;
layout(binding = 4)  uniform sampler2D normal;
layout(binding = 5)  uniform sampler3D irradiance3DStaticNegativeX;
layout(binding = 6)  uniform sampler3D irradiance3DStaticPositiveX;
layout(binding = 7)  uniform sampler3D irradiance3DStaticNegativeY;
layout(binding = 8)  uniform sampler3D irradiance3DStaticPositiveY;
layout(binding = 9)  uniform sampler3D irradiance3DStaticNegativeZ;
layout(binding = 10) uniform sampler3D irradiance3DStaticPositiveZ;
layout(binding = 11) uniform sampler3D irradiance3DDynamicNegativeX;
layout(binding = 12) uniform sampler3D irradiance3DDynamicPositiveX;
layout(binding = 13) uniform sampler3D irradiance3DDynamicNegativeY;
layout(binding = 14) uniform sampler3D irradiance3DDynamicPositiveY;
layout(binding = 15) uniform sampler3D irradiance3DDynamicNegativeZ;
layout(binding = 16) uniform sampler3D irradiance3DDynamicPositiveZ;

/////////////////////////////////////////////////////////////////////////////////////////////

/** Buffer to know, for a given 3D voxelization volume texture (whose indices are hashed to a unique uint 32-bit)
* if there are fragments at that position or that 3D voxelization volume is empty. */
layout (binding = 19) buffer coherent voxelOccupiedBuffer
{
   uint voxelOccupied[ ];
};

/////////////////////////////////////////////////////////////////////////////////////////////

/** For debug */
layout (binding = 23) buffer coherent debugBuffer
{
   float debugData[ ];
};

/////////////////////////////////////////////////////////////////////////////////////////////

/** Buffer of lit test voxels having information on a per-byte approach (each byte has information on whether any 
* voxel face is lit and what voxel faces are lit at bit level, with info [no info][-x][+x][-y][+y][-z][+z][AnyLitVoxelFace]) */
layout (binding = 24) buffer coherent litTestVoxelPerByteBuffer
{
    uint8_t litTestVoxelPerByte[ ];
};

/////////////////////////////////////////////////////////////////////////////////////////////

/** Buffer of dynamic lit test voxels having information on a per-byte approach (each byte has information on whether any 
* voxel face is lit and what voxel faces are lit at bit level, with info [no info][-x][+x][-y][+y][-z][+z][AnyLitVoxelFace]) */
layout (binding = 25) buffer coherent litTestDynamicVoxelPerByteBuffer
{
    uint8_t litTestDynamicVoxelPerByte[ ];
};

/////////////////////////////////////////////////////////////////////////////////////////////

/** Buffer to know, for a given 3D voxelization volume texture (whose indices are hashed to a unique uint 32-bit)
* if there are fragments at that position or that 3D voxelization volume is empty. */
layout (binding = 26) buffer coherent voxelOccupiedDynamicBuffer
{
    uint voxelOccupiedDynamic[ ];
};

/////////////////////////////////////////////////////////////////////////////////////////////

/** Pointer to the buffer used to tag those 2^3 tiles in the voxelization volume which have at least one 
* empty voxel that has at least one neighbour occupied static voxel that need padding to be computed */
layout (binding = 27) buffer coherent irradiancePaddingTagTilesBuffer
{
    uint8_t irradiancePaddingTagTiles[ ];
};

/////////////////////////////////////////////////////////////////////////////////////////////

/** Buffer used to tag those 2^3 tiles in the voxelization volume which have at least one 
* occupied voxel that will be processed for filtering together with neighbouring voxels */
layout (binding = 28) buffer coherent irradianceFilteringTagTilesBuffer
{
    uint8_t irradianceFilteringTagTiles[ ];
};

/////////////////////////////////////////////////////////////////////////////////////////////

layout(binding = 29) uniform accelerationStructureEXT raytracedaccelerationstructure;

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
vec3  g_fragmentNormal;          //!< Fragment sampled normal
float g_irradianceMultiplier;    //!< Value taken from raster flag IRRADIANCE_MULTIPLIER and divided by 100000

vec3 arrayDirection[6] =
{
    vec3(-1.0,  0.0,  0.0),
    vec3( 1.0,  0.0,  0.0),
    vec3( 0.0, -1.0,  0.0),
    vec3( 0.0,  1.0,  0.0),
    vec3( 0.0,  0.0, -1.0),
    vec3( 0.0,  0.0,  1.0),
};

bool record;
int counter;

/////////////////////////////////////////////////////////////////////////////////////////////

/** Returns from voxelOccupied if the current voxel is occupied by at least one fragment, or false otherwise
* @param indexHashed [in] hashed texture coordinates to test
* @return true if voxel occupied, false otherwise */
bool isVoxelOccupied(uint indexHashed)
{
    float integerPart;
    float indexDecimalFloat = float(indexHashed) / 32.0;
    float fractional        = modf(indexDecimalFloat, integerPart);
    uint index              = uint(integerPart);
    uint bit                = uint(fractional * 32.0);
    bool result             = ((voxelOccupied[index] & (1 << bit)) > 0);
    return result;
}

/////////////////////////////////////////////////////////////////////////////////////////////

/** Returns from voxelOccupiedDynamic if the current voxel is occupied by one voxel, or false otherwise
* @param indexHashed [in] hashed texture coordinates to test
* @return true if voxel occupied, false otherwise */
bool isVoxelOccupiedDynamic(uint indexHashed)
{
    float integerPart;
    float indexDecimalFloat = float(indexHashed) / 32.0;
    float fractional        = modf(indexDecimalFloat, integerPart);
    uint index              = uint(integerPart);
    uint bit                = uint(fractional * 32.0);
    bool result             = ((voxelOccupiedDynamic[index] & (1 << bit)) > 0);
    return result;
}

/////////////////////////////////////////////////////////////////////////////////////////////

/** hashed value for texture coordinates given as parameter in a voxelization of size given by g_voxelSizeInt
* @param texcoord  [in] texture coordinates
* @param voxelSize [in] voxelization size
* @return hashed value */
int getHashedIndexInteger(ivec3 texcoord)
{
    return texcoord.x * g_voxelSizeInt * g_voxelSizeInt + texcoord.y * g_voxelSizeInt + texcoord.z;
}

/////////////////////////////////////////////////////////////////////////////////////////////

/** Computes world space coordinates for 3D texture coordinates
* @param coordinates [in] texture coordinates
* @return world coordinates of the voxel coordinates given as parameter */
vec3 voxelSpaceToWorld(ivec3 coordinates)
{
    vec3 result = vec3(float(coordinates.x), float(coordinates.y), float(coordinates.z));
    result     /= g_voxelSizeThreeDimension;
    result     *= g_sceneExtent;
    result     += g_sceneMin;
    result     += (g_sceneExtent / g_voxelSize) * 0.5;
    return result;
}

/////////////////////////////////////////////////////////////////////////////////////////////

/** Computes world space coordinates for 3D texture coordinates
* @param coordinates [in] texture coordinates
* @return world coordinates of the voxel coordinates given as parameter */
vec3 voxelSpaceToWorld(uvec3 coordinates)
{
    vec3 result = vec3(float(coordinates.x), float(coordinates.y), float(coordinates.z));
    result     /= g_voxelSizeThreeDimension;
    result     *= g_sceneExtent;
    result     += g_sceneMin;
    result     += (g_sceneExtent / g_voxelSize) * 0.5;
    return result;
}

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

/** hashed value for texture coordinates given as parameter in a voxelization of size given by g_voxelSizeUint
* @param texcoord  [in] texture coordinates
* @param voxelSize [in] voxelization size
* @return hashed value */
uint getHashedIndex(uvec3 texcoord)
{
    return texcoord.x * g_voxelSizeUint * g_voxelSizeUint + texcoord.y * g_voxelSizeUint + texcoord.z;
}

/////////////////////////////////////////////////////////////////////////////////////////////

/** Computes the unhashed 3D coordinates for the position given with hashed value as parameter, for a uniform
* voxelization volume given by voxelizationWidth
* @param value [in] value to unhash
* @return unhashed value for parameter value */
ivec3 unhashValue(uint value)
{
  float number     = float(value) / g_voxelSize;

  ivec3 result;
  float integerPart;
  float fractional = modf(number, integerPart);
  result.z         = int(fractional * g_voxelSize);
  number          /= g_voxelSize;
  fractional       = modf(number, integerPart);
  result.y         = int(fractional * g_voxelSize);
  result.x         = int(integerPart);

  return result;
}

/////////////////////////////////////////////////////////////////////////////////////////////

/** Compute hashed value for texture coordinates given as parameter in a voxelization of size given as parameter
* @param texcoord  [in] texture coordinates
* @param voxelSize [in] voxelization size
* @return hashed value */
uint getHashedIndex(uvec3 texcoord, uint voxelSize)
{
    return texcoord.x * voxelSize * voxelSize + texcoord.y * voxelSize + texcoord.z;
}

/////////////////////////////////////////////////////////////////////////////////////////////

vec3 sampleBC5Normal(vec2 uv)
{
    vec2 normalSampled  = texture(normal, uv).xy;
    vec3 normalUnpacked = vec3((normalSampled.xy * 2.0) - 1.0, 0.0);
    normalUnpacked.z    = sqrt(1.0 - dot(normalUnpacked.xy, normalUnpacked.xy));
    return normalize(normalUnpacked);
}

/////////////////////////////////////////////////////////////////////////////////////////////

/** Sample normal direction texture and transform it into world space
* @return nothing */
void computeNormalDirection()
{
    vec3 norm3PS     = worldNormalVS;
    norm3PS          = normalize(norm3PS);
    vec3 tang3PS     = worldTangentVS;
    tang3PS          = normalize(tang3PS);
    vec3 binormal3   = cross(norm3PS,tang3PS);
    binormal3        = normalize(binormal3);
    mat3 rotMat      = mat3(tang3PS, binormal3, norm3PS); //Construct the texture-space matrix and rotate the normal into the world space
    vec3 vnormal     = sampleBC5Normal(uv);
    vec3 snormal     = rotMat * vnormal;
    g_fragmentNormal = normalize(snormal);
}

/////////////////////////////////////////////////////////////////////////////////////////////

float shadowCalculationRTQuery(vec3 fragmentPosition, vec3 emitterPosition)
{
    vec3 direction     = (emitterPosition - fragmentPosition);  // vector to light
    float tMin         = 0.01;
    float tMax         = length(direction);
    direction          = normalize(direction);
    vec3 startPosition = fragmentPosition + direction * 0.1;

    // The current approach for the emitter is a directional one, with the vector from the origin to his position being the light direction
    vec3 lightDirection = normalize(myMaterialData.lightPosition.xyz);
    if(dot(direction, lightDirection) <= 0.0)
    {
        return 0.0;
    }

    // Initializes a ray query object but does not start traversal
    rayQueryEXT rayQuery;
    rayQueryInitializeEXT(rayQuery, raytracedaccelerationstructure, gl_RayFlagsTerminateOnFirstHitEXT, 0xFF, startPosition, tMin, direction, tMax);

    // First ray query is done with the scene static goemetry
    while(rayQueryProceedEXT(rayQuery))
    {
    }

    // In case of intersection, something was visible and not the emitter
    if(rayQueryGetIntersectionTypeEXT(rayQuery, true) != gl_RayQueryCommittedIntersectionNoneEXT)
    {
        return 0.0;
    }

    return 1.0;
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

    if(record)
    {
        debugData[counter++] = 1100.0;
        debugData[counter++] = float(worldPosition.x);
        debugData[counter++] = float(worldPosition.y);
        debugData[counter++] = float(worldPosition.z);
        debugData[counter++] = 1200.0;
        debugData[counter++] = float(worldNormal.x);
        debugData[counter++] = float(worldNormal.y);
        debugData[counter++] = float(worldNormal.z);
        debugData[counter++] = 1400.0;
        float temp = dynamicSceneElement ? 1.0 : -1.0;
        debugData[counter++] = float(temp);
        debugData[counter++] = 1500.0;
        debugData[counter++] = float(xAxisDotResult);
        debugData[counter++] = float(yAxisDotResult);
        debugData[counter++] = float(zAxisDotResult);
        debugData[counter++] = 1600.0;
    }

    // Normalize to interval [0, 1], with 1.0 for the direction with angle 0 radians and 1.0 for those directions with angle PI / 2.0 radians
    xAxisDotResult = 1.0 - (acos(xAxisDotResult) / (PI * 0.5));
    yAxisDotResult = 1.0 - (acos(yAxisDotResult) / (PI * 0.5));
    zAxisDotResult = 1.0 - (acos(zAxisDotResult) / (PI * 0.5));

    vec3 weight = normalize(vec3(xAxisDotResult, yAxisDotResult, zAxisDotResult));

    if(record)
    {
        debugData[counter++] = 2100.0;
        debugData[counter++] = float(xAxisDotResult);
        debugData[counter++] = float(yAxisDotResult);
        debugData[counter++] = float(zAxisDotResult);
        debugData[counter++] = 2400.0;
        debugData[counter++] = float(weight.x);
        debugData[counter++] = float(weight.y);
        debugData[counter++] = float(weight.z);
        debugData[counter++] = 2400.0;
    }

    vec3 interpolatedIrradiance = weight.x * irradianceX + weight.y * irradianceY + weight.z * irradianceZ;

    return interpolatedIrradiance;
}

/////////////////////////////////////////////////////////////////////////////////////////////

vec3 interpolateIrradiance8VoxelTest(uvec3 voxelCoordinatesTex, vec3 fragmentPosition, vec3 normalDirection, bool dynamicSceneElement)
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

void main()
{
    vec4 reflectance = texture(reflectance, uv);
    
#ifdef MATERIAL_TYPE_ALPHATESTED
    if(reflectance.w < 0.3)
    {
        discard;
    }
#endif

    record = false;
    counter = 0;
    
    computeNormalDirection();

    float shadowResult        = shadowCalculationRTQuery(positionVS, myMaterialData.lightPosition.xyz); // TODO: Try to move this query to other places and analyse possible performance improvement
    g_sceneMin                = myMaterialData.sceneMin.xyz;
    g_sceneExtent             = myMaterialData.sceneExtent.xyz;
    g_voxelSizeThreeDimension = myMaterialData.voxelSize.xyz;
    g_voxelSize               = g_voxelSizeThreeDimension.x;
    g_voxelSizeUint           = uint(g_voxelSize);
    g_voxelSizeInt            = int(g_voxelSizeUint);
    uvec3 voxelCoordinatesTex = worldToVoxelSpace(positionVS);
    uint hashedPosition       = getHashedIndex(voxelCoordinatesTex);
    outColor                  = vec4(0.0, 0.0, 1.0, 1.0);
    g_irradianceMultiplier    = myMaterialData.irradianceMultiplier / 100000.0;
    float directMultiplier    = myMaterialData.directIrradianceMultiplier;
    
    // Some fragments can be generated at voxels positions which are not occupied for precision during the voxelization step.
    // Using conservative rasterization would generate too many voxels and obstruct some areas when voxelization resolution is not high, so
    // in case the fragment is in a voxel not occupied, find an occupied close one.

    float formFactorFragmentNormal = differentialAreaFormFactor(-1.0 * normalize(lightDirection).xyz, myMaterialData.lightPosition.xyz, g_fragmentNormal, positionVS);

    float formFactor = formFactorFragmentNormal; 
    vec3 irradiance  = interpolateDirectionIrradiance(positionVS, g_fragmentNormal, isDynamicSceneElement == 1.0 ? true : false);

    outColor.xyz = g_fragmentNormal;
}

/////////////////////////////////////////////////////////////////////////////////////////////
