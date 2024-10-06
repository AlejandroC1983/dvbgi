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

#ifndef _DISTANCESHADOWMAPPINGTECHNIQUE_H_
#define _DISTANCESHADOWMAPPINGTECHNIQUE_H_

// GLOBAL INCLUDES
#include "../../external/nano-signal-slot/nano_signal_slot.hpp"

// PROJECT INCLUDES
#include "../../include/rastertechnique/rastertechnique.h"

// CLASS FORWARDING
class RenderPass;
class Framebuffer;
class MaterialDistanceShadowMapping;
class Camera;
class Texture;

// NAMESPACE

// DEFINES
typedef Nano::Signal<void()> SignalDistanceShadowMapCompletion;

/////////////////////////////////////////////////////////////////////////////////////////////

class DistanceShadowMappingTechnique : public RasterTechnique
{
	DECLARE_FRIEND_REGISTERER(DistanceShadowMappingTechnique)

protected:
	/** Parameter constructor
	* @param name      [in] technique's name
	* @param className [in] class name
	* @return nothing */
	DistanceShadowMappingTechnique(string &&name, string&& className);

	/** Default destructor
	* @return nothing */
	virtual ~DistanceShadowMappingTechnique();

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

	GET_PTR(Camera, m_camera, Camera)
	GETCOPY(float, m_emitterRadiance, EmitterRadiance)
	GET_PTR(Texture, m_distanceShadowMappingTexture, DistanceShadowMappingTexture)

protected:
	RenderPass*                    m_renderPass;                    //!< Render pass used for directional voxel shadow mapping technique
	Framebuffer*                   m_framebuffer;                   //!< Framebuffer used for directional voxel shadow mapping technique
	MaterialDistanceShadowMapping* m_material;                      //!< Material for the distance shadow mapping technique
	Camera*                        m_camera;                        //!< Camera used by the technique
	Texture*                       m_distanceShadowMappingTexture;  //!< Distance shadow mapping texture name
	int                            m_shadowMapWidth;                //!< Voxel shadow map texture width
	int                            m_shadowMapHeight;               //!< Voxel shadow map texture height
	Texture*                       m_offscreenDistanceDepthTexture; //!< Offscreen distance depth texture used together with the color attachment
	float                          m_emitterRadiance;               //!< Radiance of the emitter this shadow map represents
};

/////////////////////////////////////////////////////////////////////////////////////////////

#endif _DISTANCESHADOWMAPPINGTECHNIQUE_H_
