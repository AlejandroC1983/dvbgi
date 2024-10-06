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

#ifndef _MATERIALLIGHTBOUNCEDYNAMICVOXELIRRADIANCE_H_
#define _MATERIALLIGHTBOUNCEDYNAMICVOXELIRRADIANCE_H_

// GLOBAL INCLUDES

// PROJECT INCLUDES
#include "../../include/material/materialmanager.h"
#include "../../include/material/material.h"
#include "../../include/shader/shadermanager.h"
#include "../../include/shader/shader.h"
#include "../../include/buffer/buffermanager.h"
#include "../../include/buffer/buffer.h"
#include "../../include/material/materialscenevoxelization.h"
#include "../../include/component/basecomponent.h"
#include "../../include/component/rendercomponent.h"
#include "../../include/scene/scene.h"
#include "../../include/accelerationstructure/accelerationstructure.h"
#include "../../include/accelerationstructure/accelerationstructuremanager.h"

// CLASS FORWARDING

// NAMESPACE

// DEFINES

/////////////////////////////////////////////////////////////////////////////////////////////

class MaterialLightBounceDynamicVoxelIrradiance: public Material
{
	friend class MaterialManager;

protected:
	/** Parameter constructor
	* @param [in] shader's name
	* @return nothing */
	MaterialLightBounceDynamicVoxelIrradiance(string &&name) : Material(move(name), move(string("MaterialLightBounceDynamicVoxelIrradiance")))
		, m_sceneMinAndCameraVisibleVoxel(vec4(0.0f))
		, m_sceneExtentAndNumElement(vec4(0.0f))
	{
		m_resourcesUsed = MaterialBufferResource::MBR_MATERIAL;
	}

public:
	/** Loads the shader to be used. This could be substituted
	* by reading from a file, where the shader details would be present
	* @return true if the shader was loaded successfully, and false otherwise */
	bool loadShader()
	{
		size_t shaderSize;
		void* rayGenShaderCode  = InputOutput::readFile("../data/vulkanshaders/lightbouncedynamicvoxelirradiance.rgen",  &shaderSize);		
		void* rayMissShaderCode = InputOutput::readFile("../data/vulkanshaders/lightbouncedynamicvoxelirradiance.rmiss", &shaderSize);
		void* rayHitShaderCode  = InputOutput::readFile("../data/vulkanshaders/lightbouncedynamicvoxelirradiance.rchit", &shaderSize);

		assert(rayGenShaderCode  != nullptr);
		assert(rayMissShaderCode != nullptr);
		assert(rayHitShaderCode  != nullptr);

		string stringRayGenShaderCode  = string((const char*)rayGenShaderCode);
		string stringRayMissShaderCode = string((const char*)rayMissShaderCode);
		string stringRayHitShaderCode  = string((const char*)rayHitShaderCode);

		m_shaderResourceName = "lightbouncedynamicvoxelirradiance";
		m_shader             = shaderM->buildShaderRayGenMissHit(move(string(m_shaderResourceName)), stringRayGenShaderCode.c_str(), stringRayMissShaderCode.c_str(), stringRayHitShaderCode.c_str(), m_materialSurfaceType);

		return true;
	}

	/** Exposes the variables that will be modified in the corresponding material data uniform buffer used in the scene,
	* allowing to work from variables from C++ without needing to update the values manually (error-prone)
	* @return nothing */
	void exposeResources()
	{
		exposeStructField(ResourceInternalType::RIT_FLOAT_VEC4, (void*)(&m_sceneMinAndCameraVisibleVoxel), move(string("myMaterialData")), move(string("sceneMinAndCameraVisibleVoxel")));
		exposeStructField(ResourceInternalType::RIT_FLOAT_VEC4, (void*)(&m_sceneExtentAndNumElement),      move(string("myMaterialData")), move(string("sceneExtentAndNumElement")));
		exposeStructField(ResourceInternalType::RIT_FLOAT_VEC4, (void*)(&m_lightPositionAndVoxelSize),     move(string("myMaterialData")), move(string("lightPositionAndVoxelSize")));
		exposeStructField(ResourceInternalType::RIT_FLOAT_VEC4, (void*)(&m_lightForwardEmitterRadiance),   move(string("myMaterialData")), move(string("lightForwardEmitterRadiance")));
		
		AccelerationStructure * raytracedaccelerationstructure = accelerationStructureM->getElement(move(string("raytracedaccelerationstructure")));
		m_vectorNode = raytracedaccelerationstructure->getVectorNode();

		vector<string> vectorIndexBufferName(m_vectorNode.size());
		vector<string> vectorVertexBufferName(m_vectorNode.size());

		forI(m_vectorNode.size())
		{
			const RenderComponent* renderComponent = m_vectorNode[i]->getRenderComponent();
			vectorIndexBufferName[i]               = renderComponent->getIndexBuffer()->getName();
			vectorVertexBufferName[i]              = renderComponent->getVertexBuffer()->getName();
		}

		assignShaderStorageBuffer(move(string("cameraVisibleDynamicVoxelBuffer")),  move(string("cameraVisibleDynamicVoxelBuffer")),  VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);
		assignShaderStorageBuffer(move(string("dynamicLightBounceDebugBuffer")),    move(string("dynamicLightBounceDebugBuffer")),    VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);
		assignShaderStorageBuffer(move(string("rayDirectionBuffer")),               move(string("rayDirectionBuffer")),               VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);
		assignShaderStorageBuffer(move(string("dynamicVoxelVisibilityBuffer")),     move(string("dynamicVoxelVisibilityBuffer")),     VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);
		
		assignShaderStorageBuffer(move(string("vertices")),                         move(vectorVertexBufferName),                     VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);
		assignShaderStorageBuffer(move(string("indices")),                          move(vectorIndexBufferName),                      VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);
		assignShaderStorageBuffer(move(string("sceneDescriptorBuffer")),            move(string("sceneDescriptorBuffer")),            VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);

		assignAccelerationStructure(move(string("raytracedaccelerationstructure")), move(string("raytracedaccelerationstructure")), VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR);
	}

	SET(string, m_computeShaderThreadMapping, ComputeShaderThreadMapping)
	SET(vec4, m_sceneMinAndCameraVisibleVoxel, SceneMinAndCameraVisibleVoxel)
	SET(vec4, m_sceneExtentAndNumElement, SceneExtentAndNumElement)
	SET(vec4, m_lightPositionAndVoxelSize, LightPositionAndVoxelSize)
	SET(vec4, m_lightForwardEmitterRadiance, LightForwardEmitterRadiance)

protected:
	string        m_computeShaderThreadMapping;    //!< String with the thread mapping taking into account m_bufferNumElement, m_numElementPerLocalWorkgroupThread and m_numThreadPerLocalWorkgroup values (and physical device limits)
	vec4          m_sceneMinAndCameraVisibleVoxel; //!< Minimum value of the scene's aabb and amount of dynamic visible voxels from the camera in the .w field
	vec4          m_sceneExtentAndNumElement;      //!< Extent of the scene and number of elements to be processed by the dispatch
	vec4          m_lightPositionAndVoxelSize;     //!< Light position in world coordinates and voxelization resolution in the w component
	vec4          m_lightForwardEmitterRadiance;   //!< Forward direction of the light vector, emitter radiance in the .w field
	vectorNodePtr m_vectorNodeStatic;              //!< Vector with pointers to the static nodes to be used in the ray tracing technique that uses this shader
	vectorNodePtr m_vectorNodeDynamic;             //!< Vector with pointers to the dynamic nodes to be used in the ray tracing technique that uses this shader
	vectorNodePtr m_vectorNode;                    //!< Vector with pointers to the static and dynamic nodes to be used in the ray tracing technique that uses this shader
};

/////////////////////////////////////////////////////////////////////////////////////////////

#endif _MATERIALLIGHTBOUNCEDYNAMICVOXELIRRADIANCE_H_
