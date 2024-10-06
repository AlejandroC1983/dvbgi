/*
Copyright 2023 Alejandro Cosin

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

#ifndef _LIGTHBOUNCEDYNAMICVOXELIRRADIANCETECHNIQUE_H_
#define _LIGTHBOUNCEDYNAMICVOXELIRRADIANCETECHNIQUE_H_

// GLOBAL INCLUDES
#include "../../external/nano-signal-slot/nano_signal_slot.hpp"

// PROJECT INCLUDES
#include "../../include/rastertechnique/bufferprocesstechnique.h"

// CLASS FORWARDING
class BufferPrefixSumTechnique;
class LitVoxelTechnique;
class Camera;
class ProcessCameraVisibleResultsTechnique;
class MaterialLightBounceDynamicVoxelIrradianceCompute;
class Buffer;

// NAMESPACE

// DEFINES
#define NUM_RAY_PER_VOXEL_FACE 128 // TODO: Use as raster flag value as a single parameter 

// TODO: unify in a single define in common .h file and remove all duplicates
typedef Nano::Signal<void()> SignalLightBounceDynamicVoxelIrradianceCompletion;

/////////////////////////////////////////////////////////////////////////////////////////////

class LightBounceDynamicVoxelIrradianceTechnique: public BufferProcessTechnique
{
	DECLARE_FRIEND_REGISTERER(LightBounceDynamicVoxelIrradianceTechnique)

protected:
	/** Parameter constructor
	* @param name      [in] technique's name
	* @param className [in] class name
	* @return nothing */
	LightBounceDynamicVoxelIrradianceTechnique(string &&name, string&& className);

public:
	/** New shaders, textures, etc initialization should be done here, called when the tehnique is instantiated
	* @return nothing */
	virtual void init();

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

	REF(SignalLightBounceDynamicVoxelIrradianceCompletion, m_signalLightBounceDynamicVoxelIrradianceCompletion, SignalLightBounceDynamicVoxelIrradianceCompletion)

protected:
	/** Slot to receive signal when the prefix sum step has been done
	* @return nothing */
	void slotPrefixSumComplete();

	/** Slot to receive notification when the ProcessCameraVisibleResultsTechnique has completed
	* @return nothing */
	void slotProcessCameraVisibleResultsTechniqueCompleted();

	SignalLightBounceDynamicVoxelIrradianceCompletion m_signalLightBounceDynamicVoxelIrradianceCompletion; //!< Signal for completion of the technique
	BufferPrefixSumTechnique*                         m_techniquePrefixSum;                                //!< Pointer to the instance of the prefix sum technique
	LitVoxelTechnique*                                m_litVoxelTechnique;                                 //!< Pointer to the lit voxel technique
	uint                                              m_dynamicVisibleVoxel;                               //!< Number of dynamic visible voxels after the ProcessCameraVisibleResultsTechnique pass
	bool                                              m_prefixSumCompleted;                                //!< Flag to know if the prefix sum step has completed
	ProcessCameraVisibleResultsTechnique*             m_processCameraVisibleResultsTechnique;              //!< Pointer to the instance of the process camera visible results technique
	MaterialLightBounceDynamicVoxelIrradianceCompute* m_materialLightBounceDynamicVoxelIrradianceCompute;  //!< Pointer to the instance of the MaterialLightBounceDynamicVoxelIrradianceCompute material use in this technique
	vec4                                              m_sceneMin;                                          //!< Minimum value of the scene's aabb
	vec4                                              m_sceneExtent;                                       //!< Scene extent
	Camera*                                           m_emitterCamera;                                     //!< Emitter camera
	float                                             m_emitterRadiance;                                   //!< Radiance of the emitter
	Buffer*                                           m_lightBounceDynamicVoxelIrradianceDebugBuffer;      //!< Buffer for debug purposes
};

/////////////////////////////////////////////////////////////////////////////////////////////

#endif _LIGTHBOUNCEDYNAMICVOXELIRRADIANCETECHNIQUE_H_
