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

#ifndef _RESETLITVOXELDATATECHNIQUE_H_
#define _RESETLITVOXELDATATECHNIQUE_H_

// GLOBAL INCLUDES
#include "../../external/nano-signal-slot/nano_signal_slot.hpp"

// PROJECT INCLUDES
#include "../../include/rastertechnique/rastertechnique.h"

// CLASS FORWARDING
class BufferPrefixSumTechnique;

// NAMESPACE

// DEFINES
typedef Nano::Signal<void()> SignalResetLitvoxelDataCompletion;

/////////////////////////////////////////////////////////////////////////////////////////////

class ResetLitvoxelData : public RasterTechnique
{
	DECLARE_FRIEND_REGISTERER(ResetLitvoxelData)

protected:
	/** Parameter constructor
	* @param name      [in] technique's name
	* @param className [in] class name
	* @return nothing */
	ResetLitvoxelData(string &&name, string&& className);

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

	/** Slot to receive notification when the camera values (look at / position) are dirty
	* @return nothing */
	void slotCameraDirty();

	/** Slot to receive notification when the scene is dirty
	* @return nothing */
	void slotSceneDirty();

	/** Function called from slotCameraDirty and slotSceneDirty
	* @return nothing */
	void slotDirtyUpdateValues();

	SET(bool, m_techniqueLock, TechniqueLock)
	REF(SignalResetLitvoxelDataCompletion, m_signalResetLitvoxelDataCompletion, SignalResetLitvoxelDataCompletion)

protected:
	/** Slot to receive notification when the prefix sum of the scene voxelization has been completed
	* @return nothing */
	void slotPrefixSumComplete();

	SignalResetLitvoxelDataCompletion m_signalResetLitvoxelDataCompletion;                //!< Signal for reset lit voxel data completion
	Buffer*                           m_litTestVoxelPerByteBuffer;                        //!< Buffer of lit test voxels having information on a per-byte approach (each byte has information on whether any voxel face is lit and what voxel faces are lit at bit level, with info [no info][-x][+x][-y][+y][-z][+z][AnyLitVoxelFace])
	Buffer*                           m_litTestDynamicVoxelPerByteBuffer;                 //!< Buffer of dynamic lit test voxels having information on a per-byte approach (each byte has information on whether any voxel face is lit and what voxel faces are lit at bit level, with info [no info][-x][+x][-y][+y][-z][+z][AnyLitVoxelFace])
	Buffer*                           m_cameraVisibleVoxelPerByteBuffer;                  //!< Buffer of camera visible test static voxels having information on a per-byte approach (each byte of the buffer has information whether a dynamic voxel is vislbe from camera or not, representing each byte's index in the buffer the hashed index of a voxel's coordinates)
	Buffer*                           m_cameraVisibleDynamicVoxelPerByteBuffer;           //!< Buffer of camera visible test dynamic voxels having information on a per-byte approach (each byte of the buffer has information whether a dynamic voxel is vislbe from camera or not, representing each byte's index in the buffer the hashed index of a voxel's coordinates)
	Buffer*							  m_cameraVisibleDynamicVoxelPerByteTagProcessBuffer; //!< Buffer which will be used in ProcessCameraVisibleResultsTechnique to take all the occupied voxels from m_cameraVisibleDynamicVoxelPerByteBuffer and set those and their neighbouring voxels for processing
	Buffer*                           m_resetLitVoxelDataDebugBuffer;                     //!< Buffer for debug purposes
	Buffer*                           m_irradiancePaddingTagTilesBuffer;                  //!< Pointer to the buffer used to tag those 2^3 tiles in the voxelization volume which have at least one empty voxel that has at least one neighbour occupied static voxel that need padding to be computed
	Buffer*                           m_irradianceFilteringTagTilesBuffer;                //!< Pointer to the buffer used to tag those 2^3 tiles in the voxelization volume which have at least one occupied voxel that will be processed for filtering together with neighbouring voxels
	Buffer*                           m_dynamicVoxelVisibilityBuffer;                     //!< Buffer to store all the visibility information for each of the camera visible dynamic voxels present in the cameraVisibleDynamicVoxelBuffer buffer
	BufferPrefixSumTechnique*         m_bufferPrefixSumTechnique;                         //!< Pointer to the prefix sum technique
	Camera*                           m_mainCamera;                                       //! Scene main camera
	Camera*                           m_emitterCamera;                                    //! Emitter camera
	bool                              m_prefixSumCompleted;                               //!< Flag to know if the prefix sum step has completed
	bool                              m_techniqueLock;                                    //!< Flag to avoid the technique to request new command buffer record before all the chained techniques finish their corresponding command buffers executions
	int                               m_voxelizationWidth;                                //!< Copy of StaticSceneVoxelizationTechnique::m_voxelizedSceneWidth
	Buffer*                           m_voxelOccupiedDynamicBuffer;                       //!< Buffer with the information occupied voxels from dynamic scene elements
};

/////////////////////////////////////////////////////////////////////////////////////////////

#endif _RESETLITVOXELDATATECHNIQUE_H_
