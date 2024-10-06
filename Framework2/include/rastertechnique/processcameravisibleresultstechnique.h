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

#ifndef _PROCESSCAMERAVISIBLERESULTSTECHNIQUE_H_
#define _PROCESSCAMERAVISIBLERESULTSTECHNIQUE_H_

// GLOBAL INCLUDES
#include "../../external/nano-signal-slot/nano_signal_slot.hpp"

// PROJECT INCLUDES
#include "../../include/rastertechnique/bufferprocesstechnique.h"

// CLASS FORWARDING
class RayTracingDeferredShadowsTechnique;
class MaterialProcessCameraVisibleResults;
class MaterialTagCameraVisibleVoxelNeighbourTile;
class MaterialTemporalFilteringCleanDynamic3DTextures;

// NAMESPACE

// DEFINES
typedef Nano::Signal<void()> SignalProcessCameraVisibleCompletion;

/////////////////////////////////////////////////////////////////////////////////////////////

class ProcessCameraVisibleResultsTechnique : public BufferProcessTechnique
{
	DECLARE_FRIEND_REGISTERER(ProcessCameraVisibleResultsTechnique)

protected:
	/** Parameter constructor
	* @param name      [in] technique's name
	* @param className [in] class name
	* @return nothing */
	ProcessCameraVisibleResultsTechnique(string &&name, string&& className);

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

	REF(SignalProcessCameraVisibleCompletion, m_signalProcessCameraVisibleCompletion, SignalProcessCameraVisibleCompletion)
	GETCOPY(uint, m_staticCameraVisibleVoxelNumber, StaticCameraVisibleVoxelNumber)
	GETCOPY(uint, m_dynamicCameraVisibleVoxelNumber, DynamicCameraVisibleVoxelNumber)
	GETCOPY(uint, m_irradiancePaddingTagTilesNumber, IrradiancePaddingTagTilesNumber)
	GETCOPY(uint, m_irradianceFilteringTagTilesNumber, IrradianceFilteringTagTilesNumber)

protected:
	/** Slot to receive notification when the camrera visible ray tracing technique has been completed
	* @return nothing */
	void slotCameraVisibleRayTracingComplete();

	SignalProcessCameraVisibleCompletion             m_signalProcessCameraVisibleCompletion;            //!< Signal for when the technique has completed execution (called in post command submit method, postCommandSubmit)
	bool                                             m_prefixSumCompleted;                              //!< Flag to know if the prefix sum step has completed
	RayTracingDeferredShadowsTechnique*              m_rayTracingDeferredShadowsTechnique;              //! Pointer to the instance of the ray tracing deferred shadows technique
	Buffer*                                          m_staticVoxelVisibleCounterBuffer;                 //!< Buffer to the instance of the staticVoxelVisibleCounterBuffer buffer used to count how many static voxels are visible from the camera
	Buffer*                                          m_dynamicVoxelVisibleCounterBuffer;                //!< Buffer to the instance of the dynamicVoxelVisibleCounterBuffer buffer used to count how many dynamic voxels are visible from the camera
	uint                                             m_staticCameraVisibleVoxelNumber;                  //!< Where to put the last recovered static voxel camera visible voxel result from m_staticVoxelVisibleCounterBuffer buffer
	uint                                             m_dynamicCameraVisibleVoxelNumber;                 //!< Where to put the last recovered static voxel camera visible voxel result from m_dynamicVoxelVisibleCounterBuffer buffer
	Buffer*                                          m_processCameraVisibleResultsDebugBuffer;          //!< Pointer to the processCameraVisibleResultsDebugBuffer buffer for debugging purposes
	Buffer*                                          m_irradiancePaddingTagTilesCounterBuffer;          //!< Pointer to the instance of the m_irradiancePaddingTagTilesCounterBuffer buffer used to count how many tiles are being tagged to be used in the padding computation step
	Buffer*                                          m_irradianceFilteringTagTilesCounterBuffer;        //!< Pointer to the instance of the m_irradianceFilteringTagTilesCounterBuffer buffer used to count how many tiles are being tagged to be used in the filtering computation step
	Buffer*                                          m_irradiancePaddingTagTilesIndexBuffer;            //!< Pointer to the instance of the m_irradiancePaddingTagTilesIndexBuffer buffer used to store the indices of the tiles that need to be processed for the padding computation
	Buffer*                                          m_irradianceFilteringTagTilesIndexBuffer;          //!< Pointer to the instance of the irradianceFilteringTagTilesIndexBuffer buffer used to store the indices of the tiles that need to be processed for the filtering computation
	Buffer*                                          m_tagCameraVisibleDebugBuffer;                     //!< Pointer a buffer used for debugging purposes
	Buffer*                                          m_dynamicIrradianceTrackingBuffer;                 //!< Pointer the buffer used to track dynamic voxel irradiance generated in the last frame and that needs to be tracked to avoid leaving dynamic voxels with permanent irradiance from moving objects, following a cool down / fading time, which neaches 0 is used to reset the irradiance value
	Buffer*                                          m_staticIrradianceTrackingBuffer;                  //!< Pointer the buffer used to track static voxel irradiance generated in the last frame and that needs to be tracked to avoid leaving static voxels with permanent irradiance from moving objects, following a cool down / fading time, which neaches 0 is used to reset the irradiance value
	Buffer*                                          m_dynamicIrradianceTrackingDebugBuffer;            //!< Pointer a buffer used for debug purposes
	Buffer*                                          m_dynamicIrradianceTrackingCounterDebugBuffer;     //!< Pointer to a buffer used as atomic counter for debuggign purposes
	uint                                             m_irradiancePaddingTagTilesNumber;                 //!< Where to put the last recovered number of tagged tiles from the irradiancePaddingTagTilesCounterBuffer buffer
	uint                                             m_irradianceFilteringTagTilesNumber;               //!< Where to put the last recovered number of tagged tiles from the irradianceFilteringTagTilesCounterBuffer buffer
	MaterialProcessCameraVisibleResults*             m_materialProcessCameraVisibleResults;             //!< Pointer to the instance of the material used to process camera visible static and dyamic voxels
	MaterialTagCameraVisibleVoxelNeighbourTile*      m_materialTagCameraVisibleVoxelNeighbourTile;      //!< Pointer to the instance of the material used to tag all padding voxels for static visible voxels, surrounding voxels of dynamic voxels and the tiles to process them
	MaterialTemporalFilteringCleanDynamic3DTextures* m_materialTemporalFilteringCleanDynamic3DTextures; //!< Pointer to the instance of the material used to track dynamic voxel irradiance values and clear them once the cool down time has passed
};

/////////////////////////////////////////////////////////////////////////////////////////////

#endif _PROCESSCAMERAVISIBLERESULTSTECHNIQUE_H_
