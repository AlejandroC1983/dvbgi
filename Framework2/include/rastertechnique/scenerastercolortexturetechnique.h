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

#ifndef _SCENERASTERCOLORTEXTURETECHNIQUE_H_
#define _SCENERASTERCOLORTEXTURETECHNIQUE_H_

// GLOBAL INCLUDES

// PROJECT INCLUDES
#include "../../include/rastertechnique/rastertechnique.h"

// CLASS FORWARDING
class MaterialColorTexture;
class Texture;
class RenderPass;
class Framebuffer;

// NAMESPACE

// DEFINES

/////////////////////////////////////////////////////////////////////////////////////////////

class SceneRasterColorTextureTechnique: public RasterTechnique
{
	DECLARE_FRIEND_REGISTERER(SceneRasterColorTextureTechnique)

protected:
	/** Parameter constructor
	* @param name      [in] technique's name
	* @param className [in] class name
	* @return nothing */
	SceneRasterColorTextureTechnique(string &&name, string&& className);

	/** Default destructor
	* @return nothing */
	virtual ~SceneRasterColorTextureTechnique();

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

protected:
	/** Slot for the keyboard signal when pressing the L key to switch between lighting and debug rasterization
	* @return nothing */
	void slotLKeyPressed();

	/** Draw scene elements present in vectorNode parameter
	* @param vectorNode    [in] vector with the nodes to draw
	* @param commandBuffer [in] command buffer to record to
	* @return nothing */
	void drawSceneElement(vectorNodePtr& vectorNode, VkCommandBuffer commandBuffer);

	float                 m_elapsedTime;       //!< Elspased time since last frame
	bool                  m_addingTime;        //!< Logic flag value
	MaterialColorTexture* m_material;          //!< Material used in this technique
	Texture*              m_renderTargetColor; //!< Render target used for color
	Texture*              m_renderTargetDepth; //!< Render target used for depth
	RenderPass*           m_renderPass;        //!< Render pass used
	Framebuffer*          m_framebuffer;       //!< Framebuffer used
};

/////////////////////////////////////////////////////////////////////////////////////////////

#endif _SCENERASTERCOLORTEXTURETECHNIQUE_H_
