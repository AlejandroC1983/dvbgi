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

#ifndef _MATERIALBUFFERPREFIXSUM_H_
#define _MATERIALBUFFERPREFIXSUM_H_

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

class MaterialBufferPrefixSum : public Material
{
	friend class MaterialManager;

protected:
	/** Parameter constructor
	* @param [in] shader's name
	* @return nothing */
	MaterialBufferPrefixSum(string &&name) : Material(move(name), move(string("MaterialBufferPrefixSum")))
		, m_prefixSumTotalSize(0)
		, m_numElementBase(0)
		, m_numElementLevel0(0)
		, m_numElementLevel1(0)
		, m_numElementLevel2(0)
		, m_numElementLevel3(0)
		, m_currentStep(0)
		, m_numberStepsReduce(0)
		, m_numberStepsDownSweep(0)
		, m_currentPhase(0)
		, m_indirectionBufferRange(0)
		, m_voxelizationWidth(0)
	{

	}

public:
	/** Loads the shader to be used. This could be substituted
	* by reading from a file, where the shader details would be present
	* @return true if the shader was loaded successfully, and false otherwise */
	bool loadShader()
	{
		size_t sizeTemp;
		void* computeShaderCode = InputOutput::readFile("../data/vulkanshaders/parallelprefixsum.comp", &sizeTemp);

		m_shaderResourceName = "parallelprefixsum";
		m_shader = shaderM->buildShaderC(move(string(m_shaderResourceName)), (const char*)computeShaderCode, m_materialSurfaceType);

		return true;
	}

	/** Exposes the variables that will be modified in the corresponding material data uniform buffer used in the scene,
	* allowing to work from variables from C++ without needing to update the values manually (error-prone)
	* @return nothing */
	void exposeResources()
	{
		exposeStructField(ResourceInternalType::RIT_UNSIGNED_INT, (void*)(&m_prefixSumTotalSize),     move(string("myMaterialData")), move(string("prefixSumTotalSize")));
		exposeStructField(ResourceInternalType::RIT_UNSIGNED_INT, (void*)(&m_numElementBase),         move(string("myMaterialData")), move(string("numElementBase")));
		exposeStructField(ResourceInternalType::RIT_UNSIGNED_INT, (void*)(&m_numElementLevel0),       move(string("myMaterialData")), move(string("numElementLevel0")));
		exposeStructField(ResourceInternalType::RIT_UNSIGNED_INT, (void*)(&m_numElementLevel1),       move(string("myMaterialData")), move(string("numElementLevel1")));
		exposeStructField(ResourceInternalType::RIT_UNSIGNED_INT, (void*)(&m_numElementLevel2),       move(string("myMaterialData")), move(string("numElementLevel2")));
		exposeStructField(ResourceInternalType::RIT_UNSIGNED_INT, (void*)(&m_numElementLevel3),       move(string("myMaterialData")), move(string("numElementLevel3")));
		exposeStructField(ResourceInternalType::RIT_UNSIGNED_INT, (void*)(&m_currentStep),            move(string("myMaterialData")), move(string("currentStep")));
		exposeStructField(ResourceInternalType::RIT_UNSIGNED_INT, (void*)(&m_currentPhase),           move(string("myMaterialData")), move(string("currentPhase")));
		exposeStructField(ResourceInternalType::RIT_UNSIGNED_INT, (void*)(&m_numberStepsDownSweep),   move(string("myMaterialData")), move(string("numberStepsDownSweep")));
		exposeStructField(ResourceInternalType::RIT_UNSIGNED_INT, (void*)(&m_indirectionBufferRange), move(string("myMaterialData")), move(string("indirectionBufferRange")));
		exposeStructField(ResourceInternalType::RIT_UNSIGNED_INT, (void*)(&m_voxelizationWidth),      move(string("myMaterialData")), move(string("voxelizationWidth")));

		assignShaderStorageBuffer(move(string("voxelFirstIndexBuffer")),              move(string("voxelFirstIndexBuffer")),              VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);
		assignShaderStorageBuffer(move(string("prefixSumPlanarBuffer")),              move(string("prefixSumPlanarBuffer")),              VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);
		assignShaderStorageBuffer(move(string("voxelHashedPositionCompactedBuffer")), move(string("voxelHashedPositionCompactedBuffer")), VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);
		assignShaderStorageBuffer(move(string("IndirectionIndexBuffer")),             move(string("IndirectionIndexBuffer")),             VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);
		assignShaderStorageBuffer(move(string("IndirectionRankBuffer")),              move(string("IndirectionRankBuffer")),              VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);

		assignImageToSampler(move(string("staticVoxelIndexTexture")), move(string("staticVoxelIndexTexture")), VK_DESCRIPTOR_TYPE_STORAGE_IMAGE);
	}

	SET(uint, m_prefixSumTotalSize, PrefixSumTotalSize)
	SET(uint, m_numElementBase, NumElementBase)
	SET(uint, m_numElementLevel0, NumElementLevel0)
	SET(uint, m_numElementLevel1, NumElementLevel1)
	SET(uint, m_numElementLevel2, NumElementLevel2)
	SET(uint, m_numElementLevel3, NumElementLevel3)
	SET(uint, m_currentStep, CurrentStep)
	SET(uint, m_numberStepsReduce, NumberStepsReduce)
	SET(uint, m_numberStepsDownSweep, NumberStepsDownSweep)
	SET(uint, m_currentPhase, CurrentPhase)
	SET(uint, m_indirectionBufferRange, IndirectionBufferRange)
	SET(uint, m_voxelizationWidth, VoxelizationWidth)

protected:
	uint    m_prefixSumTotalSize;                 //!< Total buffer size for the parallel prefix sum algorithm
	uint    m_numElementBase;                     //!< Number of elements for the base step of the reduce phase of the algorithm (reading from m_voxelFirstIndexBuffer)
	uint    m_numElementLevel0;                   //!< Number of elements for the step number 0 of the reduce phase of the algorithm
	uint    m_numElementLevel1;                   //!< Number of elements for the step number 1 of the reduce phase of the algorithm
	uint    m_numElementLevel2;                   //!< Number of elements for the step number 2 of the reduce phase of the algorithm
	uint    m_numElementLevel3;                   //!< Number of elements for the step number 3 of the reduce phase of the algorithm
	uint    m_currentStep;                        //!< Index with the current step being done in the algorithm. At level 0, the number of elements to process is given by prefixSumNumElementBase, and from level one, by prefixSumNumElementLeveli
	uint    m_numberStepsReduce;                  //!< Number of steps of the algorithm (since the number of elements to apply prefix sum at bufferHistogramBuffer can vary from one execution to another)
	uint    m_numberStepsDownSweep;               //!< Number of steps of the dow sweep phase of the algorithm (since the number of elements to apply prefix sum at m_voxelFirstIndexBuffer can vary from one execution to another)
	uint    m_currentPhase;                       //!< 0 means reduction step, 1 sweep down, 2 write final compacted buffer
	uint    m_indirectionBufferRange;             //!< Number of elements covered by each index in the m_indirectionIndexBuffer buffer
	uint    m_voxelizationWidth;                  //!< Width of the voxelization volume
};

/////////////////////////////////////////////////////////////////////////////////////////////

#endif _MATERIALBUFFERPREFIXSUM_H_
