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

#ifndef _SCENELIGHTINGTECHNIQUE_H_
#define _SCENELIGHTINGTECHNIQUE_H_

// GLOBAL INCLUDES

// PROJECT INCLUDES
#include "../../include/rastertechnique/rastertechnique.h"

// CLASS FORWARDING
class Texture;
class LitVoxelTechnique;
class RenderPass;
class Framebuffer;

// NAMESPACE

// DEFINES

/////////////////////////////////////////////////////////////////////////////////////////////

class SceneLightingTechnique: public RasterTechnique
{
	DECLARE_FRIEND_REGISTERER(SceneLightingTechnique)

protected:
	/** Parameter constructor
	* @param name      [in] technique's name
	* @param className [in] class name
	* @return nothing */
	SceneLightingTechnique(string &&name, string&& className);

	/** Default destructor
	* @return nothing */
	virtual ~SceneLightingTechnique();

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

protected:
	/** Slot for the keyboard signal when pressing the L key to switch between lighting and debug rasterization
	* @return nothing */
	void slotLKeyPressed();

	/** For each instance of MaterialColorTexture, an instance of MaterialLighting is generated, with the suffix
	* "_Lighting", so it can be recovered later when rasterizing the scene elements. The method is called when the scene
	* has been already loaded and the material instances of MaterialColorTexture for the scene elements generated
	* @return nothing */
	void generateLightingMaterials();

	/** Slot for the keyboard signal when pressing the 1 key to remove 100 units from the MaterialLighting::m_irradianceMultiplier value
	* for each MaterialLighting in m_vectorMaterial
	* @return nothing */
	void slot1KeyPressed();

	/** Slot for the keyboard signal when pressing the 2 key to add 100 units from the MaterialLighting::m_irradianceMultiplier value
	* for each MaterialLighting in m_vectorMaterial
	* @return nothing */
	void slot2KeyPressed();

	/** Slot for the keyboard signal when pressing the 7 key to remove 100 units from the MaterialLighting::m_directIrradianceMultiplier value
	* for each MaterialLighting in m_vectorMaterial
	* @return nothing */
	void slot7KeyPressed();

	/** Slot for the keyboard signal when pressing the 8 key to add 100 units from the MaterialLighting::m_directIrradianceMultiplier value
	* for each MaterialLighting in m_vectorMaterial
	* @return nothing */
	void slot8KeyPressed();

	/** Perform the commands needed to draw indirect the node vector given as parameter
	* @param commandBuffer [in] command buffer to record to
	* @param vectorNode    [in] vector of nodes to draw indirect
	* @return nothing */
	void drawIndirectNodeVector(VkCommandBuffer commandBuffer, const vectorNodePtr& vectorNode);

	static string        m_lightingMaterialSuffix;          //!< Suffix added to the material names used in the voxelization step
	LitVoxelTechnique*   m_litVoxelTechnique;               //!< Pointer to the lit voxel technique
	Texture*             m_renderTargetColor;               //!< Render target used for color
	Texture*             m_renderTargetDepth;               //!< Render target used for depth
	RenderPass*          m_renderPass;                      //!< Render pass used
	Framebuffer*         m_framebuffer;                     //!< Framebuffer used
	Buffer*              m_indirectCommandBufferMainCamera; //!< Pointer to the indirect command buffer for the main camera
	vectorNodePtr        m_arrayNode;                       //!< Vector with pointers to the scene nodes with component type GRT_RENDERCOMPONENT
	vectorNodePtr        m_arrayNodeSkinnedMesh;            //!< Vector with pointers to the scene nodes with component type GRT_SKINNEDMESHRENDERCOMPONENT
	vectorTexturePtr     m_vectorTexture;                   //!< Vector with a pointer to all the static and dynamic voxel irradiance 3D textures
};

/////////////////////////////////////////////////////////////////////////////////////////////

#endif _SCENELIGHTINGTECHNIQUE_H_
