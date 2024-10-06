/*
Copyright 2014 Alejandro Cosin

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

#ifndef _BUFFERVERIFICATIONHELPER_H_
#define _BUFFERVERIFICATIONHELPER_H_

// GLOBAL INCLUDES

// PROJECT INCLUDES
#include "../headers.h"
#include "../../include/util/getsetmacros.h"

// CLASS FORWARDING

// NAMESPACE
using namespace commonnamespace;

// DEFINES

/////////////////////////////////////////////////////////////////////////////////////////////

/** This is a helper class to verify the information contained in the buffers generated
* during the voxelization and prefix sum post porcess of the scene */
class BufferVerificationHelper
{
public:

	/** Method to know if the voxel with hashed position given by indexHashed is empty or occupied
	* @param indexHashed                [in] hashed index of the 3D position to test
	* @param pVectorVoxelOccupiedBuffer [in] pointer to the buffer containing the occupied information at a bit level
	* @return rtrue if occupied, false otherwise */
	static bool isVoxelOccupied(uint indexHashed, uint* pVectorVoxelOccupiedBuffer);

	/** Output voxelOccupiedBuffer buffer
	* @return nothing */
	static void outputVoxelOccupiedBuffer();

	/** Verify voxelization generated fragment data and linked lists
	* @return nothing */
	static void verifyFragmentData();

	/** Method to know if the voxel at bit given as parameter in the uint representing a set
	* of 32 occupied voxel information is occupied or empty
	* @param value [in] value to test if it's occupied
	* @param bit   [in] bit to test if occupied
	* @return true if occupied, false otherwise
	* @return nothing */
	static bool getVoxelOccupied(uint value, uint bit);

	/** Method to debug the contents of m_voxelOccupiedBuffer buffer
	* @return nothing */
	static void verifyVoxelOccupiedBuffer();

	/** Will copy to CPU the buffer generated to store all the temporary accumulated values in m_prefixSumPlanarBuffer
	* and verify the sum is correct for each level */
	static void verifyPrefixSumData();

	/** Computes the hashed coordinates for the position given as parameter for the voxelization size given as parameter
	* @param texcoord  [in] texture coordinates
	* @param voxelSize [in] voxelization volume size
	* @return hashed value */
	static uint getHashedIndex(uvec3 texcoord, uint voxelSize);

	/** Computes the unhashed 3D coordinates for the position given with hashed value as parameter, for a uniform
	* voxelization volume given by voxelizationWidth
	* @param value             [in] value to unhash
	* @param voxelizationWidth [in] width of the voxelization volume the value parameter has been hashed
	* @return unhashed value for parameter value */
	static uvec3 unhashValue(uint value, uint voxelizationWidth);

	/** Count the number elements in voxelFirstIndexBuffer with value != max uint value, and compare the result with
	* the results of the prefix sum algorithm for all levels in the reduction phase of the algorithm
	* @return nothing */
	static void verifyVoxelFirstIndexBuffer();

	/** Verify the contents in voxelFirstIndexCompactedBuffer match the ones in voxelFirstIndexBuffer:
	* for each element in voxelFirstIndexBuffer with value != max uint value, the corresponding acculumated index
	* in voxelFirstIndexCompactedBuffer has to match the information
	* @return nothing */
	static void verifyVoxelFirstIndexAndVoxelFirstIndexCompacted();

	/** Write to file the information present in voxelHashedPositionCompactedBuffer unhashing the indices found
	* in this buffer
	* @return nothing */
	static void outputHashedPositionCompactedBufferInfo();

	/** Output the voxelHashedPositionCompactedBuffer buffer
	* @return nothing */
	static void outputIndirectionBuffer();

	/** Output the data from IndirectionIndexBuffer and IndirectionRankBuffer
	* @return nothing */
	static void outputIndirectionBufferData();

	/** Verify information at IndirectionIndexBuffer and IndirectionRankBuffer buffers
	* @return nothing */
	static void verifyIndirectionBuffers();

	/** Copy to CPU the buffer with the initial first index information given by m_voxelFirstIndexBuffer and
	* the compacted information from m_voxelFirstIndexCompactedBuffer, and verify the information has been properly compacted
	* by the parallel prefix sum algorithm
	* @return nothing */
	static void verifyVoxelizationProcessData();

	/** Debug method to verify the result of the summed area texture
	* @param initialTexture [in] texture wityh the initial data used by the GPU to generate the texture given by finalTexture
	* @param finalTexture   [in] summed-area texture result
	* @return nothing */
	static void verifySummedAreaResult(Texture* initialTexture, Texture* finalTexture);

	/** Compute the summed-area of the surface represented by the vecData parameter
	* @param vecData [in] vector representing the summed-area data
	* @param maxStep [in] max number of steps to compute the summed-area result, -1 means all steps
	* @return summed-area result */
	static vectorVectorFloat computeSummedArea(vectorVectorFloat& vecData, int maxStep);

	/** Verify the values in vecData0 and vecData1 are the same with a numerical precission error less than eps
	* @param vecData0 [in] one of the two vectors of data to compare
	* @param vecData1 [in] one of the two vectors of data to compare
	* @return true if values are equal (taking into account the eps precission value as maximum error threshold, false otherwise */
	static bool verifyAreEqual(const vectorVectorFloat& vecData0, const vectorVectorFloat& vecData1, float eps);

	/** Will verify the add-up of all values present in the texture given as parameter equal to the value parameter
	* @param texture   [in] texture to verify
	* @param value     [in] value to compare with the add-up of all values in the texture parameter
	* @param threshold [in] error threshold
	* @return true if add-up value and float are equal with a maximum error given by threshold, false otherwise */
	static bool verifyAddUpTextureValue(Texture* texture, float value, float threshold);

	/** Returns the world coordinates of the center of the voxel of texture coordinates given by coordinates in a
	* voxelization of size voxelSize
	* @param coordinates [in] integer voxel texture coordinates
	* @param voxelSize   [in] size of the voxelization texture (in float)
	* @param sceneExtent [in] scene extent
	* @param sceneMin    [in] scene aabb min value
	* @return voxel world coordinates from coordinates parameter */
	static vec3 voxelSpaceToWorld(uvec3 coordinates, vec3 voxelSize, vec3 sceneExtent, vec3 sceneMin);

	/** Returns the voxel coordinates of the world coordinates giv3en as parameter
	* @param coordinates [in] world space coordinates
	* @param voxelSize   [in] size of the voxelization texture (in float)
	* @param sceneExtent [in] scene extent
	* @param sceneMin    [in] scene aabb min value
	* @return texture space coordinates of the world coordinates given as parameter */
	static uvec3 worldToVoxelSpace(vec3 coordinates, vec3 voxelSize, vec3 sceneExtent, vec3 sceneMin);

	/** Utility method to know if the intervals (minX0, maxX0) and (minX1, maxX1) do intersect
	* @param minX0 [in] minimum value for the first interval
	* @param maxX0 [in] maximum value for the first interval
	* @param minX1 [in] minimum value for the second interval
	* @param maxX1 [in] maximum value for the second interval
	* @return true if there's an intersection, false otherwise */
	static bool BufferVerificationHelper::intervalIntersection(int minX0, int maxX0, int minX1, int maxX1);

	/** Copy to CPU the buffer with the amount of visible voxels from each voxel face, voxelVisibilityDebugBuffer
	* to verify the indices used for mapping are properly computed
	* @return nothing */
	static void verifyVoxelVisibilityIndices();

	/** Verify the voxelVisibilityFirstIndexBuffer which has the compacted data of the starting index of all
	* visible voxels from each voxel face.
	* @return nothing */
	static void verifyVoxelVisibilityFirstIndexBuffer();

	/** Verify the camera visible voxels put in cameraVisibleVoxelCompacted were processed in the light bounce step and
	* the corresponding flag was set in lightBounceProcessedVoxel
	* visible voxels from each voxel face.
	* @return nothing */
	static void verifyCameraVisibleAndLightBounceVoxels();

	/** Prints the visibility info (visible voxel coordinates for static geometry, directions for dynamic geometry) for
	* the voxel coordinates given as parameter.
	* @param voxelCoordinates [in] coordinates to look information about
	* @param voxelizationSize [in] voxelization size
	* @return nothing */
	static bool printVisibilityInfoForVoxelCoordinates(uvec3 voxelCoordinates, uint voxelizationSize);

	/** Prints the visibility info (visible voxel coordinates for static geometry, directions for dynamic geometry) for
	* the voxel coordinates given as parameter using directly the voxelVisibility4BytesBuffer buffer
	* @param voxelCoordinates [in] coordinates to look information about
	* @param voxelizationSize [in] voxelization size
	* @return nothing */
	static bool printVisibilityInfoForVoxelCoordinatesNotCompacted(uvec3 voxelCoordinates, uint voxelizationSize);

	/** Decode the 32-bit uint aprameter encodedValue into the index of the visible voxel (first 24 bits, 0-23), the
	* ray index used to come across that visible voxel in the voxel visibility ray tracing part
	* (bits 24-30) and a flag (bit 31) to know whether this is a static geometry direction from the
	* voxel visibility stage or a direction kept for testing visibility for dynamic scene elements
	* |31|30|29|28|27|26|25|24|23|22|21|20|19|18|17|16|15|14|13|12|11|10|9|8|7|6|5|4|3|2|1|0|
	*  fl|    ray Index       |                         voxel index                         |
	* @param encodedValue [in] encoded value with the visible voxel index, the ray direction index and whether it is a dynamic scene ray direction
	* @return uvec3 with x field having the decoded voxel index value, y field having the decoded ray direction index and 
	* z field having the decoded "is real time ray" flag value */
	static uvec3 decodeIndexAndIsRealTime(uint encodedValue);

	/** Verify the context of the dynamicVoxelBuffer bufer
	* @param voxelizationSize [in] voxelization size
	* @return nothing*/
	static void verifyDynamicVoxelBuffer(int voxelizationSize);

	/** Output dynamic visible voxel data for the voxel given as coordinates
	* @param voxelizationSize [in] voxelization size
	* @param voxelCoordinates [in] voxel coordinates to flook for visible dynamic voxels
	* @return nothing*/
	static void verifyDynamicVisibleVoxel(int voxelizationSize, uvec3 voxelCoordinates);

	static uint m_accumulatedReductionLevelBase; //!< Debug variable to know the accumulated value of non null elements at base level of the algorithm during the reduction step
	static uint m_accumulatedReductionLevel0;    //!< Debug variable to know the accumulated value of non null elements at level 0 of the algorithm during the reduction step
	static uint m_accumulatedReductionLevel1;    //!< Debug variable to know the accumulated value of non null elements at level 1 of the algorithm during the reduction step
	static uint m_accumulatedReductionLevel2;    //!< Debug variable to know the accumulated value of non null elements at level 2 of the algorithm during the reduction step
	static uint m_accumulatedReductionLevel3;    //!< Debug variable to know the accumulated value of non null elements at level 3 of the algorithm during the reduction step
	static bool m_outputAllInformationConsole;   //!< If true, all detailed information of all tests will be written to console
	static bool m_outputAllInformationFile;      //!< If true, all detailed information of all tests will be written to file
};

/////////////////////////////////////////////////////////////////////////////////////////////

#endif _BUFFERVERIFICATIONHELPER_H_
