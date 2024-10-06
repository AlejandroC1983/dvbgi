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

#ifndef _SCENEVOXELIZATIONTECHNIQUE_H_
#define _SCENEVOXELIZATIONTECHNIQUE_H_

// GLOBAL INCLUDES
#include "../../external/nano-signal-slot/nano_signal_slot.hpp"

// PROJECT INCLUDES
#include "../../include/rastertechnique/rastertechnique.h"

// CLASS FORWARDING
class MaterialSceneVoxelization;
class RenderPass;
class Framebuffer;
class Texture;
class Buffer;

// NAMESPACE

// DEFINES
const uint maxValue = 4294967295;

/////////////////////////////////////////////////////////////////////////////////////////////

// Per fragment struct
struct PerFragmentData
{
	vec4 position;        // Fragment position in xyz fields, scene element indes in the w field
	vec4 normal;          // Fragment normal in xyz fields, emitted fragment counter in the w field
	uvec4 compressedData; // Fragment reflectance in x field, fragment accumulated irradiance in the y field
};

// Per voxel struct
// NOTE: using vec3 instead of vec4 seems to give some problems, maybe because of the memory layout
//       (a part of the buffer would not be available at the last set of indices)
struct PerVoxelAxisIrradiance
{
	vec4 axisIrradiance[6]; //!< The irradiance of each axis is set in the array following the order -x, +x, -y, +y, -z, +z
};

/////////////////////////////////////////////////////////////////////////////////////////////

class SceneVoxelizationTechnique: public RasterTechnique
{
	DECLARE_FRIEND_REGISTERER(SceneVoxelizationTechnique)

protected:
	/** Parameter constructor
	* @param name      [in] technique's name
	* @param className [in] class name
	* @return nothing */
	SceneVoxelizationTechnique(string &&name, string&& className);

	/** Default destructor
	* @return nothing */
	virtual ~SceneVoxelizationTechnique();

public:
	/** New shaders, textures, etc initialization should be done here, called when the tehnique is instantiated
	* @return nothing */
	virtual void init();

	/** Called before rendering
	* @param dt [in] elapsed time in miliseconds since the last update call
	* @return nothing */
	virtual void prepare(float dt);

	/** Called inside each technique's while record and submit loop, after the call to prepare and before the
	* technique record, to update any dirty material that needs to be rebuilt as a consequence of changes in the
	* resources used by the materials present in m_vectorMaterial
	* @return nothing */
	virtual void postCommandSubmit();

	/** Record command buffer
	* @param currentImage      [in] current screen image framebuffer drawing to (in case it's needed)
	* @param commandBufferID   [in] Unique identifier of the command buffer returned as parameter
	* @param commandBufferType [in] Queue to submit the command buffer recorded in the call
	* @return command buffer the technique has recorded to */
	virtual VkCommandBuffer* record(int currentImage, uint& commandBufferID, CommandBufferType& commandBufferType);

	GETCOPY(int, m_voxelizedSceneWidth, VoxelizedSceneWidth)
	GETCOPY(int, m_voxelizedSceneHeight, VoxelizedSceneHeight)
	GETCOPY(int, m_voxelizedSceneDepth, VoxelizedSceneDepth)
	GET(mat4, m_projection, Projection)
	GET(mat4, m_viewX, ViewX)
	GET(mat4, m_viewY, ViewY)
	GET(mat4, m_viewZ, ViewZ)
		
protected:
	/** Record the rasterization commands to voxelize either the scene static nodes or the dynamic scene nodes
	* @param commandBuffer [in] Command buffer to record to
	* @return nothing */
	void recordRasterizationCommands(VkCommandBuffer* commandBuffer);

	/** Perform the commands needed to draw the elements in the node vector given as parameter
	* @param commandBuffer [in] command buffer to record to
	* @param vectorNode    [in] vector of nodes to draw
	* @return nothing */
	void drawNodeVector(VkCommandBuffer commandBuffer, vectorNodePtr& vectorNode);

	/** For each instance of MaterialColorTexture, an instance of MaterialSceneVoxelization is generated, with the suffix
	* "_Voxelization" or "_VoxelizationDynamic, so it can be recovered later when rasterizing the scene elements. The method is called when the scene
	* has been already loaded and the material instances of MaterialColorTexture for the scene elements generated
	* @return nothing */
	void generateVoxelizationMaterials();

	int           m_voxelizedSceneWidth;               //!< One of the dimensions of the 3D texture for the scene voxelization
	int           m_voxelizedSceneHeight;              //!< One of the dimensions of the 3D texture for the scene voxelization
	int           m_voxelizedSceneDepth;               //!< One of the dimensions of the 3D texture for the scene voxelization
	RenderPass*   m_renderPass;                        //!< Render pass used for voxelization
	Framebuffer*  m_framebuffer;                       //!< Framebuffer used for voxelization
	mat4          m_projection;                        //!< Orthographic projection matrix used for voxelization
	mat4          m_viewX;                             //!< x axis view matrix used for voxelization
	mat4          m_viewY;                             //!< y axis view matrix used for voxelization
	mat4          m_viewZ;                             //!< z axis view matrix used for voxelization
	Buffer*       m_voxelFirstIndexBuffer;             //!< Shader storage buffer to know the index of the first fragment generated for the voxelization 3D volume of coordinates hashed
	Buffer*       m_fragmentDataBuffer;                //!< Shader storage buffer with the per-fragment data generated during the voxelization
	Texture*      m_voxelizationOutputTexture;         //!< Voxelization texture output
	bool          m_isStaticSceneVoxelization;         //!< Flag to know if the instance generated is from a static scene voxelization technique (StaticSceneVoxelizationTechnique) or a dynamic scene voxelization technique (DynamicSceneVoxelizationTechnique)
	static string m_voxelizationMaterialSuffix;        //!< Suffix added to the material names used in the voxelization step
	static string m_voxelizationDynamicMaterialSuffix; //!< Suffix added to the material names used in the dynamic voxelization step
};

/////////////////////////////////////////////////////////////////////////////////////////////

#endif _SCENEVOXELIZATIONTECHNIQUE_H_
