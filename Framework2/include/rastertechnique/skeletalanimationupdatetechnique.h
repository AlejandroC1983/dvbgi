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

#ifndef _SKELETALANIMATIONUPDATETECHNIQUE_H_
#define _SKELETALANIMATIONUPDATETECHNIQUE_H_

// GLOBAL INCLUDES
#include "../../external/nano-signal-slot/nano_signal_slot.hpp"

// PROJECT INCLUDES
#include "../../include/rastertechnique/bufferprocesstechnique.h"

// CLASS FORWARDING
class Buffer;
class MaterialSkeletalAnimationUpdate;
class AccelerationStructure;

// NAMESPACE

/////////////////////////////////////////////////////////////////////////////////////////////

// DEFINES
// TODO: unify in a single define in common .h file and remove all duplicates
typedef Nano::Signal<void()> SignaSkeletalAnimationCompletion;

/////////////////////////////////////////////////////////////////////////////////////////////

class SkeletalAnimationUpdateTechnique : public BufferProcessTechnique
{
	DECLARE_FRIEND_REGISTERER(SkeletalAnimationUpdateTechnique)

protected:
	/** Parameter constructor
	* @param name      [in] technique's name
	* @param className [in] class name
	* @return nothing */
	SkeletalAnimationUpdateTechnique(string &&name, string&& className);

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

protected:
	/** Slot to receive notification when the VoxelVisibilityRayTracingTechnique technique has completed
	* @return nothing */
	void slotVoxelVisibilityRayTracingComplete();

	vectorNodePtr                       m_vectorSkinnedMesh;                     //!< Vector with the nodes which are skinned mesh and will be updated
	vectorSkinnedMeshRenderComponentPtr m_vectorSkinnedMeshComponent;            //!< Vector with all skinned mesh components present in the scene nodes
	Buffer*                             m_poseMatrixBuffer;                      //!< Pointer to the buffer containing the matrices with the pose information used by each skeletal mesh to be updated
	MaterialSkeletalAnimationUpdate*    m_materialSkeletalAnimationUpdate;       //!< Pointer to the instance of the skeletal animation update material
	vectorFloat                         m_vectorPoseMatrixData;                  //!< Vector with all the matrix data for each skeletal mesh to be updated
	Buffer*                             m_skeletalMeshDebugBuffer;               //!< Buffer for debug purposes
	vectorFloat                         m_boneMatrixOffset;                      //!< Vector with the element offset in the poseMatrix buffer for loading the bone transform information for each skeletal mesh, used for the push constants recorded for each skeletal animation dispatch
	vectorFloat                         m_vectorNumVertexToProcess;              //!< Vector with the amount of vertices to process for each skeletal mesh, used for the push constants recorded for each skeletal animation dispatch
	vectorFloat                         m_vectorVertexOffset;                    //!< Vector with the offset in the buffer vertexBufferSkinnedMeshOriginal for each skeletal mesh being procesed in the each compute dispatch
	int                                 m_numSkinnedMeshComponent;               //!< Number of skinned mesh components to update its skeletal mesh
	Buffer*                             m_sceneLoadTransformBuffer;              //!< Buffer with the scene load transform required by each skeletal mesh updated, as a set of float values, to be used
	vectorUVec2                         m_vectorDispatchSize;                    //!< Vector with the dispatch size for each skeletal mesh to process
	Buffer*                             m_vertexBufferSkinnedMeshBuffer;         //!< Pointer to the buffer with all the skeletal mesh updated information
	AccelerationStructure*              m_raytracedaccelerationstructure;        //!< Pointer to the raytracedaccelerationstructure acceleration stucture to update the BLAS nodes with skeletal mesh
	AccelerationStructure*              m_dynamicraytracedaccelerationstructure; //!< Pointer to the dynamicraytracedaccelerationstructure acceleration stucture to update the BLAS nodes with skeletal mesh
	bool                                m_cachedAccelerationStructures;          //!< Flag to know whether the two acceleration structures needed to update skeletal mesh elements which are done later in the pipeline have been cached
};

/////////////////////////////////////////////////////////////////////////////////////////////

#endif _SKELETALANIMATIONUPDATETECHNIQUE_H_
