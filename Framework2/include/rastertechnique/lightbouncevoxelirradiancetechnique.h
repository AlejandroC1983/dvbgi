/*
Copyright 2018 Alejandro Cosin

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

#ifndef _LIGTHBOUNCEVOXELIRRADIANCETECHNIQUE_H_
#define _LIGTHBOUNCEVOXELIRRADIANCETECHNIQUE_H_

// GLOBAL INCLUDES
#include "../../external/nano-signal-slot/nano_signal_slot.hpp"

// PROJECT INCLUDES
#include "../../include/rastertechnique/bufferprocesstechnique.h"

// CLASS FORWARDING
class Buffer;
class BufferPrefixSumTechnique;
class LitVoxelTechnique;
class ResetLitvoxelData;
class Camera;
class ProcessCameraVisibleResultsTechnique;
class Texture;
class DynamicVoxelVisibilityRayTracingTechnique;
class MaterialLightBounceStaticVoxelPadding;
class MaterialLightBounceStaticVoxelFiltering;

// NAMESPACE

// DEFINES
// TODO: unify in a single define in common .h file and remove all duplicates
typedef Nano::Signal<void()> SignalLightBounceVoxelIrradianceCompletion;

/////////////////////////////////////////////////////////////////////////////////////////////

class LightBounceVoxelIrradianceTechnique: public BufferProcessTechnique
{
	DECLARE_FRIEND_REGISTERER(LightBounceVoxelIrradianceTechnique)

protected:
	/** Parameter constructor
	* @param name      [in] technique's name
	* @param className [in] class name
	* @return nothing */
	LightBounceVoxelIrradianceTechnique(string &&name, string&& className);

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

	REF(SignalLightBounceVoxelIrradianceCompletion, m_signalLightBounceVoxelIrradianceCompletion, SignalLightBounceVoxelIrradianceCompletion)

protected:
	/** Slot to receive signal when the prefix sum step has been done
	* @return nothing */
	void slotPrefixSumComplete();

	/** Slot to receive notification when the DynamicVoxelVisibilityRayTracingTechnique has completed
	* @return nothing */
	void slotDynamicVoxelVisibilityRayTracingCompleted();

	/** Slot for the keyboard signal when pressing the 3 key to remove 100 units from MaterialLightBounceVoxelIrradiance::m_formFactorVoxelToVoxelAdded
	* @return nothing */
	void slot3KeyPressed();

	/** Slot for the keyboard signal when pressing the 4 key to add 100 units from MaterialLightBounceVoxelIrradiance::m_formFactorVoxelToVoxelAdded
	* @return nothing */
	void slot4KeyPressed();

	/** Build the buffer with the per voxel face directions (128) to rebuilf the position of the ray intersected.
	* @return nothing */
	void buildRayDirectionBuffer();

	/** Build the buffer with the triangulated indices information
	* @return nothing */
	void buildTriangulatedIndicesBuffer();

	SignalLightBounceVoxelIrradianceCompletion m_signalLightBounceVoxelIrradianceCompletion; //!< Signal for completion of the technique
	BufferPrefixSumTechnique*                  m_techniquePrefixSum;                         //!< Pointer to the instance of the prefix sum technique
	LitVoxelTechnique*                         m_litVoxelTechnique;                          //!< Pointer to the lit voxel technique
	ResetLitvoxelData*                         m_resetLitvoxelData;                          //!< Poiner to the reeset lit voxel data technique
	Buffer*                                    m_lightBounceVoxelIrradianceBuffer;           //!< Pointer to the lightBounceVoxelIrradianceBuffer buffer
	Buffer*                                    m_lightBounceVoxelDebugBuffer;                //!< Pointer to debug buffer
	Buffer*                                    m_lightBounceIndirectLitCounterBuffer;        //!< Pointer to buffer used as an atomic counter tho count the number of voxels which received an amount of irradiance greater than zero
	Buffer*                                    m_debugCounterBuffer;                         //!< Pointer to buffer used as an atomic counter for debug purposes
	vec4                                       m_sceneMin;                                   //!< Minimum value of the scene's aabb
	vec4                                       m_sceneExtent;                                //!< Scene extent
	uint                                       m_numOccupiedVoxel;                           //!< Number of occupied voxels after voxelization process
	bool                                       m_prefixSumCompleted;                         //!< Flag to know if the prefix sum step has completed
	Camera*                                    m_mainCamera;                                 //!< Scene main camera
	uint                                       m_cameraVisibleVoxelNumber;                   //!< Number of visible voxel determined by the CameraVisibleVoxelTechnique technique
	uint                                       m_lightBounceIndirectLitCounter;              //!< Helper variable to take the value from m_lightBounceIndirectLitCounterBuffer
	Buffer*                                    m_lightBounceVoxelGaussianFilterDebugBuffer;  //!< Buffer for debug purposes
	ProcessCameraVisibleResultsTechnique*      m_processCameraVisibleResultsTechnique;       //!< Pointer to the instance of the process camera visible results technique
	DynamicVoxelVisibilityRayTracingTechnique* m_dynamicVoxelVisibilityRayTracingTechnique;  //!< Pointer to the instance of the dynamic voxel visibility ray tracing technique
	MaterialLightBounceStaticVoxelPadding*     m_materialLightBounceStaticVoxelPadding;      //!< Pointer to the instance of the MaterialLightBounceStaticVoxelPadding material used to geenrate the padding around those static voxels which are occupied
	vectorTexturePtr                           m_vectorTexture;                              //!< Vector with a pointer to all the static and dynamic voxel irradiance 3D textures
	MaterialLightBounceStaticVoxelFiltering*   m_materialLightBounceStaticVoxelFiltering;    //!< Pointer to the instance of the MaterialLightBounceStaticVoxelFiltering material used to apply filtering to the static voxels
	Buffer*                                    m_staticVoxelFilteringDebugBuffer;            //!< Buffer for debug purposes
	Buffer*                                    m_rayDirectionBuffer;                         //!< Pointer to buffer with the per voxel face directions (128) to rebuilf the position of the ray intersected.
	Buffer*                                    m_triangulatedIndicesBuffer;                  //!< Pointer to buffer with the per triagle triangulated indices information used in the light bounce step to gather extra samples of lit voxels for surfaces almost planar in voxel coordinates
};

/////////////////////////////////////////////////////////////////////////////////////////////

#endif _LIGTHBOUNCEIRRADIANCEFIELDTECHNIQUE_H_
