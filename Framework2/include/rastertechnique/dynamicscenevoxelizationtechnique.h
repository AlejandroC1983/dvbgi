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

#ifndef _DYNAMICSCENEVOXELIZATIONTECHNIQUE_H_
#define _DYNAMICSCENEVOXELIZATIONTECHNIQUE_H_

// GLOBAL INCLUDES
#include "../../external/nano-signal-slot/nano_signal_slot.hpp"

// PROJECT INCLUDES
#include "../../include/rastertechnique/scenevoxelizationtechnique.h"

// CLASS FORWARDING
class MaterialSceneVoxelization;
class RenderPass;
class Framebuffer;
class Texture;
class ResetLitvoxelData;
class Buffer;

// NAMESPACE

// DEFINES
typedef Nano::Signal<void()> SignalDynamicVoxelizationComplete;

/////////////////////////////////////////////////////////////////////////////////////////////

class DynamicSceneVoxelizationTechnique: public SceneVoxelizationTechnique
{
	DECLARE_FRIEND_REGISTERER(DynamicSceneVoxelizationTechnique)

protected:
	/** Parameter constructor
	* @param name      [in] technique's name
	* @param className [in] class name
	* @return nothing */
	DynamicSceneVoxelizationTechnique(string &&name, string&& className);

	/** Default destructor
	* @return nothing */
	virtual ~DynamicSceneVoxelizationTechnique();

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

	REF(SignalDynamicVoxelizationComplete, m_dynamicVoxelizationComplete, DynamicVoxelizationComplete)
		
protected:
	/** For each instance of MaterialColorTexture, an instance of MaterialSceneVoxelization is generated, with the suffix
	* "_Voxelization", so it can be recovered later when rasterizing the scene elements. The method is called when the scene
	* has been already loaded and the material instances of MaterialColorTexture for the scene elements generated
	* @return nothing */
	void generateVoxelizationMaterials();

	/** Slot to receive notification when the LitVoxelTechnique technique has been completed
	* @return nothing */
	void slotResetLitVoxelComplete();

	SignalDynamicVoxelizationComplete m_dynamicVoxelizationComplete;           //!< Signal to notify when the voxelization step is complete
	Texture*                          m_dynamicVoxelizationReflectanceTexture; //!< 3D texture with the reflectance values for the dynamic voxelization technique
	RenderPass*                       m_dynamicTextureClearRenderPass;         //!< Render pass used for clearing the 3D texture used for dynamic scene voxelization
	Framebuffer*                      m_dynamicTextureClearFramebuffer;        //!< Framebuffer used for clearing the 3D texture used for dynamic scene voxelization
	ResetLitvoxelData*                m_resetLitvoxelData;                     //!< Pointer to the instance of the reset light data technique
	Buffer*                           m_dynamicVoxelizationDebugBuffer;        //!< Buffer for debug purposes
	int                               m_sceneVoxelizationResolution;           //!< Voxelization size, value of raster flag value SCENE_VOXELIZATION_RESOLUTION
};

/////////////////////////////////////////////////////////////////////////////////////////////

#endif _DYNAMICSCENEVOXELIZATIONTECHNIQUE_H_
