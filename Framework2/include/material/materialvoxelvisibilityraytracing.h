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

#ifndef _MATERIALVOXELVISIBILITYRAYTRACING_H_
#define _MATERIALVOXELVISIBILITYRAYTRACING_H_

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

class MaterialVoxelVisibilityRayTracing : public Material
{
	friend class MaterialManager;

protected:
	/** Parameter constructor
	* @param [in] shader's name
	* @return nothing */
	MaterialVoxelVisibilityRayTracing(string &&name) : Material(move(name), move(string("MaterialVoxelVisibilityRayTracing")))
		, m_localWorkGroupsXDimension(0)
		, m_localWorkGroupsYDimension(0)
		, m_numThreadExecuted(0)
		, m_numberThreadPerElement(0)
	{
		m_resourcesUsed = MaterialBufferResource::MBR_CAMERA | MaterialBufferResource::MBR_MATERIAL;
	}

public:
	/** Loads the shader to be used. This could be substituted
	* by reading from a file, where the shader details would be present
	* @return true if the shader was loaded successfully, and false otherwise */
	bool loadShader()
	{
		size_t shaderSize;
		void* rayGenShaderCode  = InputOutput::readFile("../data/vulkanshaders/voxelvisibilityraytraceraygeneration.rgen", &shaderSize);
		void* rayMissShaderCode = InputOutput::readFile("../data/vulkanshaders/voxelvisibilityraytraceraymiss.rmiss",       &shaderSize);
		void* rayHitShaderCode  = InputOutput::readFile("../data/vulkanshaders/voxelvisibilityraytracerayhit.rchit",      &shaderSize);

		assert(rayGenShaderCode  != nullptr);
		assert(rayMissShaderCode != nullptr);
		assert(rayHitShaderCode  != nullptr);

		string stringRayGenShaderCode  = string((const char*)rayGenShaderCode);
		string stringRayMissShaderCode = string((const char*)rayMissShaderCode);
		string stringRayHitShaderCode  = string((const char*)rayHitShaderCode);

		m_shaderResourceName = "voxelvisibilityraytracing";
		m_shader             = shaderM->buildShaderRayGenMissHit(move(string(m_shaderResourceName)), stringRayGenShaderCode.c_str(), stringRayMissShaderCode.c_str(), stringRayHitShaderCode.c_str(), m_materialSurfaceType);

		return true;
	}

	/** Exposes the variables that will be modified in the corresponding material data uniform buffer used in the scene,
	* allowing to work from variables from C++ without needing to update the values manually (error-prone)
	* @return nothing */
	void exposeResources()
	{
		exposeStructField(ResourceInternalType::RIT_FLOAT_VEC4, (void*)(&m_sceneMinAndNumberVoxel),  move(string("myMaterialData")), move(string("sceneMinAndNumberVoxel")));
		exposeStructField(ResourceInternalType::RIT_FLOAT_VEC4, (void*)(&m_sceneExtentAndVoxelSize), move(string("myMaterialData")), move(string("sceneExtentAndVoxelSize")));
		
		assignShaderStorageBuffer(move(string("voxelHashedPositionCompactedBuffer")), move(string("voxelHashedPositionCompactedBuffer")), VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);
		assignShaderStorageBuffer(move(string("IndirectionIndexBuffer")),             move(string("IndirectionIndexBuffer")),             VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);
		assignShaderStorageBuffer(move(string("IndirectionRankBuffer")),              move(string("IndirectionRankBuffer")),              VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);
		assignShaderStorageBuffer(move(string("voxelVisibility4BytesBuffer")),        move(string("voxelVisibility4BytesBuffer")),        VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);
		assignShaderStorageBuffer(move(string("voxelVisibilityDebugBuffer")),         move(string("voxelVisibilityDebugBuffer")),         VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);
		assignShaderStorageBuffer(move(string("voxelOccupiedBuffer")),                move(string("voxelOccupiedBuffer")),                VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);
		assignShaderStorageBuffer(move(string("rayDirectionBuffer")),                 move(string("rayDirectionBuffer")),                 VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);

		assignImageToSampler(move(string("voxelizationReflectance")), move(string("voxelizationReflectance")), VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_FILTER_LINEAR);
		assignImageToSampler(move(string("raytracingoffscreen")),     move(string("raytracingoffscreen")),     VK_DESCRIPTOR_TYPE_STORAGE_IMAGE);
		
		assignTextureToSampler(move(string("staticVoxelIndexTexture")), move(string("staticVoxelIndexTexture")), VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);

		assignAccelerationStructure(move(string("staticraytracedaccelerationstructure")), move(string("staticraytracedaccelerationstructure")), VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR);

		AccelerationStructure* staticraytracedaccelerationstructure = accelerationStructureM->getElement(move(string("staticraytracedaccelerationstructure")));
		m_vectorNode = staticraytracedaccelerationstructure->getVectorNode();

		vector<string> vectorIndexBufferName(m_vectorNode.size());
		vector<string> vectorVertexBufferName(m_vectorNode.size());

		forI(m_vectorNode.size())
		{
			const RenderComponent* renderComponent = m_vectorNode[i]->getRenderComponent();
			vectorIndexBufferName[i]               = renderComponent->getIndexBuffer()->getName();
			vectorVertexBufferName[i]              = renderComponent->getVertexBuffer()->getName();
		}

		Buffer* temp = bufferM->getElement(move(string("sceneDescriptorStaticBuffer")));

		assignShaderStorageBuffer(move(string("vertices")),                    move(vectorVertexBufferName),                VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);
		assignShaderStorageBuffer(move(string("indices")),                     move(vectorIndexBufferName),                 VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);
		assignShaderStorageBuffer(move(string("sceneDescriptorStaticBuffer")), move(string("sceneDescriptorStaticBuffer")), VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);
	}

	GETCOPY_SET(uint, m_localWorkGroupsXDimension, LocalWorkGroupsXDimension)
	GETCOPY_SET(uint, m_localWorkGroupsYDimension, LocalWorkGroupsYDimension)
	SET(uint, m_numThreadExecuted, NumThreadExecuted)
	SET(uint, m_numberThreadPerElement, NumberThreadPerElement)
	SET(vec4, m_sceneMinAndNumberVoxel, SceneMinAndNumberVoxel)
	SET(vec4, m_sceneExtentAndVoxelSize, SceneExtentAndVoxelSize)

protected:
	uint          m_localWorkGroupsXDimension; //!< Number of dispatched local workgroups in the x dimension
	uint          m_localWorkGroupsYDimension; //!< Number of dispatched local workgroups in the y dimension
	uint          m_numThreadExecuted;         //!< Number of thread executed
	uint          m_numberThreadPerElement;    //!< Number of threads that process each element
	vec4          m_sceneMinAndNumberVoxel;    //!< Minimum value of the scene's aabb in xyz coordinates, number of occupied voxels
	vec4          m_sceneExtentAndVoxelSize;   //!< Extent of the scene in the xyz coordinates, voxelization texture size in the w coordinate
	vectorNodePtr m_vectorNode;                //!< Vector with pointers to the nodes to be used in the ray tracing technique that uses this shader
};

/////////////////////////////////////////////////////////////////////////////////////////////

#endif _MATERIALVOXELVISIBILITYRAYTRACING_H_
