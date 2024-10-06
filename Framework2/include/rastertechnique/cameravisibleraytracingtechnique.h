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

#ifndef _CAMERAVISIBLERAYTRACINGTECHNIQUE_H_
#define _CAMERAVISIBLERAYTRACINGTECHNIQUE_H_

// GLOBAL INCLUDES
#include "../../external/nano-signal-slot/nano_signal_slot.hpp"

// PROJECT INCLUDES
#include "../../include/rastertechnique/rastertechnique.h"

// CLASS FORWARDING
class Buffer;
class MaterialCameraVisibleRayTracing;
class Camera;
class LitVoxelTechnique;
class VoxelVisibilityRayTracingTechnique;
class BufferPrefixSumTechnique;

// NAMESPACE

// DEFINES
// TODO: unify in a single define in common .h file and remove all duplicates
typedef Nano::Signal<void()> SignalCameraVisibleRayTracing;

/////////////////////////////////////////////////////////////////////////////////////////////

class CameraVisibleRayTracingTechnique: public RasterTechnique
{
	DECLARE_FRIEND_REGISTERER(CameraVisibleRayTracingTechnique)

protected:
	/** Parameter constructor
	* @param name      [in] technique's name
	* @param className [in] class name
	* @return nothing */
	CameraVisibleRayTracingTechnique(string &&name, string&& className);

	/** Default destructor
	* @return nothing */
	virtual ~CameraVisibleRayTracingTechnique();

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

	REF(SignalCameraVisibleRayTracing, m_signalCameraVisibleRayTracing, SignalCameraVisibleRayTracing)

protected:
	/** Build the shader binding table
	* @return nothing */
	void buildShaderBindingTable();

	/** Slot to receive signal when the prefix sum step has been done
	* @return nothing */
	void slotPrefixSumComplete();

	/** Slot to receive notification when the LitVoxelTechnique technique has been completed
	* @return nothing */
	void slotLitVoxelComplete();

	SignalCameraVisibleRayTracing       m_signalCameraVisibleRayTracing;         //!< Signal for completion of the technique
	MaterialCameraVisibleRayTracing*    m_materialCameraVisibleRayTracing;       //!< Pointer to the instance of the camera visible ray tracing
	Buffer*                             m_cameraVisibleShaderBindingTableBuffer; //!< Shader binding table buffer for the dynamic scene elements
	bool                                m_shaderBindingTableBuilt;               //!< Flag to know whther the shader binding table has been built
	uint                                m_shaderGroupBaseAlignment;              //!< Value of field VkPhysicalDeviceRayTracingPipelinePropertiesKHR::shaderGroupBaseAlignment
	uint                                m_shaderGroupHandleSize;                 //!< Value of field VkPhysicalDeviceRayTracingPipelinePropertiesKHR::shaderGroupHandleSize
	uint                                m_shaderGroupSizeAligned;                //!< Size of a shader group when considering aligned memory, for building teh shader binding table
	uint                                m_shaderGroupStride;                     //!< Stride of a shader group
	uint                                m_offscreenWidth;                        //!< Width of the offscreen texture used to store the ray tracing render pass results
	uint                                m_offscreenHeight;                       //!< Height of the offscreen texture used to store the ray tracing render pass resultsç
	Camera*                             m_mainCamera;                            //!< Pointer to the main camera used
	Buffer*                             m_cameraVisibleDebugBuffer;              //!< Pointer to the cameraVisibleDebugBuffer buffer for debugging purposes
	LitVoxelTechnique*                  m_litVoxelTechnique;                     //!< Pointer to the instance of the lit voxel techinque
	Buffer*                             m_cameraVisibleVoxelCompactedBuffer;     //!< Buffer storage buffer where to flag whether a voxel is visible from the camera as in m_cameraVisibleVoxelBuffer but with all the visible from camera voxel hashed indices starting from index 0
	Buffer*                             m_cameraVisibleVoxelDebugBuffer;         //!< Buffer storage buffer for debug purposes
	Buffer*                             m_cameraVisibleCounterBuffer;            //!< Buffer used as atomic counter for the camera visible voxels
	Buffer*                             m_cameraVisibleDynamicVoxelBuffer;       //!< Buffer to store all the dynamically generated voxels that are visible from camera, this buffer has size DynamicVoxelCopyToBufferTechnique::m_maxDynamicVoxel
	Buffer*                             m_cameraVisibleDynamicVoxelIrradianceBuffer; //!< Buffer to store the irradiance per voxel face of each of the camera dynamic visible voxels
	Buffer*                             m_dynamicVisibleVoxelCounterBuffer;      //!< Pointer the instance of the dynamic visible voxel counter buffer used to count how many voxels have been set as occupied in voxelOccupiedDynamicBuffer
	Texture*                            m_cameraVisibleNormal;                   //!< Texture with the normal information for the camera visible ray tracing pass
	Texture*                            m_cameraVisibleReflectance;              //!< Texture with the reflectance information for the camera visible ray tracing pass
	Texture*                            m_cameraVisiblePosition;                 //!< Texture with the position information for the camera visible ray tracing pass
	VoxelVisibilityRayTracingTechnique* m_voxelVisibilityRayTracingTechnique;    //!< Pointer to the instante of the VoxelVisibilityRayTracingTechnique technique
	uint                                m_numOccupiedVoxel;                      //!< Number of occupied voxel
	BufferPrefixSumTechnique*           m_bufferPrefixSumTechnique;              //!< Pointer to the prefix sum technique
};

/////////////////////////////////////////////////////////////////////////////////////////////

#endif _CAMERAVISIBLERAYTRACINGTECHNIQUE_H_
