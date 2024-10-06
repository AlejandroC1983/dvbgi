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

#ifndef _MATERIALLIGHTBOUNCESTATICVOXELPADDING_H_
#define _MATERIALLIGHTBOUNCESTATICVOXELPADDING_H_

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

class MaterialLightBounceStaticVoxelPadding: public Material
{
	friend class MaterialManager;

protected:
	/** Parameter constructor
	* @param [in] shader's name
	* @return nothing */
	MaterialLightBounceStaticVoxelPadding(string &&name) : Material(move(name), move(string("MaterialLightBounceStaticVoxelPadding")))
		, m_localWorkGroupsXDimension(0)
		, m_localWorkGroupsYDimension(0)
		, m_numThreadExecuted(0)
		, m_numberThreadPerElement(1)
		, m_voxelizationSize(0)
		, m_numThreadPerLocalWorkgroup(8)
		, m_numElementPerLocalWorkgroupThread(1)
	{
		m_resourcesUsed = MaterialBufferResource::MBR_MATERIAL;

		buildShaderThreadMapping();
	}

public:
	/** Loads the shader to be used. This could be substituted
	* by reading from a file, where the shader details would be present
	* @return true if the shader was loaded successfully, and false otherwise */
	bool loadShader()
	{
		size_t sizeFirstPart;
		size_t sizeSecondPart;

		void* shaderFirstPartCode  = InputOutput::readFile("../data/vulkanshaders/lightbouncestaticvoxelpaddingfirstpart.comp", &sizeFirstPart);
		void* shaderSecondPartCode = InputOutput::readFile("../data/vulkanshaders/lightbouncestaticvoxelpaddingsecondpart.comp", &sizeSecondPart);

		assert(shaderFirstPartCode  != nullptr);
		assert(shaderSecondPartCode != nullptr);

		string firstPart     = string((const char*)shaderFirstPartCode);
		string secondPart    = string((const char*)shaderSecondPartCode);
		string shaderCode    = firstPart;
		shaderCode          += m_computeShaderThreadMapping;
		shaderCode          += secondPart;
		m_shaderResourceName = "lightbouncestaticvoxelpadding";
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
		exposeStructField(ResourceInternalType::RIT_UNSIGNED_INT, (void*)(&m_voxelizationSize),          move(string("myMaterialData")), move(string("voxelizationSize")));

		assignShaderStorageBuffer(move(string("irradiancePaddingTagTilesIndexBuffer")), move(string("irradiancePaddingTagTilesIndexBuffer")), VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);
		assignShaderStorageBuffer(move(string("staticVoxelPaddingDebugBuffer")),        move(string("staticVoxelPaddingDebugBuffer")),        VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);
		assignShaderStorageBuffer(move(string("voxelOccupiedBuffer")),                  move(string("voxelOccupiedBuffer")),                  VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);
		assignShaderStorageBuffer(move(string("lightBounceVoxelIrradianceBuffer")),     move(string("lightBounceVoxelIrradianceBuffer")),     VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);
		assignShaderStorageBuffer(move(string("lightBounceVoxelIrradianceTempBuffer")), move(string("lightBounceVoxelIrradianceTempBuffer")), VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);

		assignImageToSampler(move(string("irradiance3DStaticNegativeX")),  move(string("irradiance3DStaticNegativeX")),  VK_DESCRIPTOR_TYPE_STORAGE_IMAGE);
		assignImageToSampler(move(string("irradiance3DStaticPositiveX")),  move(string("irradiance3DStaticPositiveX")),  VK_DESCRIPTOR_TYPE_STORAGE_IMAGE);
		assignImageToSampler(move(string("irradiance3DStaticNegativeY")),  move(string("irradiance3DStaticNegativeY")),  VK_DESCRIPTOR_TYPE_STORAGE_IMAGE);
		assignImageToSampler(move(string("irradiance3DStaticPositiveY")),  move(string("irradiance3DStaticPositiveY")),  VK_DESCRIPTOR_TYPE_STORAGE_IMAGE);
		assignImageToSampler(move(string("irradiance3DStaticNegativeZ")),  move(string("irradiance3DStaticNegativeZ")),  VK_DESCRIPTOR_TYPE_STORAGE_IMAGE);
		assignImageToSampler(move(string("irradiance3DStaticPositiveZ")),  move(string("irradiance3DStaticPositiveZ")),  VK_DESCRIPTOR_TYPE_STORAGE_IMAGE);

		assignTextureToSampler(move(string("staticVoxelNeighbourInfo")), move(string("staticVoxelNeighbourInfo")), VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
		assignTextureToSampler(move(string("staticVoxelIndexTexture")),  move(string("staticVoxelIndexTexture")),  VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
	}

	// TODO: Prepare a parent class for this specific kind of materials and add the commonly used member variables
	void buildShaderThreadMapping()
	{
		uvec3 minComputeWorkGroupSize = coreM->getMinComputeWorkGroupSize();
		float tempValue = float(m_numThreadPerLocalWorkgroup) / float(minComputeWorkGroupSize.y);

		if (tempValue >= 1.0f)
		{
			m_localSizeX = int(ceil(tempValue));
			m_localSizeY = minComputeWorkGroupSize.y;
		}
		else
		{
			m_localSizeX = m_numThreadPerLocalWorkgroup;
			m_localSizeY = 1;
		}

		m_computeShaderThreadMapping += "\n\n";
		m_computeShaderThreadMapping += "layout(local_size_x = " + to_string(m_localSizeX) + ", local_size_y = " + to_string(m_localSizeY) + ", local_size_z = 1) in;\n\n";
		m_computeShaderThreadMapping += "void main()\n";
		m_computeShaderThreadMapping += "{\n";
		m_computeShaderThreadMapping += "\tconst uint ELEMENT_PER_THREAD         = " + to_string(m_numElementPerLocalWorkgroupThread) + ";\n";
		m_computeShaderThreadMapping += "\tconst uint THREAD_PER_LOCAL_WORKGROUP = " + to_string(m_numThreadPerLocalWorkgroup) + ";\n";
		m_computeShaderThreadMapping += "\tconst uint LOCAL_SIZE_X_VALUE         = " + to_string(m_localSizeX) + ";\n";
		m_computeShaderThreadMapping += "\tconst uint LOCAL_SIZE_Y_VALUE         = " + to_string(m_localSizeY) + ";\n";
	}

	SET(string, m_computeShaderThreadMapping, ComputeShaderThreadMapping)
	GETCOPY_SET(uint, m_localWorkGroupsXDimension, LocalWorkGroupsXDimension)
	GETCOPY_SET(uint, m_localWorkGroupsYDimension, LocalWorkGroupsYDimension)
	SET(uint, m_numThreadExecuted, NumThreadExecuted)
	GETCOPY(uint, m_numberThreadPerElement, NumberThreadPerElement)
	SET(uint, m_voxelizationSize, VoxelizationSize)
	GETCOPY(uint, m_numThreadPerLocalWorkgroup, NumThreadPerLocalWorkgroup)

protected:
	string m_computeShaderThreadMapping;        //!< String with the thread mapping taking into account m_bufferNumElement, m_numElementPerLocalWorkgroupThread and m_numThreadPerLocalWorkgroup values (and physical device limits)
	uint   m_localWorkGroupsXDimension;         //!< Number of dispatched local workgroups in the x dimension
	uint   m_localWorkGroupsYDimension;         //!< Number of dispatched local workgroups in the y dimension
	uint   m_numThreadExecuted;                 //!< Number of thread executed
	uint   m_numberThreadPerElement;            //!< Number of threads that process each element
	uint   m_voxelizationSize;                  //!< Size of the voxelization texture (assuiming the same resolution in all dimensions)
	uint   m_numElementPerLocalWorkgroupThread; //!< Number of elements per thread in each local workgroup
	uint   m_numThreadPerLocalWorkgroup;        //!< Number of threads per local workgroup
	int    m_localSizeX;                        //!< Compute shader value for local_size_x
	int    m_localSizeY;                        //!< Compute shader value for local_size_x
};

/////////////////////////////////////////////////////////////////////////////////////////////

#endif _MATERIALLIGHTBOUNCESTATICVOXELPADDING_H_
