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

layout (points) in;

/////////////////////////////////////////////////////////////////////////////////////////////

// layout (triangle_strip, max_vertices = 36) out;
layout (line_strip, max_vertices = 24) out;

/////////////////////////////////////////////////////////////////////////////////////////////

layout (location = 0) in vec3 positionTextureCoordinatesVS[];
layout (location = 1) in uint hashedPositionVS[];

/////////////////////////////////////////////////////////////////////////////////////////////

layout (location = 0) out vec4 colorGS;

/////////////////////////////////////////////////////////////////////////////////////////////

layout (std140, binding = 1) uniform materialData
{
    mat4 viewProjection;   //!< Matrix to transform each cube's center coordinates in the geometry shader
	vec4 voxelizationSize; //!< Voxelizatoin size
	vec4 sceneMin;         //!< Scene minimum value
	vec4 sceneExtent;      //!< Extent of the scene. Also, w value is used to distinguish between rasterization of reflectance or normal information
	uint numOcupiedVoxel;  //!< Number of occupied voxel
	uint padding0;         //!< Padding value to achieve 4 32-bit data in the material in the shader
	uint padding1;         //!< Padding value to achieve 4 32-bit data in the material in the shader
	uint padding2;         //!< Padding value to achieve 4 32-bit data in the material in the shader
} myMaterialData;

layout(binding = 2, r32ui) uniform coherent volatile uimage3D voxelizationReflectance;
layout(binding = 3, r32ui) uniform coherent volatile uimage3D dynamicVoxelizationReflectanceTexture;

/////////////////////////////////////////////////////////////////////////////////////////////

/** For debug */
layout (binding = 4) buffer coherent voxelrasterinscenariodebugbuffer
{
   float voxelrasterinscenariodebug[ ];
};

/////////////////////////////////////////////////////////////////////////////////////////////

/** Buffer for accelerating the search of elements in voxelFirstIndexCompactedBuffer and
* voxelHashedPositionCompactedBuffer. Contains the index to the first element in
* voxelFirstIndexCompactedBuffer and voxelHashedPositionCompactedBuffer for fixed
* 3D voxelization values, like (0,0,0), (0,0,128), (0,1,0), (0,1,128), (0,2,0), (0,2,128), ... */
layout (binding = 5) buffer coherent IndirectionIndexBuffer
{
   uint IndirectionIndex[ ];
};

/////////////////////////////////////////////////////////////////////////////////////////////

/** Buffer for accelerating the search of elements in voxelFirstIndexCompactedBuffer and
* voxelHashedPositionCompactedBuffer. Contains the amount of elements at the same index in
* IndirectionIndexBuffer, to know how many elements to the right are there when using the
* value at the same index of IndirectionIndexBuffe in voxelFirstIndexCompactedBuffer */
layout (binding = 6) buffer coherent IndirectionRankBuffer
{
   uint IndirectionRank[ ];
};

/////////////////////////////////////////////////////////////////////////////////////////////

/** Buffer with the hashed position of the 3D volume voxelization coordinates the fragment
* data in the same index at the voxelFirstIndexCompactedBuffer buffer
* occupied initially in the non-compacted buffer */
layout (binding = 7) buffer coherent voxelHashedPositionCompactedBuffer
{
   uint voxelHashedPositionCompacted[ ];
};

/////////////////////////////////////////////////////////////////////////////////////////////

/** Buffer of lit test voxels having information on a per-byte approach (each byte has information on whether any 
* voxel face is lit and what voxel faces are lit at bit level, with info [no info][-x][+x][-y][+y][-z][+z][AnyLitVoxelFace]) */
layout (binding = 11) buffer coherent litTestVoxelPerByteBuffer
{
	uint8_t litTestVoxelPerByte[ ];
};

/////////////////////////////////////////////////////////////////////////////////////////////

/** Buffer of dynamic lit test voxels having information on a per-byte approach (each byte has information on whether any 
* voxel face is lit and what voxel faces are lit at bit level, with info [no info][-x][+x][-y][+y][-z][+z][AnyLitVoxelFace]) */
layout (binding = 12) buffer coherent litTestDynamicVoxelPerByteBuffer
{
	uint8_t litTestDynamicVoxelPerByte[ ];
};

/////////////////////////////////////////////////////////////////////////////////////////////

/** Push constant used to know whether the voxel information needs to be taken from the
* voxelHashedPositionCompactedBuffer buffer or the voxelOccupiedDynamicBuffer buffer */
layout(push_constant) uniform pushConstant
{
	int pushConstantVoxelFromStaticCompacted;
} myPushConstant;

/////////////////////////////////////////////////////////////////////////////////////////////

// GLOBAL VARIABLES
const uint maxValue          = 4294967295;
const uint maxIterationIndex = 20;

/////////////////////////////////////////////////////////////////////////////////////////////

/** Decode 8-bit per channel RGBA value in val
* @param val [in] value to decode
* @return decoded 8-bit per channel RGBA color value */
uvec4 convRGBA8ToVec4( in uint val )
{
    return uvec4( uint( (val&0x000000FF) ), uint( (val&0x0000FF00)>>8U), uint( (val&0x00FF0000)>>16U), uint( (val&0xFF000000)>>24U) );
}

/////////////////////////////////////////////////////////////////////////////////////////////

/** Computes world space coordinates for 3D texture coordinates
* @param voxelCoordinates [in] texture coordinates
* @return world coordinates of the voxel coordinates given as parameter */
vec3 voxelSpaceToWorldFloat(vec3 voxelCoordinates)
{
    vec3 result = voxelCoordinates;
    result     /= myMaterialData.voxelizationSize.xyz;
    result     *= myMaterialData.sceneExtent.xyz;
    result     += myMaterialData.sceneMin.xyz;
    result     += (myMaterialData.sceneExtent.xyz / myMaterialData.voxelizationSize.x) * 0.5;

    return result;
}

/////////////////////////////////////////////////////////////////////////////////////////////

/** Search in voxelHashedPositionCompactedBuffer for the index of the hashed value
* of the texcoord parameter.
* @param texcoord       [in] 3D texture coordinates to find index for
* @param hashedPosition [in] texcoord parameter hashed texture coordinates
* @return uvec2 with index in .x field and test result in .y field */
uvec2 findHashedCompactedPisitionIndex(uvec3 texcoord, uint hashedPosition)
{
    uvec2 result          = uvec2(0, 0); // y field is control value, 0 means element not found, 1 means element found
    uint indirectionIndex = uint(myMaterialData.voxelizationSize.x) * texcoord.x + texcoord.y;
    uint index            = IndirectionIndex[indirectionIndex];
    uint rank             = IndirectionRank[indirectionIndex];

    for(uint i = index; i < (index + rank); ++i)
    {
        if(voxelHashedPositionCompacted[i] == hashedPosition)
        {
            return uvec2(i, 1);
        }
    }

    return result;
}

/////////////////////////////////////////////////////////////////////////////////////////////

/** Computes world space coordinates for 3D texture coordinates
* @param voxelCoordinates [in] texture coordinates
* @return world coordinates of the voxel coordinates given as parameter */
vec3 voxelSpaceToWorld(uvec3 coordinates)
{
    vec3 result = vec3(float(coordinates.x), float(coordinates.y), float(coordinates.z));
    result     /= vec3(float(myMaterialData.voxelizationSize.x));
    result     *= myMaterialData.sceneExtent.xyz;
    result     += myMaterialData.sceneMin.xyz;
    result     += (vec3(myMaterialData.sceneExtent.xyz) / myMaterialData.voxelizationSize.x) * 0.5;
    return result;
}

/////////////////////////////////////////////////////////////////////////////////////////////

/** Computes the unhashed 3D coordinates for the position given with hashed value as parameter, for a uniform
* voxelization volume given by voxelizationWidth
* @param value [in] value to unhash
* @return unhashed value for parameter value */
uvec3 unhashValue(uint value)
{
  float sizeFloat = myMaterialData.voxelizationSize.x;
  float number    = float(value) / sizeFloat;

  uvec3 result;
  float integerPart;
  float fractional = modf(number, integerPart);
  result.z         = uint(fractional * sizeFloat);
  number          /= sizeFloat;
  fractional       = modf(number, integerPart);
  result.y         = uint(fractional * sizeFloat);
  result.x         = uint(integerPart);

  return result;
}

/////////////////////////////////////////////////////////////////////////////////////////////

/** Modulus operation with format variables
* voxelization volume given by voxelizationWidth
* @param x [in] one of the two parameters for the x modulus y computation
* @param y [in] one of the two parameters for the x modulus y computation
* @return value of x modulus y */
float modulus(float x, float y)
{
  return x - y * floor(x / y);
}

/////////////////////////////////////////////////////////////////////////////////////////////

/** Map curval parameter in the interval minval to maxval
* @param minval [in] minimum value of the interval
* @param maxval [in] maximum value of the interval
* @param curval [in] value to map
* @return mapped value */
float remap(float minval, float maxval, float curval)
{
    return ( curval - minval ) / ( maxval - minval );
} 

/////////////////////////////////////////////////////////////////////////////////////////////

vec4 convRGBA8ToVec4New(uint val)
{
	return vec4(float((val&0x000000FF)), float((val&0x0000FF00)>>8U), float ((val&0x00FF0000)>>16U), float((val&0xFF000000)>>24U));
}

////////////////////////////////////////////////////////////////////////////////////////////////////////

/** Test whether the voxel at index indexHashed has any face set as lit
* @param indexHashed [in] voxel index to test if it has any voxel face as lit
* @return true if any voxel face as been set as lit, false otherwise */
bool isVoxelLit(uint indexHashed)
{
	return (uint(litTestVoxelPerByte[indexHashed]) > 0);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////

/** Test whether the dynamic voxel at index indexHashed has any face set as lit
* @param indexHashed [in] dynamic voxel index to test if it has any voxel face as lit
* @return true if any voxel face has been set as lit, false otherwise */
bool isDynamicVoxelLit(uint indexHashed)
{
	return (uint(litTestDynamicVoxelPerByte[indexHashed]) > 0);
}

/////////////////////////////////////////////////////////////////////////////////////////////

//       V6------------------------------V7
//        /                             /
//       /                             / |
//      /                             /  |
//     /                             /   |
//    /                             /    |
//  V4------------------------------V5   |
//    |                            |     |
//    |                            |     |             +y
//    |                            |     |             ^
//    |                            |     |             |
//    |                            |     |             |
//    |    V2                      |     /V3           |
//    |                            |    /              |
//    |                            |   /               |
//    |                            |  /                |
//    |                            | /                 |
//    ------------------------------/                  /------------> +x
//   V0                            V1                 /
//                                                   /
//                                                  /
//                                                 +z

/////////////////////////////////////////////////////////////////////////////////////////////

/** Emits the geometry to draw each voxel's edge considering vertices as described above
* @param voxelCenterWorldCoordinates [in] voxel center in world coordinates
* @param colorOutput [in] voxel color
* @return nothing */
void emitVoxelEdgeGeometry(vec3 voxelCenterWorldCoordinates, vec4 colorOutput)
{
	vec3 voxelizationSize = myMaterialData.voxelizationSize.xyz;
	vec3 sceneMin         = myMaterialData.sceneMin.xyz;
	vec3 sceneExtent      = myMaterialData.sceneExtent.xyz;

    vec3 voxelSide        = (sceneExtent / voxelizationSize.x) * 0.5;

    float minX            = voxelCenterWorldCoordinates.x - voxelSide.x;
    float maxX            = voxelCenterWorldCoordinates.x + voxelSide.x;

    float minY            = voxelCenterWorldCoordinates.y - voxelSide.y;
    float maxY            = voxelCenterWorldCoordinates.y + voxelSide.y;

    float minZ            = voxelCenterWorldCoordinates.z - voxelSide.z;
    float maxZ            = voxelCenterWorldCoordinates.z + voxelSide.z;

    vec3 V0               = vec3(minX, minY, maxZ);
    vec3 V1               = vec3(maxX, minY, maxZ);
    vec3 V2               = vec3(minX, minY, minZ);
    vec3 V3               = vec3(maxX, minY, minZ);

    vec3 V4               = vec3(minX, maxY, maxZ);
    vec3 V5               = vec3(maxX, maxY, maxZ);
    vec3 V6               = vec3(minX, maxY, minZ);
    vec3 V7               = vec3(maxX, maxY, minZ);

    mat4 viewProjection   = myMaterialData.viewProjection;

    gl_Position = viewProjection * vec4((voxelCenterWorldCoordinates + V0).xyz, 1.0);
	colorGS     = colorOutput;
	EmitVertex();
	gl_Position = viewProjection * vec4((voxelCenterWorldCoordinates + V1).xyz, 1.0);
	colorGS     = colorOutput;
	EmitVertex();
	EndPrimitive();

	gl_Position = viewProjection * vec4((voxelCenterWorldCoordinates + V0).xyz, 1.0);
	colorGS     = colorOutput;
	EmitVertex();
	gl_Position = viewProjection * vec4((voxelCenterWorldCoordinates + V2).xyz, 1.0);
	colorGS     = colorOutput;
	EmitVertex();
	EndPrimitive();

	gl_Position = viewProjection * vec4((voxelCenterWorldCoordinates + V1).xyz, 1.0);
	colorGS     = colorOutput;
	EmitVertex();
	gl_Position = viewProjection * vec4((voxelCenterWorldCoordinates + V3).xyz, 1.0);
	colorGS     = colorOutput;
	EmitVertex();
	EndPrimitive();

	gl_Position = viewProjection * vec4((voxelCenterWorldCoordinates + V3).xyz, 1.0);
	colorGS     = colorOutput;
	EmitVertex();
	gl_Position = viewProjection * vec4((voxelCenterWorldCoordinates + V2).xyz, 1.0);
	colorGS     = colorOutput;
	EmitVertex();
	EndPrimitive();

	gl_Position = viewProjection * vec4((voxelCenterWorldCoordinates + V0).xyz, 1.0);
	colorGS     = colorOutput;
	EmitVertex();
	gl_Position = viewProjection * vec4((voxelCenterWorldCoordinates + V4).xyz, 1.0);
	colorGS     = colorOutput;
	EmitVertex();
	EndPrimitive();

	gl_Position = viewProjection * vec4((voxelCenterWorldCoordinates + V1).xyz, 1.0);
	colorGS     = colorOutput;
	EmitVertex();
	gl_Position = viewProjection * vec4((voxelCenterWorldCoordinates + V5).xyz, 1.0);
	colorGS     = colorOutput;
	EmitVertex();
	EndPrimitive();

	gl_Position = viewProjection * vec4((voxelCenterWorldCoordinates + V3).xyz, 1.0);
	colorGS     = colorOutput;
	EmitVertex();
	gl_Position = viewProjection * vec4((voxelCenterWorldCoordinates + V7).xyz, 1.0);
	colorGS     = colorOutput;
	EmitVertex();
	EndPrimitive();

	gl_Position = viewProjection * vec4((voxelCenterWorldCoordinates + V2).xyz, 1.0);
	colorGS     = colorOutput;
	EmitVertex();
	gl_Position = viewProjection * vec4((voxelCenterWorldCoordinates + V6).xyz, 1.0);
	colorGS     = colorOutput;
	EmitVertex();
	EndPrimitive();

	gl_Position = viewProjection * vec4((voxelCenterWorldCoordinates + V4).xyz, 1.0);
	colorGS     = colorOutput;
	EmitVertex();
	gl_Position = viewProjection * vec4((voxelCenterWorldCoordinates + V5).xyz, 1.0);
	colorGS     = colorOutput;
	EmitVertex();
	EndPrimitive();

	gl_Position = viewProjection * vec4((voxelCenterWorldCoordinates + V5).xyz, 1.0);
	colorGS     = colorOutput;
	EmitVertex();
	gl_Position = viewProjection * vec4((voxelCenterWorldCoordinates + V7).xyz, 1.0);
	colorGS     = colorOutput;
	EmitVertex();
	EndPrimitive();

	gl_Position = viewProjection * vec4((voxelCenterWorldCoordinates + V7).xyz, 1.0);
	colorGS     = colorOutput;
	EmitVertex();
	gl_Position = viewProjection * vec4((voxelCenterWorldCoordinates + V6).xyz, 1.0);
	colorGS     = colorOutput;
	EmitVertex();
	EndPrimitive();

	gl_Position = viewProjection * vec4((voxelCenterWorldCoordinates + V6).xyz, 1.0);
	colorGS     = colorOutput;
	EmitVertex();
	gl_Position = viewProjection * vec4((voxelCenterWorldCoordinates + V4).xyz, 1.0);
	colorGS     = colorOutput;
	EmitVertex();
	EndPrimitive();
}

/////////////////////////////////////////////////////////////////////////////////////////////

void main()
{
	// Push constant to know whether this voxel has been generated from the static or the dynamic part of the scene
	int voxelFromStaticCompacted = myPushConstant.pushConstantVoxelFromStaticCompacted;

	uvec2 index = findHashedCompactedPisitionIndex(uvec3(uint(positionTextureCoordinatesVS[0].x),
			                                             uint(positionTextureCoordinatesVS[0].y),
			                                             uint(positionTextureCoordinatesVS[0].z)),
												   hashedPositionVS[0]);

	bool showReflectance     = (myMaterialData.sceneExtent.w == 1.0);
	bool showNormal          = (myMaterialData.sceneExtent.w == 0.0);
	bool showLitVoxel        = (myMaterialData.sceneExtent.w == 0.5);
	bool showNormalDensity   = (myMaterialData.sceneExtent.w == 7.0);
	vec4 boxColor            = vec4(0.0, 0.0, 0.0, 1.0);

	vec4 negativeXColor;
	vec4 positiveXColor;
	vec4 negativeYColor;
	vec4 positiveYColor;
	vec4 negativeZColor;
	vec4 positiveZColor;

	uvec3 coordinates = uvec3(uint(positionTextureCoordinatesVS[0].x), uint(positionTextureCoordinatesVS[0].y), uint(positionTextureCoordinatesVS[0].z));

	if(showReflectance)
	{
    	if(voxelFromStaticCompacted == 1)
		{
			negativeXColor = convRGBA8ToVec4New(imageLoad(voxelizationReflectance, ivec3(coordinates)).x);
		}
    	else if(voxelFromStaticCompacted == 0)
		{
			negativeXColor = convRGBA8ToVec4New(imageLoad(dynamicVoxelizationReflectanceTexture, ivec3(coordinates)).x);
		}

		boxColor.xyz = negativeXColor.xyz / 255.0;
	}
	else if(showLitVoxel)
	{
		if(voxelFromStaticCompacted == 1)
		{
			if(isVoxelLit(hashedPositionVS[0]))
			{
				boxColor.xyz = vec3(1.0, 1.0, 1.0);	
			}
		}
    	else if(voxelFromStaticCompacted == 0)
		{
			if(isDynamicVoxelLit(hashedPositionVS[0]))
			{
				boxColor.xyz = vec3(1.0, 1.0, 1.0);
			}
		}
	}

	vec3 worldCoordinates = voxelSpaceToWorldFloat(positionTextureCoordinatesVS[0]);
	worldCoordinates      *= 0.5;
	vec4 colorOutput      = (voxelFromStaticCompacted == 1) ? vec4(1.0, 0.0, 0.0, 1.0) : vec4(0.0, 1.0, 0.0, 1.0);
	
	emitVoxelEdgeGeometry(worldCoordinates, colorOutput);
}

/////////////////////////////////////////////////////////////////////////////////////////////
