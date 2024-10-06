/*
Copyright 2021 Alejandro Cosin

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

#ifndef _SCENERAYTRACINGTECHNIQUE_H_
#define _SCENERAYTRACINGTECHNIQUE_H_

// GLOBAL INCLUDES

// PROJECT INCLUDES
#include "../../include/rastertechnique/rastertechnique.h"

// CLASS FORWARDING
class MaterialSceneRayTrace;
class AccelerationStructure;
class Texture;
class Buffer;

// NAMESPACE

// DEFINES

/////////////////////////////////////////////////////////////////////////////////////////////

class SceneRayTracingTechnique: public RasterTechnique
{
	DECLARE_FRIEND_REGISTERER(SceneRayTracingTechnique)

protected:
	/** Parameter constructor
	* @param name      [in] technique's name
	* @param className [in] class name
	* @return nothing */
	SceneRayTracingTechnique(string &&name, string&& className);

	/** Default destructor
	* @return nothing */
	virtual ~SceneRayTracingTechnique();

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
	/** Build the shader binding table
	* @return nothing */
	void buildShaderBindingTable();

	MaterialSceneRayTrace* m_materialSceneRayTrace;       //!< Pointer to the instance of the scene ray tracing shader
	AccelerationStructure* m_staticAccelerationStructure; //!< Acceleration structure to be ray traced
	Texture*               m_rayTracingOffscreen;         //!< Offscreen render target for the ray tracing pass
	Buffer*                m_sceneDescriptorBuffer;       //!< Buffer with the scene descriptor information
	Buffer*                m_shaderBindingTableBuffer;    //!< Shader binding table buffer
	bool                   m_shaderBindingTableBuilt;     //!< Flag to know whther the shader binding table has been built
	uint                   m_shaderGroupBaseAlignment;    //!< Value of field VkPhysicalDeviceRayTracingPipelinePropertiesKHR::shaderGroupBaseAlignment
	uint                   m_shaderGroupHandleSize;       //!< Value of field VkPhysicalDeviceRayTracingPipelinePropertiesKHR::shaderGroupHandleSize
	uint                   m_shaderGroupSizeAligned;      //!< Size of a shader group when considering aligned memory, for building teh shader binding table
	uint                   m_shaderGroupStride;           //!< Stride of a shader group
};

/////////////////////////////////////////////////////////////////////////////////////////////

#endif _SCENERAYTRACINGTECHNIQUE_H_
