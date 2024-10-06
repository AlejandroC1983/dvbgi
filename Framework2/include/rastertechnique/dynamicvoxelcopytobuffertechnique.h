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

#ifndef _DYNAMICVOXELCOPYTOBUFFERTECHNIQUE_H_
#define _DYNAMICVOXELCOPYTOBUFFERTECHNIQUE_H_

// GLOBAL INCLUDES
#include "../../external/nano-signal-slot/nano_signal_slot.hpp"

// PROJECT INCLUDES
#include "../../include/rastertechnique/bufferprocesstechnique.h"

// CLASS FORWARDING
class Buffer;
class MaterialDynamicVoxelCopyToBuffer;
class ResetLitvoxelData;

// NAMESPACE

/////////////////////////////////////////////////////////////////////////////////////////////

// DEFINES
// TODO: unify in a single define in common .h file and remove all duplicates
typedef Nano::Signal<void()> SignalDynamicVoxelCopyToBufferCompletion;

/////////////////////////////////////////////////////////////////////////////////////////////

class DynamicVoxelCopyToBufferTechnique : public BufferProcessTechnique
{
	DECLARE_FRIEND_REGISTERER(DynamicVoxelCopyToBufferTechnique)

protected:
	/** Parameter constructor
	* @param name      [in] technique's name
	* @param className [in] class name
	* @return nothing */
	DynamicVoxelCopyToBufferTechnique(string &&name, string&& className);

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

	REF(SignalDynamicVoxelCopyToBufferCompletion, m_signalDynamicVoxelCopyToBufferCompletion, SignalDynamicVoxelCopyToBufferCompletion)
	GETCOPY(uint, m_dynamicVoxelCounter, DynamicVoxelCounter)

protected:
	/** Slot to receive signal when the prefix sum step has been done
	* @return nothing */
	void slotPrefixSumComplete();

	/** Slot to receive notification when the LitVoxelTechnique technique has been completed
	* @return nothing */
	void slotResetLitVoxelComplete();

	SignalDynamicVoxelCopyToBufferCompletion m_signalDynamicVoxelCopyToBufferCompletion; //!< Signal for completion of the technique
	Buffer*                                  m_dynamicVoxelCounterBuffer;                //!< Pointer the instance of the dynamic voxel counter buffer used to count how many voxels have been set as occupied in voxelOccupiedDynamicBuffer
	Buffer*                                  m_dynamicvoxelCopyDebugBuffer;              //!< Buffer for debugging purposes
	uint                                     m_dynamicVoxelCounter;                      //!< Variable where to store the value of m_dynamicVoxelCounterBuffer
	int                                      m_voxelizationResolution;                   //!< Voxelization resolution
	MaterialDynamicVoxelCopyToBuffer*        m_materialDynamicVoxelCopyToBuffer;         //!< Pointer to the copy to dynamic voxel buffer material instance
	ResetLitvoxelData* m_resetLitvoxelData;
};

/////////////////////////////////////////////////////////////////////////////////////////////

#endif _DYNAMICVOXELCOPYTOBUFFERTECHNIQUE_H_
