/*
Copyright 2023 Alejandro Cosin

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

#ifndef _GBUFFERTECHNIQUE_H_
#define _GBUFFERTECHNIQUE_H_

// GLOBAL INCLUDES
#include "../../external/nano-signal-slot/nano_signal_slot.hpp"

// PROJECT INCLUDES
#include "../../include/rastertechnique/rastertechnique.h"

// CLASS FORWARDING
class RenderPass;
class Framebuffer;
class Buffer;
class Texture;
class MaterialIndirectColorTexture;

// NAMESPACE

// DEFINES
typedef Nano::Signal<void()> SignalGBuffer;

/////////////////////////////////////////////////////////////////////////////////////////////

class GBufferTechnique: public RasterTechnique
{
	DECLARE_FRIEND_REGISTERER(GBufferTechnique)

protected:
	/** Parameter constructor
	* @param name      [in] technique's name
	* @param className [in] class name
	* @return nothing */
	GBufferTechnique(string &&name, string&& className);

	/** Default destructor
	* @return nothing */
	virtual ~GBufferTechnique();

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

	/** Perform the commands needed to draw indirect the node vector given as parameter
	* @param commandBuffer [in] command buffer to record to
	* @param vectorNode    [in] vector of nodes to draw indirect
	* @return nothing */
	void drawIndirectNodeVector(VkCommandBuffer commandBuffer, const vectorNodePtr& vectorNode);

protected:
	/** For each instance of MaterialColorTexture, an instance of MaterialGBuffer is generated, with the suffix
	* "_GBuffer", so it can be recovered later when rasterizing the scene elements
	* @return nothing */
	void generateGBufferMaterials();

	SignalGBuffer                 m_signalGBuffer;                   //!< Signal for completion of the technique
	RenderPass*                   m_renderPass;                      //!< Render pass used
	Framebuffer*                  m_framebuffer;                     //!< Framebuffer used
	Buffer*                       m_indirectCommandBufferMainCamera; //!< Pointer to the indirect command buffer for the main camera
	Texture*                      m_GBufferNormal;                   //!< Texture with the normal information
	Texture*                      m_GBufferReflectance;              //!< Texture with the reflectance information
	Texture*                      m_GBufferPosition;                 //!< Texture with the position information
	Texture*                      m_GBufferDepth;                    //!< Depth texture
	uint                          m_offscreenWidth;                  //!< Width of the offscreen texture used to store the ray tracing render pass results
	uint                          m_offscreenHeight;                 //!< Height of the offscreen texture used to store the ray tracing render pass resultsç
	vector<VkClearValue>          m_vectorGBufferClearValue;         //!< Vector with clear values for the GBuffer attachments
	vectorNodePtr                 m_arrayNode;                       //!< Vector with pointers to the scene nodes with component type GRT_RENDERCOMPONENT
	vectorNodePtr                 m_arrayNodeSkinnedMesh;            //!< Vector with pointers to the scene nodes with component type GRT_SKINNEDMESHRENDERCOMPONENT
	MaterialIndirectColorTexture* m_material;                        //!< Material used in this technique
	static string                 m_GBufferMaterialSuffix;           //!< Suffix added to the material names for the deferred pass part
};

/////////////////////////////////////////////////////////////////////////////////////////////

#endif _GBUFFERTECHNIQUE_H_
