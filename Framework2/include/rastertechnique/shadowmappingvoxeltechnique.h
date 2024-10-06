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

#ifndef _SHADOWMAPPINGVOXELTECHNIQUE_H_
#define _SHADOWMAPPINGVOXELTECHNIQUE_H_

// GLOBAL INCLUDES

// PROJECT INCLUDES
#include "../../include/rastertechnique/rastertechnique.h"

// CLASS FORWARDING
class RenderPass;
class Framebuffer;
class MaterialShadowMappingVoxel;
class Camera;
class Texture;
class Buffer;

// NAMESPACE

// DEFINES

/////////////////////////////////////////////////////////////////////////////////////////////

class ShadowMappingVoxelTechnique : public RasterTechnique
{
	DECLARE_FRIEND_REGISTERER(ShadowMappingVoxelTechnique)

protected:
	/** Parameter constructor
	* @param name      [in] technique's name
	* @param className [in] class name
	* @return nothing */
	ShadowMappingVoxelTechnique(string &&name, string&& className);

	/** Default destructor
	* @return nothing */
	virtual ~ShadowMappingVoxelTechnique();

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

	GET_PTR(Texture, m_shadowMappingTexture, ShadowMappingTexture)

protected:
	/** Slot to receive notification when the prefix sum process is completed
	* @return nothing */
	void slotPrefixSumCompleted();

	/** Slot to receive notification when the camera values (look at / position) are dirty
	* @return nothing */
	void slotCameraDirty();

	string                      m_shadowMappingTextureName;  //!< Name for the shadow mapping texture
	string                      m_offscreenDepthTextureName; //!< Name for the offscreen depth texture
	string                      m_renderPassName;            //!< Name for the render pass name
	string                      m_framebufferName;           //!< Name for the framebuffer name
	string                      m_cameraName;                //!< Name for the camera used
	RenderPass*                 m_renderPass;                //!< Render pass used for directional voxel shadow mapping technique
	Framebuffer*                m_framebuffer;               //!< Framebuffer used for directional voxel shadow mapping technique
	MaterialShadowMappingVoxel* m_material;                  //!< Material for directional voxel shadow mapping
	Camera*                     m_shadowMappingCamera;       //!< Voxel shadow mapping camera, is the same as the one used in the shadow mapping technique
	Texture*                    m_shadowMappingTexture;      //!< Voxel shadow mapping texture
	int                         m_shadowMapWidth;            //!< Voxel shadow map texture width
	int                         m_shadowMapHeight;           //!< Voxel shadow map texture height
	uint                        m_numOccupiedVoxel;          //!< Number of occupied voxels after voxelization process
	bool                        m_prefixSumCompleted;        //!< Flag to know if the prefix sum step has completed
	bool                        m_newPassRequested;          //!< If true, new pass was requested to process the assigned buffer
	Texture*                    m_offscreenDepthTexture;     //!< Offscreen depth texture used together with the color attachment
};

/////////////////////////////////////////////////////////////////////////////////////////////

#endif _SHADOWMAPPINGVOXELTECHNIQUE_H_
