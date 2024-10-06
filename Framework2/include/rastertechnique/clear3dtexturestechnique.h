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

#ifndef _CLEAR3DTEXTURESTECHNIQUE_H_
#define _CLEAR3DTEXTURESTECHNIQUE_H_

// GLOBAL INCLUDES
#include "../../external/nano-signal-slot/nano_signal_slot.hpp"

// PROJECT INCLUDES
#include "../../include/rastertechnique/rastertechnique.h"

// CLASS FORWARDING
class RenderPass;
class Framebuffer;
class Texture;
class DynamicVoxelVisibilityRayTracingTechnique;

// NAMESPACE

// DEFINES
typedef Nano::Signal<void()> SignalClear3DTexturesCompletion;

/////////////////////////////////////////////////////////////////////////////////////////////

class Clear3DTexturesTechnique: public RasterTechnique
{
	DECLARE_FRIEND_REGISTERER(Clear3DTexturesTechnique)

protected:
	/** Parameter constructor
	* @param name      [in] technique's name
	* @param className [in] class name
	* @return nothing */
	Clear3DTexturesTechnique(string &&name, string&& className);

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

	REF(SignalClear3DTexturesCompletion, m_signalClear3DTexturesCompletion, SignalClear3DTexturesCompletion)

protected:
	/** Slot to receive notification when the DynamicVoxelVisibilityRayTracingTechnique has completed
	* @return nothing */
	void slotDynamicVoxelVisibilityRayTracingCompleted();

	SignalClear3DTexturesCompletion            m_signalClear3DTexturesCompletion;           //!< Signal for completion of the technique
	RenderPass*                                m_clear3DTexturesRenderPass;                 //!< Render pass used for clearing the 3D texture used for dynamic scene voxelization
	Framebuffer*                               m_clear3DTexturesFramebufferStatic;          //!< Framebuffer used for clearing the static 3D irradiance textures
	Framebuffer*                               m_clear3DTexturesFramebufferDynamic;         //!< Framebuffer used for clearing the dynamic 3D irradiance textures
	DynamicVoxelVisibilityRayTracingTechnique* m_dynamicVoxelVisibilityRayTracingTechnique; //!< Pointer to the instance of the dynamic voxel visibility ray tracing technique
	int                                        m_sceneVoxelizationResolution;               //!< Voxelization size, value of raster flag value SCENE_VOXELIZATION_RESOLUTION
	vectorTexturePtr                           m_vectorTextureStatic;                       //!< Vector with a pointer to all the static voxel irradiance 3D textures
	vectorTexturePtr                           m_vectorTextureDynamic;                      //!< Vector with a pointer to all the dynamic voxel irradiance 3D textures
};

/////////////////////////////////////////////////////////////////////////////////////////////

#endif _CLEAR3DTEXTURESTECHNIQUE_H_
