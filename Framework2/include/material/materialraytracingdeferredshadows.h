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

#ifndef _MATERIALRAYTRACINGDEFERREDSHADOWS_H_
#define _MATERIALRAYTRACINGDEFERREDSHADOWS_H_

// GLOBAL INCLUDES

// PROJECT INCLUDES
#include "../../include/material/materialmanager.h"
#include "../../include/material/material.h"
#include "../../include/shader/shadermanager.h"
#include "../../include/shader/shader.h"
#include "../../include/buffer/buffermanager.h"
#include "../../include/buffer/buffer.h"
#include "../../include/component/basecomponent.h"
#include "../../include/component/rendercomponent.h"
#include "../../include/accelerationstructure/accelerationstructure.h"
#include "../../include/accelerationstructure/accelerationstructuremanager.h"

// CLASS FORWARDING

// NAMESPACE

// DEFINES

/////////////////////////////////////////////////////////////////////////////////////////////

class MaterialRayTracingDeferredShadows: public Material
{
	friend class MaterialManager;

protected:
	/** Parameter constructor
	* @param [in] shader's name
	* @return nothing */
	MaterialRayTracingDeferredShadows(string &&name) : Material(move(name), move(string("MaterialRayTracingDeferredShadows")))
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
		void* rayGenShaderCode  = InputOutput::readFile("../data/vulkanshaders/raytracingdeferredshadows.rgen", &shaderSize);		
		void* rayMissShaderCode = InputOutput::readFile("../data/vulkanshaders/raytracingdeferredshadows.rmiss",      &shaderSize);
		void* rayHitShaderCode  = InputOutput::readFile("../data/vulkanshaders/raytracingdeferredshadows.rchit",       &shaderSize);

		assert(rayGenShaderCode  != nullptr);
		assert(rayMissShaderCode != nullptr);
		assert(rayHitShaderCode  != nullptr);

		string stringRayGenShaderCode  = string((const char*)rayGenShaderCode);
		string stringRayMissShaderCode = string((const char*)rayMissShaderCode);
		string stringRayHitShaderCode  = string((const char*)rayHitShaderCode);

		m_shaderResourceName = "cameravisibleraytracing";
		m_shader             = shaderM->buildShaderRayGenMissHit(move(string(m_shaderResourceName)), stringRayGenShaderCode.c_str(), stringRayMissShaderCode.c_str(), stringRayHitShaderCode.c_str(), m_materialSurfaceType);

		return true;
	}

	/** Exposes the variables that will be modified in the corresponding material data uniform buffer used in the scene,
	* allowing to work from variables from C++ without needing to update the values manually (error-prone)
	* @return nothing */
	void exposeResources()
	{
		exposeStructField(ResourceInternalType::RIT_FLOAT_VEC4, (void*)(&m_sceneMin),                    move(string("myMaterialData")), move(string("sceneMin")));
		exposeStructField(ResourceInternalType::RIT_FLOAT_VEC4, (void*)(&m_sceneExtentVoxelizationSize), move(string("myMaterialData")), move(string("sceneExtentVoxelizationSize")));
		exposeStructField(ResourceInternalType::RIT_FLOAT_VEC4, (void*)(&m_lightPosition),               move(string("myMaterialData")), move(string("lightPosition")));
		exposeStructField(ResourceInternalType::RIT_FLOAT_VEC4, (void*)(&m_lightForward),                move(string("myMaterialData")), move(string("lightForward")));

		assignAccelerationStructure(move(string("raytracedaccelerationstructure")), move(string("raytracedaccelerationstructure")), VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR);

		assignTextureToSampler(move(string("GBufferPosition")), move(string("GBufferPosition")), VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);

		assignImageToSampler(move(string("GBufferReflectance")), move(string("GBufferReflectance")), VK_DESCRIPTOR_TYPE_STORAGE_IMAGE);

		AccelerationStructure* raytracedaccelerationstructure = accelerationStructureM->getElement(move(string("raytracedaccelerationstructure")));
		m_vectorNode                                          = raytracedaccelerationstructure->getVectorNode();

		vector<string> vectorIndexBufferName(m_vectorNode.size());
		vector<string> vectorVertexBufferName(m_vectorNode.size());

		forI(m_vectorNode.size())
		{
			const RenderComponent* renderComponent = m_vectorNode[i]->getRenderComponent();
			vectorIndexBufferName[i]               = renderComponent->getIndexBuffer()->getName();
			vectorVertexBufferName[i]              = renderComponent->getVertexBuffer()->getName();
		}

		assignShaderStorageBuffer(move(string("cameraVisibleVoxelPerByteBuffer")),        move(string("cameraVisibleVoxelPerByteBuffer")),        VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);
		assignShaderStorageBuffer(move(string("cameraVisibleDynamicVoxelPerByteBuffer")), move(string("cameraVisibleDynamicVoxelPerByteBuffer")), VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);
		assignShaderStorageBuffer(move(string("vertices")),                               move(vectorVertexBufferName),                           VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);
		assignShaderStorageBuffer(move(string("indices")),                                move(vectorIndexBufferName),                            VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);
		assignShaderStorageBuffer(move(string("sceneDescriptorBuffer")),                  move(string("sceneDescriptorBuffer")),                  VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);
		assignShaderStorageBuffer(move(string("voxelOccupiedBuffer")),                    move(string("voxelOccupiedBuffer")),                    VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);
		assignShaderStorageBuffer(move(string("voxelOccupiedDynamicBuffer")),             move(string("voxelOccupiedDynamicBuffer")),             VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);
	}

	SET(vec4, m_sceneMin, SceneMin)
	SET(vec4, m_sceneExtentVoxelizationSize, SceneExtentVoxelizationSize)
	SET(vec4, m_lightPosition, LightPosition)
	SET(vec4, m_lightForward, LightForward)

protected:
	vec4          m_sceneMin;                    //!< Scene min
	vec4          m_sceneExtentVoxelizationSize; //!< Scene extent and voxel size in w field
	vec4          m_lightPosition;               //!< Light position in world coordinates
	vec4          m_lightForward;                //!< Light direction in world coordinates
	vectorNodePtr m_vectorNode;                  //!< Vector with pointers to the static and dynamic nodes to be used in the ray tracing technique that uses this shader
};

/////////////////////////////////////////////////////////////////////////////////////////////

#endif _MATERIALRAYTRACINGDEFERREDSHADOWS_H_
