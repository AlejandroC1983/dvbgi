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

#ifndef _LITVOXELTECHNIQUE_H_
#define _LITVOXELTECHNIQUE_H_

// GLOBAL INCLUDES
#include "../../external/nano-signal-slot/nano_signal_slot.hpp"

// PROJECT INCLUDES
#include "../../include/rastertechnique/bufferprocesstechnique.h"

// CLASS FORWARDING
class Buffer;
class Camera;
class DistanceShadowMappingTechnique;
class BufferPrefixSumTechnique;
class LitVoxelTechnique;
class MaterialLitVoxel;
class MaterialNeighbourLitVoxelInformation;

// NAMESPACE

// DEFINES
typedef Nano::Signal<void()> SignalLitVoxelCompletion;
#define NUM_ADDUP_ELEMENT_PER_THREAD 25

/////////////////////////////////////////////////////////////////////////////////////////////

class LitVoxelTechnique : public BufferProcessTechnique
{
	DECLARE_FRIEND_REGISTERER(LitVoxelTechnique)

protected:
	/** Parameter constructor
	* @param name      [in] technique's name
	* @param className [in] class name
	* @return nothing */
	LitVoxelTechnique(string &&name, string&& className);

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

	/** Slot to receive notification when the camera values (look at / position) are dirty
	* @return nothing */
	void slotCameraDirty();

	/** Slot to receive notification when the scene is dirty
	* @return nothing */
	void slotSceneDirty();

	/** Function called from slotCameraDirty and slotSceneDirty
	* @param needsToRecord [in] Flag to know if the technique needs to record again
	* @return nothing */
	//void slotDirtyUpdateValues(bool needsToRecord);
	void slotDirtyUpdateValues();

	REF(SignalLitVoxelCompletion, m_signalLitVoxelCompletion, SignalLitVoxelCompletion)
	GETCOPY(vec3, m_cameraPosition, CameraPosition)
	GETCOPY(vec3, m_cameraForward, CameraForward)
	GETCOPY(float, m_emitterRadiance, EmitterRadiance)
	//SET(bool, m_techniqueLock, TechniqueLock)

protected:
	/** Slot to receive notification when the prefix sum of the scene voxelization has been completed
	* @return nothing */
	void slotPrefixSumComplete();
	
	SignalLitVoxelCompletion              m_signalLitVoxelCompletion;             //!< Signal for lit voxel completion
	Buffer*                               m_litVoxelDebugBuffer;                  //!< Buffer for debug purposes
	DistanceShadowMappingTechnique*       m_distanceShadowMappingTechnique;       //!< Pointer to the distance shadow mapping technique
	// TODO: Remove													     
	LitVoxelTechnique*                    m_litVoxelTechnique;                    //!< Pointer to the lit voxel technique
	BufferPrefixSumTechnique*             m_bufferPrefixSumTechnique;             //!< Pointer to the prefix sum technique
	Camera*                               m_voxelShadowMappingCamera;             //!< Camera used for voxel shadow mapping
	bool                                  m_prefixSumCompleted;                   //!< Flag to know if the prefix sum step has completed
	float                                 m_emitterRadiance;                      //!< Emitter radiance in the moment the slot reset radiance data is signaled, to store the original camera positon in the moment all the simulation starts (it takes several frames and the camera poition and forward direction can change in the meantime
	vec3                                  m_cameraPosition;                       //!< Camera position in the moment the slot reset radiance data is signaled, to store the original camera positon in the moment all the simulation starts (it takes several frames and the camera poition and forward direction can change in the meantime
	vec3                                  m_cameraForward;                        //!< Camera forward direction in the moment the slot reset radiance data is signaled, to store the original camera positon in the moment all the simulation starts (it takes several frames and the camera poition and forward direction can change in the meantime
	uvec3                                 m_accumulatedIrradiance;                //!< Member variable to store accumulated irradiance values from m_accumulatedRadianceBuffer
	MaterialLitVoxel*                     m_materialLitVoxel;                     //!< Pointer to the instance of the lit voxel material
	vec4                                  m_sceneExtentAndVoxelSize;              //!< Extent of the scene in the xyz coordinates, voxelization texture size in the w coordinate
	vec4                                  m_sceneMin;						      //!< Scene min 3D coordinates
	uint                                  m_dynamicVoxelCounter;                  //!< Copy from DynamicVoxelCopyToBufferTechnique::m_dynamicVoxelCounter
	uint                                  m_dynamicVoxelDispatchXDimension;       //!< Dispatch x dimensions for the lit voxel test dispatch for dynamic voxels
	uint                                  m_dynamicVoxelDispatchYDimension;       //!< Dispatch y dimensions for the lit voxel test dispatch for dynamic voxels
	int                                   m_voxelizationWidth;                    //!< Copy of StaticSceneVoxelizationTechnique::m_voxelizedSceneWidth
	bool                                  m_sceneDirty;                           //!< Flag to know if the scene has notified as dirty
	MaterialNeighbourLitVoxelInformation* m_materialNeighbourLitVoxelInformation; //!< Pointer to the instance of the material used to store information about what neighbour voxels are lit and the voxel itself
	Buffer*                               m_neighbourLitVoxelDebugBuffer;         //!< Buffer for debug purposes
	Buffer*                               m_neighbourLitVoxelInformationBuffer;   //!< Buffer with the information about what neighbour voxels are lit and the voxel itself
};

/////////////////////////////////////////////////////////////////////////////////////////////

#endif _LITVOXELTECHNIQUE_H_
