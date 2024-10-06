/*
Copyright 2022 Alejandro Cosin

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

#ifndef _VOXELVISIBILITYRAYTRACINGTECHNIQUE_H_
#define _VOXELVISIBILITYRAYTRACINGTECHNIQUE_H_

// GLOBAL INCLUDES
#include "../../external/nano-signal-slot/nano_signal_slot.hpp"

// PROJECT INCLUDES
#include "../../include/rastertechnique/rastertechnique.h"

// CLASS FORWARDING
class MaterialVoxelVisibilityRayTracing;
class AccelerationStructure;
class Texture;
class Buffer;
class BufferPrefixSumTechnique;

// NAMESPACE

// DEFINES
// TODO: unify in a single define in common .h file and remove all duplicates
typedef Nano::Signal<void()> SignalVoxelVisibilityRayTracing;

/////////////////////////////////////////////////////////////////////////////////////////////

class VoxelVisibilityRayTracingTechnique: public RasterTechnique
{
	DECLARE_FRIEND_REGISTERER(VoxelVisibilityRayTracingTechnique)

protected:
	/** Parameter constructor
	* @param name      [in] technique's name
	* @param className [in] class name
	* @return nothing */
	VoxelVisibilityRayTracingTechnique(string &&name, string&& className);

	/** Default destructor
	* @return nothing */
	virtual ~VoxelVisibilityRayTracingTechnique();

public:
	/** New shaders, textures, etc initialization should be done here, called when the tehnique is instantiated
	* @return nothing */
	virtual void init();

	/** Called before rendering
	* @param dt [in] elapsed time in miliseconds since the last update call
	* @return nothing */
	virtual void prepare(float dt);

	/** Record command buffer
	* @param currentImage      [in] current screen image framebuffer drawing to (in case it's needed)
	* @param commandBufferID   [in] Unique identifier of the command buffer returned as parameter
	* @param commandBufferType [in] Queue to submit the command buffer recorded in the call
	* @return command buffer the technique has recorded to */
	virtual VkCommandBuffer* record(int currentImage, uint& commandBufferID, CommandBufferType& commandBufferType);

	/** Called inside each technique's while record and submit loop, after the call to prepare and before the
	* technique record, to update any dirty material that needs to be rebuilt as a consequence of changes in the
	* resources used by the materials present in m_vectorMaterial
	* @return nothing */
	virtual void postCommandSubmit();

	REF(SignalVoxelVisibilityRayTracing, m_signalVoxelVisibilityRayTracing, SignalVoxelVisibilityRayTracing)
	GETCOPY(uint, m_numElementToProcess, NumElementToProcess);
	GETCOPY(uint, m_numRaysPerVoxelFace, NumRaysPerVoxelFace);
	GETCOPY(int, m_maxDynamicVoxel, MaxDynamicVoxel)

protected:
	/** Build the shader binding table
	* @return nothing */
	void buildShaderBindingTable();

	/** Slot to receive signal when the prefix sum step has been done
	* @return nothing */
	void slotPrefixSumComplete();

	/** Slot to receive signal when the voxel merge step has been done
	* @return nothing */
	void slotVoxelMergeComplete();

	/** Builds a buffer with all the information from the precomputed samples per voxel face's triangle for the three different LOD levels available
	* @param vectorData [inout] Float vecotr with the final data
	* @return nothing */
	void prepareTriangleSamplingBuffers(vector<float>& vectorData);

	SignalVoxelVisibilityRayTracing    m_signalVoxelVisibilityRayTracing;    //!< Signal for completion of the technique
	MaterialVoxelVisibilityRayTracing* m_materialVoxelVisibilityRayTracing;  //!< Pointer to the instance of the voxel visibility ray tracing shader
	AccelerationStructure*             m_staticAccelerationStructure;        //!< Acceleration structure to be ray traced just for static scene geometry
	AccelerationStructure*             m_accelerationStructure;              //!< Acceleration structure to be ray traced having all scene geometry (static and dynamic)
	Texture*                           m_rayTracingOffscreen;                //!< Offscreen render target for the ray tracing pass
	Buffer*                            m_voxelVisibility4BytesBuffer;        //!< Pointer to the voxelVisibility4BytesBuffer buffer, where for each voxel the visibility of static voxels is cached. The information stored in each 32-bit uint is the distance from the voxel to the visible voxel (if any) in 16 bits and the hit normal in the other 16 bits encoded as 7 bits for x and y components and 1 bit for the z component's sign
	Buffer*                            m_voxelVisibilityCompactedBuffer;     //!< Pointer to the voxelVisibilityCompactedBuffer buffer, being the compacted version of the voxelVisibilityCompactedBuffer buffer
	Buffer*                            m_voxelVisibilityNumberBuffer;        //!< Pointer to the voxelVisibilityNumberBuffer buffer having, for each voxel face, the amount of visible voxels for that voxel face
	Buffer*                            m_voxelVisibilityDynamicNumberBuffer; //!< Pointer to the voxelVisibilityDynamicNumberBuffer buffer having, for each voxel face, the amount of directions that did not came across any static geometry but need to be taken into account when computing visibility for dynamic scene elements
	Buffer*                            m_voxelVisibilityFirstIndexBuffer;    //!< Pointer to the voxelVisibilityFirstIndexBuffer buffer having, for each voxel face, the index in voxelVisibilityCompactedBuffer where the visible voxels for that voxel face start
	Buffer*                            m_voxelVisibilityDebugBuffer;         //!< Pointer to the voxelVisibilityDebugBuffer for debugging purposes
	Buffer*                            m_triangleSampleDataBuffer;           //!< Pointer to the triangleSampleDataBuffer with all the information for rebuilding the precomputed samples per triangle
	Buffer*                            m_sceneDescriptorStaticBuffer;        //!< Buffer with the static part of the scene descriptor information
	Buffer*                            m_sceneDescriptorBuffer;              //!< Buffer with both the static and the dynamic part of the scene descriptor information
	Buffer*                            m_shaderBindingTableBuffer;           //!< Shader binding table buffer
	Buffer*                            m_dynamicVoxelBuffer;                 //!< Pointer the instance of the dynamic voxel buffer used to store the hashed coordinates of all the voxels generated dynamically after each execution of the DynamicSceneVoxelizationTechnique technique, it has a fixed size
	Buffer*                            m_dynamicVoxelVisibilityBuffer;       //!< Buffer to store all the visibility information for each of the camera visible dynamic voxels present in the cameraVisibleDynamicVoxelBuffer buffer
	BufferPrefixSumTechnique*          m_techniquePrefixSum;                 //!< Pointer to the instance of the prefix sum technique
	bool                               m_shaderBindingTableBuilt;            //!< Flag to know whther the shader binding table has been built
	uint                               m_shaderGroupBaseAlignment;           //!< Value of field VkPhysicalDeviceRayTracingPipelinePropertiesKHR::shaderGroupBaseAlignment
	uint                               m_shaderGroupHandleSize;              //!< Value of field VkPhysicalDeviceRayTracingPipelinePropertiesKHR::shaderGroupHandleSize
	uint                               m_shaderGroupSizeAligned;             //!< Size of a shader group when considering aligned memory, for building teh shader binding table
	uint                               m_shaderGroupStride;                  //!< Stride of a shader group
	vec4                               m_sceneMin;                           //!< Minimum value of the scene's aabb
	vec4                               m_sceneExtent;                        //!< Scene extent
	uint                               m_numOccupiedVoxel;                   //!< Number of occupied voxels after voxelization process
	bool                               m_prefixSumCompleted;                 //!< Flag to know if the prefix sum step has completed
	uint                               m_numElementToProcess;                //!< Number of elements to process
	uint                               m_numRaysPerVoxelFace;                //!< Number of rays from each voxel face
	int                                m_maxDynamicVoxel;                    //!< Max number of dynamic voxel that can be stored in buffers that store dynamic voxel-related information like dynamicVoxelBuffer
};

/////////////////////////////////////////////////////////////////////////////////////////////

#endif _VOXELVISIBILITYRAYTRACINGTECHNIQUE_H_
