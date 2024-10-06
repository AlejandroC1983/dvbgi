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

#ifndef _IRRADIANCEGAUSSIANBLURTECHNIQUE_H_
#define _IRRADIANCEGAUSSIANBLURTECHNIQUE_H_

// GLOBAL INCLUDES
#include "../../external/nano-signal-slot/nano_signal_slot.hpp"

// PROJECT INCLUDES
#include "../../include/rastertechnique/bufferprocesstechnique.h"

// CLASS FORWARDING
class BufferPrefixSumTechnique;
class Texture;
class ProcessCameraVisibleResultsTechnique;
class DynamicLightBounceVoxelIrradianceTechnique;
class MaterialLightBounceDynamicCopyIrradiance;
class MaterialLightBounceVoxel2DFilter;
class ResetLitvoxelData;

// NAMESPACE

// DEFINES
// TODO: unify in a single define in common .h file and remove all duplicates
typedef Nano::Signal<void()> SignalIrradianceGaussianBlurCompletion;

/////////////////////////////////////////////////////////////////////////////////////////////

class IrradianceGaussianBlurTechnique: public BufferProcessTechnique
{
	DECLARE_FRIEND_REGISTERER(IrradianceGaussianBlurTechnique)

protected:
	/** Parameter constructor
	* @param name      [in] technique's name
	* @param className [in] class name
	* @return nothing */
	IrradianceGaussianBlurTechnique(string &&name, string&& className);

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

	REF(SignalIrradianceGaussianBlurCompletion, m_signalIrradianceGaussianBlurCompletion, SignalIrradianceGaussianBlurCompletion)

protected:
	/** Slot to receive signal when the prefix sum step has been done
	* @return nothing */
	void slotPrefixSumComplete();

	/** Slot to receive notification when the DynamicLightBounceVoxelIrradianceTechnique has completed
	* @return nothing */
	void slotDynamicLightBounceVoxelIrradianceCompleted();

	SignalIrradianceGaussianBlurCompletion      m_signalIrradianceGaussianBlurCompletion;     //!< Signal for completion of the technique
	BufferPrefixSumTechnique*                   m_techniquePrefixSum;                         //!< Pointer to the instance of the prefix sum technique
	bool                                        m_prefixSumCompleted;                         //!< Flag to know if the prefix sum step has completed
	uint                                        m_cameraVisibleVoxelNumber;                   //!< Number of visible voxel determined by the CameraVisibleVoxelTechnique technique
	ProcessCameraVisibleResultsTechnique*       m_processCameraVisibleResultsTechnique;       //!< Pointer to the instance of the process camera visible results technique
	DynamicLightBounceVoxelIrradianceTechnique* m_dynamicLightBounceVoxelIrradianceTechnique; //!< Poiner to the instance of the dynamic light bounce voxel irradiance technique
	Buffer*                                     m_cameraVisibleDynamicVoxelIrradianceBuffer;  //!< Buffer to store the irradiance per voxel face of each of the camera dynamic visible voxels
	MaterialLightBounceDynamicCopyIrradiance*   m_materialLightBounceDynamicCopyIrradiance;   //!< Pointer to the instance of the light bounce dynamic copy irradiance material
	vectorTexturePtr                            m_vectorTexture;                              //!< Vector with a pointer to all the dynamic voxel irradiance 3D textures
	MaterialLightBounceVoxel2DFilter*           m_materialLightBounceVoxel2DFilter;           //!< Pointer to the instance of the light bounce voxel 2D filter material used to filter the dynamic voxels
	Buffer*                                     m_voxel2dFilterDebugBuffer;                   //!< Buffer for debug purposes
	Buffer*                                     m_dynamicCopyIrradianceDebugBuffer;           //!< Buffer for debug purposes
	ResetLitvoxelData*                          m_resetLitvoxelData;                          //!< Poiner to the reeset lit voxel data technique
	Buffer*                                     m_staticVoxelPaddingDebugBuffer;              //!< Buffer for debug purposes
};

/////////////////////////////////////////////////////////////////////////////////////////////

#endif _IRRADIANCEGAUSSIANBLURTECHNIQUE_H_
