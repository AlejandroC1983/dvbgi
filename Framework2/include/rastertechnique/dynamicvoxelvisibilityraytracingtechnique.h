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

#ifndef _DYNAMICVOXELVISIBILITYRAYTRACINGTECHNIQUE_H_
#define _DYNAMICVOXELVISIBILITYRAYTRACINGTECHNIQUE_H_

// GLOBAL INCLUDES
#include "../../external/nano-signal-slot/nano_signal_slot.hpp"

// PROJECT INCLUDES
#include "../../include/rastertechnique/rastertechnique.h"

// CLASS FORWARDING
class MaterialDynamicVoxelVisibilityRayTracing;
class AccelerationStructure;
class Texture;
class Buffer;
class BufferPrefixSumTechnique;
class ProcessCameraVisibleResultsTechnique;
class Camera;

// NAMESPACE

// DEFINES
// TODO: unify in a single define in common .h file and remove all duplicates
typedef Nano::Signal<void()> SignalDynamicVoxelVisibilityRayTracing;

/////////////////////////////////////////////////////////////////////////////////////////////

class DynamicVoxelVisibilityRayTracingTechnique: public RasterTechnique
{
	DECLARE_FRIEND_REGISTERER(DynamicVoxelVisibilityRayTracingTechnique)

protected:
	/** Parameter constructor
	* @param name      [in] technique's name
	* @param className [in] class name
	* @return nothing */
	DynamicVoxelVisibilityRayTracingTechnique(string &&name, string&& className);

	/** Default destructor
	* @return nothing */
	virtual ~DynamicVoxelVisibilityRayTracingTechnique();

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

	REF(SignalDynamicVoxelVisibilityRayTracing, m_signalDynamicVoxelVisibilityRayTracing, SignalDynamicVoxelVisibilityRayTracing)

protected:
	/** Build the shader binding table
	* @return nothing */
	void buildShaderBindingTable();

	/** Slot to receive signal when the prefix sum step has been done
	* @return nothing */
	void slotPrefixSumComplete();

	/** Slot to receive notification when the CameraVisibleVoxelTechnique has completed
	* @return nothing */
	void slotCameraVisibleVoxelCompleted();

	SignalDynamicVoxelVisibilityRayTracing    m_signalDynamicVoxelVisibilityRayTracing;   //!< Signal for completion of the technique
	MaterialDynamicVoxelVisibilityRayTracing* m_materialDynamicVoxelVisibilityRayTracing; //!< Pointer to the instance of the voxel visibility ray tracing shader used for the dynamc part of the scene
	AccelerationStructure*                    m_dynamicAccelerationStructure;             //!< Dynamic scene element acceleration structure to be ray traced
	AccelerationStructure*                    m_accelerationStructure;                    //!< Static and dynamic scene element acceleration structure to be ray traced
	Buffer*                                   m_voxelVisibilityDynamic4ByteBuffer;        //!< Pointer to the voxelVisibilityDynamic4ByteBuffer buffer, having for each set of m_numThreadPerLocalWorkgroup elements and for each voxel face, the distance from the current voxel for each sampling direction of any dynamic intersected geometry (2 bytes) as well as the intersection normal direction as 2 bytes (xy components of the normal vector)
	Buffer*                                   m_dynamicVoxelVisibilityDebugBuffer;        //!< Pointer to the dynamicVoxelVisibilityDebugBuffer for debugging purposes
	Buffer*                                   m_sceneDescriptorDynamicBuffer;             //!< Buffer with the dynamic scene descriptor information
	Buffer*                                   m_dynamicShaderBindingTableBuffer;          //!< Shader binding table buffer for the dynamic scene elements
	Buffer*                                   m_dynamicVoxelVisibilityFlagBuffer;         //!< Buffer with the dynamic voxel vsibility information: For each voxel face, 4 uint elements are considered to tag as static (0) / dynamic (1) visible voxels for the corresponding voxel face, mapping up to 128 voxel face rays (curently 36 are being used).
	BufferPrefixSumTechnique*                 m_techniquePrefixSum;                       //!< Pointer to the instance of the prefix sum technique
	bool                                      m_shaderBindingTableBuilt;                  //!< Flag to know whther the shader binding table has been built
	uint                                      m_shaderGroupBaseAlignment;                 //!< Value of field VkPhysicalDeviceRayTracingPipelinePropertiesKHR::shaderGroupBaseAlignment
	uint                                      m_shaderGroupHandleSize;                    //!< Value of field VkPhysicalDeviceRayTracingPipelinePropertiesKHR::shaderGroupHandleSize
	uint                                      m_shaderGroupSizeAligned;                   //!< Size of a shader group when considering aligned memory, for building teh shader binding table
	uint                                      m_shaderGroupStride;                        //!< Stride of a shader group
	vec4                                      m_sceneMin;                                 //!< Minimum value of the scene's aabb
	vec4                                      m_sceneExtent;                              //!< Scene extent
	uint                                      m_numStaticOccupiedVoxel;                   //!< Number of occupied voxel after voxelization process of static scene geoemtry
	bool                                      m_prefixSumCompleted;                       //!< Flag to know if the prefix sum step has completed
	uint                                      m_numRaysPerVoxelFace;                      //!< Number of rays from each voxel face
	uint                                      m_numElementToProcess;                      //!< Number of elements to process
	uint                                      m_cameraVisibleVoxelNumber;                 //!< Number of visible voxel determined by the CameraVisibleVoxelTechnique technique
	ProcessCameraVisibleResultsTechnique*     m_processCameraVisibleResultsTechnique;     //! Pointer to the instance of the process camera visible results technique
	Camera*                                   m_emitterCamera;                            //!< Camera emitter
};

/////////////////////////////////////////////////////////////////////////////////////////////

#endif _DYNAMICVOXELVISIBILITYRAYTRACINGTECHNIQUE_H_
