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

#ifndef _VOXELRASTERINSCENARIOTECHNIQUE_H_
#define _VOXELRASTERINSCENARIOTECHNIQUE_H_

// GLOBAL INCLUDES

// PROJECT INCLUDES
#include "../../include/rastertechnique/rastertechnique.h"

// CLASS FORWARDING
class MaterialVoxelRasterInScenario;
class Buffer;
class Texture;
class RenderPass;
class Framebuffer;

// NAMESPACE

// DEFINES

/////////////////////////////////////////////////////////////////////////////////////////////

class VoxelRasterInScenarioTechnique : public RasterTechnique
{
	DECLARE_FRIEND_REGISTERER(VoxelRasterInScenarioTechnique)

protected:
	/** Parameter constructor
	* @param name      [in] technique's name
	* @param className [in] class name
	* @return nothing */
	VoxelRasterInScenarioTechnique(string &&name, string&& className);

	/** Default destructor
	* @return nothing */
	virtual ~VoxelRasterInScenarioTechnique();

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
	/** Slot to receive notification when the prefix sum process is completed
	* @return nothing */
	void slotPrefixSumCompleted();

	/** Slot for the keyboard signal when pressing the R key to switch between voxel reflectance data and 
	* debug rasterization
	* @return nothing */
	void slotRKeyPressed();

	/** Slot for the keyboard signal when pressing the N key to switch between voxel normal data and
	* debug rasterization
	* @return nothing */
	void slotNKeyPressed();

	/** Slot for the keyboard signal when pressing the V key to switch between voxel lit data and
	* debug rasterization
	* @return nothing */
	void slotVKeyPressed();

	/** Slot for the keyboard signal when pressing the L key to switch between lighting and debug rasterization
	* @return nothing */
	void slotLKeyPressed();

	/** Slot for the keyboard signal when pressing the Y key to switch between lighting and fragment density
	* @return nothing */
	void slotYKeyPressed();

	MaterialVoxelRasterInScenario* m_material;                         //!< Voxel raster in scenario material
	uint                           m_numOccupiedVoxel;                 //!< Number of occupied voxels after voxelization process
	Buffer*                        m_voxelrasterinscenariodebugbuffer; //!< Pointer to debug buffer
	Texture*                       m_renderTargetColor;                //!< Render target used for color
	Texture*                       m_renderTargetDepth;                //!< Render target used for depth
	RenderPass*                    m_renderPass;                       //!< Render pass used
	Framebuffer*                   m_framebuffer;                      //!< Framebuffer used
};

/////////////////////////////////////////////////////////////////////////////////////////////

#endif _VOXELRASTERINSCENARIOTECHNIQUE_H_
