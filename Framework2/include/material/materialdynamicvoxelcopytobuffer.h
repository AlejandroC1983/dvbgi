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

#ifndef _MATERIALDYNAMICVOXELCOPYTOBUFFER_H_
#define _MATERIALDYNAMICVOXELCOPYTOBUFFER_H_

// GLOBAL INCLUDES

// PROJECT INCLUDES
#include "../../include/material/materialmanager.h"
#include "../../include/material/material.h"
#include "../../include/shader/shadermanager.h"
#include "../../include/shader/shader.h"
#include "../../include/buffer/buffermanager.h"
#include "../../include/buffer/buffer.h"

// CLASS FORWARDING

// NAMESPACE

// DEFINES

/////////////////////////////////////////////////////////////////////////////////////////////

class MaterialDynamicVoxelCopyToBuffer : public Material
{
	friend class MaterialManager;

protected:
	/** Parameter constructor
	* @param [in] shader's name
	* @return nothing */
	MaterialDynamicVoxelCopyToBuffer(string &&name) : Material(move(name), move(string("MaterialDynamicVoxelCopyToBuffer")))
		, m_localWorkGroupsXDimension(0)
		, m_localWorkGroupsYDimension(0)
		, m_numThreadExecuted(0)
	{
		m_resourcesUsed = MaterialBufferResource::MBR_MATERIAL;
	}

public:
	/** Loads the shader to be used. This could be substituted
	* by reading from a file, where the shader details would be present
	* @return true if the shader was loaded successfully, and false otherwise */
	bool loadShader()
	{
		size_t sizeFirstPart;
		size_t sizeSecondPart;

		void* shaderFirstPartCode  = InputOutput::readFile("../data/vulkanshaders/dynamicvoxelcopytobufferfirstpart.comp", &sizeFirstPart);
		void* shaderSecondPartCode = InputOutput::readFile("../data/vulkanshaders/dynamicvoxelcopytobuffersecondpart.comp", &sizeSecondPart);

		assert(shaderFirstPartCode  != nullptr);
		assert(shaderSecondPartCode != nullptr);

		string firstPart     = string((const char*)shaderFirstPartCode);
		string secondPart    = string((const char*)shaderSecondPartCode);
		string shaderCode    = firstPart;
		shaderCode          += m_computeShaderThreadMapping;
		shaderCode          += secondPart;
		m_shaderResourceName = "dynamicvoxelcopytobuffer";
		m_shader             = shaderM->buildShaderC(move(string(m_shaderResourceName)), shaderCode.c_str(), m_materialSurfaceType);

		return true;
	}

	/** Exposes the variables that will be modified in the corresponding material data uniform buffer used in the scene,
	* allowing to work from variables from C++ without needing to update the values manually (error-prone)
	* @return nothing */
	void exposeResources()
	{
		exposeStructField(ResourceInternalType::RIT_UNSIGNED_INT, (void*)(&m_localWorkGroupsXDimension), move(string("myMaterialData")), move(string("localWorkGroupsXDimension")));
		exposeStructField(ResourceInternalType::RIT_UNSIGNED_INT, (void*)(&m_localWorkGroupsYDimension), move(string("myMaterialData")), move(string("localWorkGroupsYDimension")));
		exposeStructField(ResourceInternalType::RIT_UNSIGNED_INT, (void*)(&m_numThreadExecuted),         move(string("myMaterialData")), move(string("numThreadExecuted")));
		exposeStructField(ResourceInternalType::RIT_UNSIGNED_INT, (void*)(&m_padding),                   move(string("myMaterialData")), move(string("padding")));

		assignShaderStorageBuffer(move(string("voxelOccupiedDynamicBuffer")),  move(string("voxelOccupiedDynamicBuffer")),  VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);
		assignShaderStorageBuffer(move(string("dynamicVoxelCounterBuffer")),   move(string("dynamicVoxelCounterBuffer")),   VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);
		assignShaderStorageBuffer(move(string("dynamicVoxelBuffer")),          move(string("dynamicVoxelBuffer")),          VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);
		assignShaderStorageBuffer(move(string("dynamicvoxelCopyDebugBuffer")), move(string("dynamicvoxelCopyDebugBuffer")), VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);
	}

	SET(string, m_computeShaderThreadMapping, ComputeShaderThreadMapping)
	GETCOPY_SET(uint, m_localWorkGroupsXDimension, LocalWorkGroupsXDimension)
	GETCOPY_SET(uint, m_localWorkGroupsYDimension, LocalWorkGroupsYDimension)
	SET(uint, m_numThreadExecuted, NumThreadExecuted)

protected:
	string m_computeShaderThreadMapping; //!< String with the thread mapping taking into account m_bufferNumElement, m_numElementPerLocalWorkgroupThread and m_numThreadPerLocalWorkgroup values (and physical device limits)
	uint   m_localWorkGroupsXDimension;  //!< Number of dispatched local workgroups in the x dimension
	uint   m_localWorkGroupsYDimension;  //!< Number of dispatched local workgroups in the y dimension
	uint   m_numThreadExecuted;          //!< Number of threads executed
	uint   m_padding;                    //!< Just for padding to have vec4 alignment in the shader
};

/////////////////////////////////////////////////////////////////////////////////////////////

#endif _MATERIALDYNAMICVOXELCOPYTOBUFFER_H_
