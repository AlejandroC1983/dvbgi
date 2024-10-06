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

#ifndef _STATICNEIGHBOURINFORMATIONTECHNIQUE_H_
#define _STATICNEIGHBOURINFORMATIONTECHNIQUE_H_

// GLOBAL INCLUDES
#include "../../external/nano-signal-slot/nano_signal_slot.hpp"

// PROJECT INCLUDES
#include "../../include/rastertechnique/bufferprocesstechnique.h"

// CLASS FORWARDING
class Texture;
class VoxelVisibilityRayTracingTechnique;
class MaterialStaticNeighbourInformation;
class Buffer;
class MaterialBuildStaticVoxelTileBuffer;

// NAMESPACE

/////////////////////////////////////////////////////////////////////////////////////////////

// DEFINES
// TODO: unify in a single define in common .h file and remove all duplicates
typedef Nano::Signal<void()> SignaStaticNeighbourInformationCompletion;

/////////////////////////////////////////////////////////////////////////////////////////////

class StaticNeighbourInformationTechnique : public BufferProcessTechnique
{
	DECLARE_FRIEND_REGISTERER(StaticNeighbourInformationTechnique)

protected:
	/** Parameter constructor
	* @param name      [in] technique's name
	* @param className [in] class name
	* @return nothing */
	StaticNeighbourInformationTechnique(string &&name, string&& className);

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

	/** Called in record method before end recording command buffer, to allow any image / memory barriers
	* needed for the resources being used
	* @param commandBuffer [in] command buffer to record to
	* @return nothing */
	virtual void recordBarriersEnd(VkCommandBuffer* commandBuffer);

	/** Called inside each technique's while record and submit loop, after the call to prepare and before the
	* technique record, to update any dirty material that needs to be rebuilt as a consequence of changes in the
	* resources used by the materials present in m_vectorMaterial
	* @return nothing */
	virtual void postCommandSubmit();

	REF(SignaStaticNeighbourInformationCompletion, m_signaStaticNeighbourInformationCompletion, SignaStaticNeighbourInformationCompletion)
	GETCOPY(uint, m_occupiedStaticVoxelTileCounter, OccupiedStaticVoxelTileCounter)

protected:
	/** Slot to receive notification when the VoxelVisibilityRayTracingTechnique technique has completed
	* @return nothing */
	void slotVoxelVisibilityRayTracingComplete();

	SignaStaticNeighbourInformationCompletion m_signaStaticNeighbourInformationCompletion; //!< Signal for completion of the technique
	int                                       m_voxelizationResolution;                    //!< Voxelization resolution
	Texture*                                  m_staticVoxelNeighbourInfo;                  //!< 
	MaterialStaticNeighbourInformation*       m_materialStaticNeighbourInformation;        //!< Pointer the instance of the static neighbour information material, used to pre compute, for each static voxel, in a32-bit uint, which neighbours are occupied by other static voxels
	VoxelVisibilityRayTracingTechnique*       m_voxelVisibilityRayTracingTechnique;        //!< Pointer to the instance of the voxel visibility ray tracing technique
	Buffer*                                   m_occupiedStaticVoxelTileBuffer;             //!< Buffer with the hashed coordinates of all tiles with at least one static voxel within
	Buffer*                                   m_occupiedStaticVoxelTileCounterBuffer;      //!< Buffer to the instance of the occupiedStaticVoxelTileBuffer buffer with the amount of tiles in the occupiedStaticVoxelTileBuffer buffer
	uint                                      m_occupiedStaticVoxelTileCounter;            //!< Member variable with the value in the m_occupiedStaticVoxelTileCounterBuffer buffer
	int                                       m_tileSide;                                  //!< Copy of the raster flag TILE_SIZE_TOTAL
	MaterialBuildStaticVoxelTileBuffer*       m_materialBuildStaticVoxelTileBuffer;        //!< Pointer to the instance of the material used to buld the buffer with all tiles which have at least one static voxel
	Buffer*                                   m_occupiedStaticDebugBuffer;                 //!< Buffer for debug purposes
};

/////////////////////////////////////////////////////////////////////////////////////////////

#endif _STATICNEIGHBOURINFORMATIONTECHNIQUE_H_
