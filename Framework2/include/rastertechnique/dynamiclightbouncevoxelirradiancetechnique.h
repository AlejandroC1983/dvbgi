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

#ifndef _DYNAMICLIGHTBOUNCEVOXELIRRADIANCETECHNIQUE_H_
#define _DYNAMICLIGHTBOUNCEVOXELIRRADIANCETECHNIQUE_H_

// GLOBAL INCLUDES
#include "../../external/nano-signal-slot/nano_signal_slot.hpp"

// PROJECT INCLUDES
#include "../../include/rastertechnique/rastertechnique.h"

// CLASS FORWARDING
class LightBounceVoxelIrradianceTechnique;
class ProcessCameraVisibleResultsTechnique;
class ResetLitvoxelData;
class MaterialLightBounceDynamicVoxelIrradiance;
class Camera;
class Buffer;

// NAMESPACE

// DEFINES
typedef Nano::Signal<void()> SignalDynamicLightBounceVoxelIrradianceCompletion;
#define NUM_VOXEL_FACE           6
#define NUM_RAY_PER_VOXEL_FACE 128 // TODO: Use as raster flag value as a single parameter 

/////////////////////////////////////////////////////////////////////////////////////////////

class DynamicLightBounceVoxelIrradianceTechnique : public RasterTechnique
{
	DECLARE_FRIEND_REGISTERER(DynamicLightBounceVoxelIrradianceTechnique)

protected:
	/** Parameter constructor
	* @param name      [in] technique's name
	* @param className [in] class name
	* @return nothing */
	DynamicLightBounceVoxelIrradianceTechnique(string &&name, string&& className);

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

	REF(SignalDynamicLightBounceVoxelIrradianceCompletion, m_signalDynamicLightBounceVoxelIrradianceCompletion, SignalDynamicLightBounceVoxelIrradianceCompletion)

protected:
	/** Slot to receive notification when the LightBounceVoxelIrradianceTechnique has completed
	* @return nothing */
	void slotLightBounceVoxelIrradianceCompleted();

	/** Build the shader binding table
	* @return nothing */
	void buildShaderBindingTable();

	SignalDynamicLightBounceVoxelIrradianceCompletion m_signalDynamicLightBounceVoxelIrradianceCompletion; //!< Signal for when the technique has completed execution (called in post command submit method, postCommandSubmit)
	LightBounceVoxelIrradianceTechnique*              m_lightBounceVoxelIrradianceTechnique;               //!< Pointer to the instance of the light bounce voxel irradiance technique
	ProcessCameraVisibleResultsTechnique*             m_processCameraVisibleResultsTechnique;              //! Pointer to the instance of the process camera visible results technique
	ResetLitvoxelData*                                m_resetLitvoxelData;                                 //!< Poiner to the reset lit voxel data technique
	Camera*                                           m_emitterCamera;                                     //!< Camera emitter
	MaterialLightBounceDynamicVoxelIrradiance*        m_materialLightBounceDynamicVoxelIrradiance;         //!< Pointer to the instance of the light bounce for dynamic voxels
	Buffer*                                           m_dynamicLightBounceDebugBuffer;                     //!< Buffer for debug purposes
	vec4                                              m_sceneMinAndCameraVisibleVoxel;                     //!< Minimum value of the scene's aabb and amount of dynamic visible voxels from the camera in the .w field
	vec4                                              m_sceneExtentAndNumElement;                          //!< Extent of the scene and number of elements to be processed by the dispatch
	float                                             m_voxelizationWidth;                                 //!< Width of the voxelization resolition chosen to voxelize the scene
	uint                                              m_numVisibleDynamicVoxel;                            //!< Number of dynamic voxels vislbe from camera
	uint                                              m_shaderGroupBaseAlignment;                          //!< Value of field VkPhysicalDeviceRayTracingPipelinePropertiesKHR::shaderGroupBaseAlignment
	uint                                              m_shaderGroupHandleSize;                             //!< Value of field VkPhysicalDeviceRayTracingPipelinePropertiesKHR::shaderGroupHandleSize
	uint                                              m_shaderGroupSizeAligned;                            //!< Size of a shader group when considering aligned memory, for building teh shader binding table
	uint                                              m_shaderGroupStride;                                 //!< Stride of a shader group
	Buffer*                                           m_dynamicLightBounceShaderBindingTableBuffer;        //!< Shader binding table buffer for the dynamic scene elements
	bool                                              m_shaderBindingTableBuilt;                           //!< Flag to know whther the shader binding table has been built
};

/////////////////////////////////////////////////////////////////////////////////////////////

#endif _DYNAMICLIGHTBOUNCEVOXELIRRADIANCETECHNIQUE_H_
