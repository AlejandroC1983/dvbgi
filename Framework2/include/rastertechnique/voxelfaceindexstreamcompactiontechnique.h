/*
Copyright 2021 Alejandro Cosin

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

#ifndef _VOXELFACEINDEXSTREAMCOMPACTIONTECHNIQUE_H_
#define _VOXELFACEINDEXSTREAMCOMPACTIONTECHNIQUE_H_

// GLOBAL INCLUDES
#include "../../external/nano-signal-slot/nano_signal_slot.hpp"

// PROJECT INCLUDES
#include "../../include/rastertechnique/bufferprocesstechnique.h"

// CLASS FORWARDING
class BufferPrefixSumTechnique;
class VoxelVisibilityRayTracingTechnique;

// NAMESPACE

// DEFINES
// TODO: unify in a single define in common .h file and remove all duplicates
typedef Nano::Signal<void()> SignalVoxelFaceIndexStreamCompactionTechnique;

/////////////////////////////////////////////////////////////////////////////////////////////

class VoxelFaceIndexStreamCompactionTechnique : public BufferProcessTechnique
{
	DECLARE_FRIEND_REGISTERER(VoxelFaceIndexStreamCompactionTechnique)

protected:
	/** Parameter constructor
	* @param name      [in] technique's name
	* @param className [in] class name
	* @return nothing */
	VoxelFaceIndexStreamCompactionTechnique(string&& name, string&& className);

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

	REF(SignalVoxelFaceIndexStreamCompactionTechnique, m_signalVoxelFaceIndexStreamCompactionTechnique, SignalVoxelFaceIndexStreamCompactionTechnique)

protected:
	/** Slot to receive signal when the prefix sum step has been done
	* @return nothing */
	void slotPrefixSumComplete();

	/** Slot to receive signal when the voxel visibility ray tracing step has been done
	* @return nothing */
	void slotVoxelVisibilityRayTracingMergeComplete();

	SignalVoxelFaceIndexStreamCompactionTechnique m_signalVoxelFaceIndexStreamCompactionTechnique; //!< Signal for completion of the technique
	BufferPrefixSumTechnique*                     m_techniquePrefixSum;                            //!< Pointer to the instance of the prefix sum technique
	VoxelVisibilityRayTracingTechnique*           m_voxelVisibilityRayTracingTechnique;            //!< Pointer to the instance of the voxel visibility ray tracing technique
	Buffer*                                       m_voxelVisibilityBuffer;                         //!< Pointer to the voxelVisibilityBuffer buffer, having for each set of m_numThreadPerLocalWorkgroup elements and for each voxel face, the indices of the visible voxels from that voxel face, in sets of m_numThreadPerLocalWorkgroup elements (this buffer is latter compacted to save memory since in many cases most of the m_numThreadPerLocalWorkgroup elements will be not used)
	Buffer*                                       m_voxelVisibilityNumberBuffer;                   //!< Pointer to the voxelVisibilityNumberBuffer buffer having, for each voxel face, the amount of visible voxels for that voxel face
	Buffer*                                       m_voxelVoxelFaceIndexDebugBuffer;                //!< Pointer to the voxelVoxelFaceIndexDebugBuffer for debugging purposes
	uint                                          m_numOccupiedVoxel;                              //!< Number of occupied voxels after voxelization process
	bool                                          m_prefixSumCompleted;                            //!< Flag to know if the prefix sum step has completed
};

/////////////////////////////////////////////////////////////////////////////////////////////

#endif _VOXELFACEINDEXSTREAMCOMPACTIONTECHNIQUE_H_
