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

#ifndef _STATICSCENEVOXELIZATIONTECHNIQUE_H_
#define _STATICSCENEVOXELIZATIONTECHNIQUE_H_

// GLOBAL INCLUDES
#include "../../external/nano-signal-slot/nano_signal_slot.hpp"

// PROJECT INCLUDES
#include "../../include/rastertechnique/scenevoxelizationtechnique.h"

// CLASS FORWARDING
class MaterialSceneVoxelization;
class RenderPass;
class Framebuffer;
class Texture;
class Buffer;

// NAMESPACE

// DEFINES
typedef Nano::Signal<void()> SignalStaticVoxelizationComplete;

/////////////////////////////////////////////////////////////////////////////////////////////

class StaticSceneVoxelizationTechnique: public SceneVoxelizationTechnique
{
	DECLARE_FRIEND_REGISTERER(StaticSceneVoxelizationTechnique)

protected:
	/** Parameter constructor
	* @param name      [in] technique's name
	* @param className [in] class name
	* @return nothing */
	StaticSceneVoxelizationTechnique(string &&name, string&& className);

	/** Default destructor
	* @return nothing */
	virtual ~StaticSceneVoxelizationTechnique();

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

	REF(SignalStaticVoxelizationComplete, m_staticVoxelizationComplete, StaticVoxelizationComplete)
		
protected:
	Texture*                         m_voxelizationReflectanceTexture;    //!< 3D texture with the reflectance values
	Texture*                         m_staticVoxelIndexTexture;           //!< 3D texture to map, for a specifi 3D voxel coordinates, where in the voxelHashedPositionCompactedBuffer the voxel is located
	Buffer*                          m_voxelOccupiedBuffer;               //!< Shader storage buffer to know whether a voxel position is occupied or is not
	Buffer*                          m_voxelFirstIndexBuffer;             //!< Shader storage buffer to know the index of the first fragment generated for the voxelization 3D volume of coordinates hashed
	RenderPass*                      m_clearVoxelIndexTextureRenderPass;  //!< Render pass used for clearing the staticVoxelIndexTexture 3D texture
	Framebuffer*                     m_clearVoxelIndexTextureFramebuffer; //!< Framebuffer used for clearing the staticVoxelIndexTexture 3D texture
	SignalStaticVoxelizationComplete m_staticVoxelizationComplete;        //!< Signal to notify when the voxelization step is complete
};

/////////////////////////////////////////////////////////////////////////////////////////////

#endif _STATICSCENEVOXELIZATIONTECHNIQUE_H_
