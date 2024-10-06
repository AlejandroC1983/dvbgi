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

#ifndef _MATERIALNEIGHBOURLITVOXELINFORMATION_H_
#define _MATERIALNEIGHBOURLITVOXELINFORMATION_H_

// GLOBAL INCLUDES

// PROJECT INCLUDES
#include "../../include/material/materialmanager.h"
#include "../../include/material/material.h"
#include "../../include/shader/shadermanager.h"
#include "../../include/shader/shader.h"
#include "../../include/buffer/buffermanager.h"
#include "../../include/buffer/buffer.h"
#include "../../include/material/materialscenevoxelization.h"

// CLASS FORWARDING

// NAMESPACE

// DEFINES

/////////////////////////////////////////////////////////////////////////////////////////////

class MaterialNeighbourLitVoxelInformation: public Material
{
	friend class MaterialManager;

protected:
	/** Parameter constructor
	* @param [in] shader's name
	* @return nothing */
	MaterialNeighbourLitVoxelInformation(string &&name) : Material(move(name), move(string("MaterialNeighbourLitVoxelInformation")))
		, m_localWorkGroupsXDimension(0)
		, m_localWorkGroupsYDimension(0)
		, m_voxelSize(0.0f)
		, m_numElementPerLocalWorkgroupThread(1)
		, m_numThreadPerLocalWorkgroup(0)
		, m_localSizeX(1)
		, m_localSizeY(1)
		, m_numElementToProcess(0)
	{
		m_resourcesUsed              = MaterialBufferResource::MBR_MATERIAL;
		m_numThreadPerLocalWorkgroup = 64;

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

		void* shaderFirstPartCode  = InputOutput::readFile("../data/vulkanshaders/neighbourlitvoxelinformationfirstpart.comp",  &sizeFirstPart);
		void* shaderSecondPartCode = InputOutput::readFile("../data/vulkanshaders/neighbourlitvoxelinformationsecondpart.comp", &sizeSecondPart);

		assert(shaderFirstPartCode  != nullptr);
		assert(shaderSecondPartCode != nullptr);

		string firstPart  = string((const char*)shaderFirstPartCode);
		string secondPart = string((const char*)shaderSecondPartCode);
		string shaderCode = firstPart;
		shaderCode       += m_computeShaderThreadMapping;
		shaderCode       += secondPart;

		m_shaderResourceName = "neighbourlitvoxelinformation";
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
		exposeStructField(ResourceInternalType::RIT_UNSIGNED_INT, (void*)(&m_numElementToProcess),       move(string("myMaterialData")), move(string("numElementToProcess")));
		exposeStructField(ResourceInternalType::RIT_FLOAT,        (void*)(&m_voxelSize),                 move(string("myMaterialData")), move(string("voxelSize")));
		
		assignShaderStorageBuffer(move(string("voxelHashedPositionCompactedBuffer")), move(string("voxelHashedPositionCompactedBuffer")), VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);
		assignShaderStorageBuffer(move(string("voxelOccupiedBuffer")),                move(string("voxelOccupiedBuffer")),                VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);
		assignShaderStorageBuffer(move(string("litTestVoxelPerByteBuffer")),          move(string("litTestVoxelPerByteBuffer")),          VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);
		assignShaderStorageBuffer(move(string("neighbourLitVoxelDebugBuffer")),       move(string("neighbourLitVoxelDebugBuffer")),       VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);
		assignShaderStorageBuffer(move(string("neighbourLitVoxelInformationBuffer")), move(string("neighbourLitVoxelInformationBuffer")), VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);
	}

	/** Set the number of compute workgroups to dispatch in x and y dimension (m_localWorkGroupsXDimension and
	* m_localWorkGroupsYDimension) taking into account m_bufferNumElement, m_numElementPerLocalWorkgroupThread
	* and m_numThreadPerLocalWorkgroup values (and physical device limits)
	* @param bufferNumElement [in] number of elements to process
	* @return nothing */
	void obtainDispatchWorkGroupCount(uint bufferNumElement)
	{
		uvec3 maxComputeWorkGroupCount     = coreM->getMaxComputeWorkGroupCount();
		uint maxLocalWorkGroupXDimension   = maxComputeWorkGroupCount.x;
		uint maxLocalWorkGroupYDimension   = maxComputeWorkGroupCount.y;
		uint numElementPerLocalWorkgroup   = m_numThreadPerLocalWorkgroup * m_numElementPerLocalWorkgroupThread;
		float numLocalWorkgroupsToDispatch = ceil(float(bufferNumElement) / float(numElementPerLocalWorkgroup));

		if (numLocalWorkgroupsToDispatch <= maxLocalWorkGroupYDimension)
		{
			m_localWorkGroupsXDimension = 1;
			m_localWorkGroupsYDimension = uint(numLocalWorkgroupsToDispatch);
		}
		else
		{
			float integerPart;
			float fractional            = glm::modf(float(numLocalWorkgroupsToDispatch) / float(maxLocalWorkGroupYDimension), integerPart);
			m_localWorkGroupsXDimension = uint(ceil(integerPart + 1.0f));
			m_localWorkGroupsYDimension = uint(ceil(float(numLocalWorkgroupsToDispatch) / float(m_localWorkGroupsXDimension)));
		}
	}

	/** Set in m_computeShaderThreadMapping the compute shader code for mapping the execution threads
	* @return nothing */
	void buildShaderThreadMapping()
	{
		uvec3 minComputeWorkGroupSize = coreM->getMinComputeWorkGroupSize();
		float tempValue               = float(m_numThreadPerLocalWorkgroup) / float(minComputeWorkGroupSize.y);

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
	SET(float, m_voxelSize, VoxelSize)
	GETCOPY_SET(uint, m_numThreadPerLocalWorkgroup, NumThreadPerLocalWorkgroup)
	SET(uint, m_numElementToProcess, NumElementToProcess)

protected:
	string  m_computeShaderThreadMapping;        //!< String with the thread mapping taking into account m_bufferNumElement, m_numElementPerLocalWorkgroupThread and m_numThreadPerLocalWorkgroup values (and physical device limits)
	uint    m_localWorkGroupsXDimension;         //!< Number of dispatched local workgroups in the x dimension
	uint    m_localWorkGroupsYDimension;         //!< Number of dispatched local workgroups in the y dimension
	float   m_voxelSize;                         //!< 3D voxel texture size
	uint    m_numElementPerLocalWorkgroupThread; //!< Number of elements per thread in each local workgroup
	uint    m_numThreadPerLocalWorkgroup;        //!< Number of threads per local workgroup
	int     m_localSizeX;                        //!< Compute shader value for local_size_x
	int     m_localSizeY;                        //!< Compute shader value for local_size_y
	uint    m_numElementToProcess;               //!< Number of static voxels to process
};

/////////////////////////////////////////////////////////////////////////////////////////////

#endif _MATERIALNEIGHBOURLITVOXELINFORMATION_H_
