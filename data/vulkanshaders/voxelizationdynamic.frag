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

#extension GL_ARB_gpu_shader_int64 : enable

/////////////////////////////////////////////////////////////////////////////////////////////

layout (binding = 2) uniform materialData
{
	mat4 projection;       //!< Orthographic projection matrix used for voxelization
	mat4 viewX;            //!< x axis view matrix used for voxelization
	mat4 viewY;            //!< y axis view matrix used for voxelization
	mat4 viewZ;            //!< z axis view matrix used for voxelization
	vec4 voxelizationSize; //!< 3D voxel texture size
	vec4 sceneMin;         //!< Scene AABB min coordinates
	vec4 sceneExtent;      //!< Scene extent
} myMaterialData;

/////////////////////////////////////////////////////////////////////////////////////////////

layout(binding = 3, r32ui) uniform coherent volatile uimage3D dynamicVoxelizationReflectanceTexture;

/////////////////////////////////////////////////////////////////////////////////////////////

/** Buffer to know, for a given 3D voxelization volume texture (whose indices are hashed to a unique uint 32-bit)
* if there are fragments at that position or that 3D voxelization volume is empty. */
layout (binding = 10) buffer coherent voxelOccupiedDynamicBuffer
{
	uint voxelOccupiedDynamic[ ];
};

/////////////////////////////////////////////////////////////////////////////////////////////

/** Buffer for debug purposes */
layout (binding = 11) buffer coherent dynamicVoxelizationDebugBuffer
{
	float dynamicVoxelizationDebug[ ];
};

/////////////////////////////////////////////////////////////////////////////////////////////

// Samplers
layout(binding = 12) uniform sampler2D reflectanceMap;

/////////////////////////////////////////////////////////////////////////////////////////////

layout (location = 1) in vec2 gsUV;
layout (location = 2) flat in uint faxis;

/////////////////////////////////////////////////////////////////////////////////////////////

layout (location = 0) out vec4 outColor;

/////////////////////////////////////////////////////////////////////////////////////////////

/** hashed value for texture coordinates given as parameter in a voxelization of size given as parameter
* @param texcoord  [in] texture coordinates
* @param voxelSize [in] voxelization size
* @return hashed value */
uint getHashedIndex(uvec3 texcoord, uint voxelSize)
{
	return texcoord.x * voxelSize * voxelSize + texcoord.y * voxelSize + texcoord.z;
}

/////////////////////////////////////////////////////////////////////////////////////////////

/** Decode 8-bit per channel RGBA value in val
* @param val [in] value to decode
* @return decoded 8-bit per channel RGBA color value */
vec4 convRGBA8ToVec4(uint val)
{
	return vec4(float((val&0x000000FF)), float((val&0x0000FF00)>>8U), float ((val&0x00FF0000)>>16U), float((val&0xFF000000)>>24U));
}

/////////////////////////////////////////////////////////////////////////////////////////////

/** Encode 8-bit per channel RGBA value given as parameter in a single unsigned it
* @param value [in] value to encode
* @return unsigned int with encoded values */
uint convVec4ToRGBA8(vec4 val)
{
	return (uint(val.w)&0x000000FF)<<24U | (uint(val.z)&0x000000FF) <<16U | (uint(val.y)&0x000000FF)<<8U | (uint(val.x)&0x000000FF);
}

/////////////////////////////////////////////////////////////////////////////////////////////

/** Use a per-bit approach to flag occupied voxels for the given index, which is a hashing of the 3D voxel position.
* Several threads cannot be writing on the same 32 bit uint, since this function is called inside a thread spinlock
* @param indexHashed [in] index of the voxel to set as occupied (hashed voxel's 3D texture coordinates)
* @return nothing */
void setVoxelOccupiedDynamic(uint indexHashed)
{
	float integerPart;
	float indexDecimalFloat = float(indexHashed) / 32.0;
	float fractional        = modf(indexDecimalFloat, integerPart);
	uint index              = uint(integerPart);
	uint bit                = uint(fractional * 32.0);
	uint value              = (1 << bit);
	uint originalValue      = atomicOr(voxelOccupiedDynamic[index], value);
}

/////////////////////////////////////////////////////////////////////////////////////////////

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
uint getArrayDirectionIndex(vec3 direction)
{
    uint indexClosest   = 0;
    float minimumAngle = -1.0;
    float minimumAngleTemp;

    for(uint i = 0; i < 6; ++i)
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

/** Encodes in the encode parameter the offset given as parameter (value in rank [0, 63]) for the voxel face index
* given by the voxelFace parameter in rank [0, 5]
* @param encode    [in] 32-bit uint where to encode the value
* @param voxelFace [in] voxel face index where to encode the value (value in rank [0, 5])
* @param offset    [in] offset value to encode (value in rank [0, 63])
* @return encoded value */
uint encodeOffsetInVoxelFace(uint encode, uint voxelFace, uint offset)
{
	// Set the set of five bits to 0 before writing new value
	uint temp = encode & ~(0x1F << (voxelFace * 5));
	return temp | (offset << (voxelFace * 5));
}

/////////////////////////////////////////////////////////////////////////////////////////////

/** Decode the voxel offset for the voxel face given by the voxelFace parameter, encoded in the decode 
* variable given as parameter
* @param decode    [in] 32-bit uint where to decode the value from (value in rank [0, 31])
* @param voxelFace [in] voxel face index where to decode the value (value in rank [0, 5])
* @return decoded offset for the voxel face (value in rank [0, 31]) */
uint decodeOffsetInVoxelFace(uint decode, uint voxelFace)
{
	//uint temp = decode << ((5 - voxelFace) * 5 + 2);
	uint temp = decode << ((5 - voxelFace) * 5 + 2);
	return temp >> 27;
}

/////////////////////////////////////////////////////////////////////////////////////////////

void main()
{
	uvec3 texcoord;
	int voxelSize = int(myMaterialData.voxelizationSize.x);
	uvec4 temp    = uvec4(gl_FragCoord.x, voxelSize - gl_FragCoord.y,      min(float(voxelSize) * gl_FragCoord.z, voxelSize - 1), 0);

	if( faxis == 0 )
	{
		texcoord.x = temp.z;
		texcoord.y = temp.y;
		texcoord.z = temp.x;
	}
	else if( faxis == 1 )
	{
		texcoord.x = uint(voxelSize) - 1 - temp.y;
		texcoord.y = temp.z;        
		texcoord.z = temp.x;
	}
	else
	{
		texcoord.x = uint(voxelSize) - 1 - temp.x;
		texcoord.y = temp.y;
		texcoord.z = temp.z;
	}

	uint prevStoredVal = 0;
	uint indexHashed   = getHashedIndex(uvec3(texcoord), uint(voxelSize));
	vec3 reflectance   = texture(reflectanceMap, gsUV.st).xyz;
	vec4 val           = vec4(0.0);
	val.rgb            = reflectance * 255.0; // Optimise following calculations
	val.w              = 1.0;
	uint newVal        = convVec4ToRGBA8(val);
	uint curStoredVal;

	while((curStoredVal = imageAtomicCompSwap(dynamicVoxelizationReflectanceTexture, ivec3(texcoord), prevStoredVal, newVal)) != prevStoredVal)
	{
		prevStoredVal = curStoredVal;
		vec4 rval     = convRGBA8ToVec4(curStoredVal);
		rval.xyz      = (rval.xyz * rval.w);
		vec4 curValF  = rval + val;
		curValF.xyz  /= curValF.w;
		newVal        = convVec4ToRGBA8(curValF);
	}

	setVoxelOccupiedDynamic(indexHashed);

	// Use atomic operations to build the AABB for all the fragments generated inside each of the voxels, this will later be used in the dynamic 
	// light bounce voxel irradiance for the per-voxel face ray origin where to dispatch rays from

	//bool record = (texcoord == uvec3(6, 4, 22));
	bool record = false;
	uint counter = 0;

	outColor = vec4(1.0, 0.0, 0.0, 1.0);
}

/////////////////////////////////////////////////////////////////////////////////////////////
