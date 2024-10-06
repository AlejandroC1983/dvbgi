/*
Copyright 2017 Alejandro Cosin

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

// GLOBAL INCLUDES

// PROJECT INCLUDES
#include "../../include/util/bufferverificationhelper.h"
#include "../../include/buffer/buffer.h"
#include "../../include/buffer/buffermanager.h"
#include "../../include/core/coremanager.h"
#include "../../include/rastertechnique/staticscenevoxelizationtechnique.h"
#include "../../include/rastertechnique/bufferprefixsumtechnique.h"
#include "../../include/util/vulkanstructinitializer.h"
#include "../../include/rastertechnique/dynamicvoxelcopytobuffertechnique.h"

// NAMESPACE

// DEFINES

// STATIC MEMBER INITIALIZATION
uint BufferVerificationHelper::m_accumulatedReductionLevelBase = 0;
uint BufferVerificationHelper::m_accumulatedReductionLevel0    = 0;
uint BufferVerificationHelper::m_accumulatedReductionLevel1    = 0;
uint BufferVerificationHelper::m_accumulatedReductionLevel2    = 0;
uint BufferVerificationHelper::m_accumulatedReductionLevel3    = 0;
bool BufferVerificationHelper::m_outputAllInformationConsole   = true;
bool BufferVerificationHelper::m_outputAllInformationFile      = true;

/////////////////////////////////////////////////////////////////////////////////////////////

bool BufferVerificationHelper::isVoxelOccupied(uint indexHashed, uint* pVectorVoxelOccupiedBuffer)
{
	float integerPart;
	float indexDecimalFloat = float(indexHashed) / 32.0f;
	float fractional        = glm::modf(indexDecimalFloat, integerPart);
	uint index              = uint(integerPart);
	uint bit                = uint(fractional * 32.0);
	uint value              = pVectorVoxelOccupiedBuffer[index];
	bool result             = ((value & 1 << bit) > 0);
	return result;
}

/////////////////////////////////////////////////////////////////////////////////////////////

void BufferVerificationHelper::outputVoxelOccupiedBuffer()
{
	ofstream outFile;
	if (m_outputAllInformationFile)
	{
		outFile.open("verify.txt", ofstream::app);
	}

	if (m_outputAllInformationConsole)
	{
		cout << endl;
		cout << "*************************************" << endl;
		cout << "Voxel occupied buffer content" << endl;
	}
	if (m_outputAllInformationFile)
	{
		outFile << endl;
		outFile << "*************************************" << endl;
		outFile << "Voxel occupied buffer content" << endl;
	}

	vectorUint8 vectorVoxelOccupied;
	Buffer* voxelOccupiedBuffer = bufferM->getElement(move(string("voxelOccupiedBuffer")));
	voxelOccupiedBuffer->getContentCopy(vectorVoxelOccupied);
	uint numVoxelOccupied       = uint(voxelOccupiedBuffer->getDataSize()) / sizeof(uint);
	uint* pVoxelOccupied        = (uint*)(vectorVoxelOccupied.data());
	uint numFlagElements        = 0;

	forI(numVoxelOccupied)
	{
		uint vectorData = pVoxelOccupied[i];

		if (vectorData > 0)
		{
			if (m_outputAllInformationConsole)
			{
				cout << "Occupied data for index " << i << endl;
			}
			if (m_outputAllInformationFile)
			{
				outFile << "Occupied data for index " << i << endl;
			}
			forJ(32)
			{
				uint value    = 1 << j;
				uint bitIndex = i * 32 + j;
				bool result   = ((vectorData & value) > 0);

				if (m_outputAllInformationConsole)
				{
					cout << "\t bit number " << j << " has occupation flag " << result << endl;
				}
				if (m_outputAllInformationFile)
				{
					outFile << "\t bit number " << j << " has occupation flag " << result << endl;
				}

				if (result)
				{
					numFlagElements++;
				}
			}
		}
	}

	BufferPrefixSumTechnique* technique = static_cast<BufferPrefixSumTechnique*>(gpuPipelineM->getRasterTechniqueByName(move(string("BufferPrefixSumTechnique"))));
	uint firstIndexOccupiedElement = technique->getFirstIndexOccupiedElement();

	cout << ">>>>> Number of voxel occupied from occupied voxel is " << numFlagElements << endl;
	cout << ">>>>> According to prefix sum, the number of occupied voxels is " << firstIndexOccupiedElement << endl;
	if (m_outputAllInformationFile)
	{
		outFile << ">>>>> Number of voxel occupied from occupied voxel is " << numFlagElements << endl;
		outFile << ">>>>> According to prefix sum, the number of occupied voxels is " << firstIndexOccupiedElement << endl;
	}

	assert(numFlagElements == firstIndexOccupiedElement);

	if (m_outputAllInformationConsole)
	{
		cout << "*************************************" << endl;
		cout << endl;
	}
	if (m_outputAllInformationFile)
	{
		outFile << "*************************************" << endl;
		outFile << endl;
	}

	if (m_outputAllInformationFile)
	{
		outFile.close();
	}
}

/////////////////////////////////////////////////////////////////////////////////////////////

void BufferVerificationHelper::verifyFragmentData()
{

}

/////////////////////////////////////////////////////////////////////////////////////////////

bool BufferVerificationHelper::getVoxelOccupied(uint value, uint bit)
{
	uint bitShift = (1 << bit);
	bool result = ((value & (1 << bit)) > 0);
	return result;
}

/////////////////////////////////////////////////////////////////////////////////////////////

void BufferVerificationHelper::verifyVoxelOccupiedBuffer()
{
	ofstream outFile;
	if (m_outputAllInformationFile)
	{
		outFile.open("verify.txt", ofstream::trunc);
	}

	if (m_outputAllInformationConsole)
	{
		cout << endl;
		cout << "*************************************" << endl;
		cout << "Fragment data" << endl;
	}
	if (m_outputAllInformationFile)
	{
		outFile << endl;
		outFile << "*************************************" << endl;
		outFile << "Fragment data" << endl;
	}

	vectorUint8 vectorVoxelOccupied;
	Buffer* voxelOccupiedBuffer = bufferM->getElement(move(string("voxelOccupiedBuffer")));
	voxelOccupiedBuffer->getContentCopy(vectorVoxelOccupied);
	uint numVoxelOccupied       = uint(voxelOccupiedBuffer->getDataSize()) / sizeof(uint);
	uint* pVoxelOccupied        = (uint*)(vectorVoxelOccupied.data());

	vectorUint8 vectorVoxelFirstIndex;
	Buffer* voxelFirstIndexBuffer = bufferM->getElement(move(string("voxelFirstIndexBuffer")));
	voxelFirstIndexBuffer->getContentCopy(vectorVoxelFirstIndex);
	uint numVoxelFirstIndex       = uint(voxelFirstIndexBuffer->getDataSize()) / sizeof(uint);
	uint* pVoxelFirstIndex        = (uint*)(vectorVoxelFirstIndex.data());

	bool anyError                       = false;
	uint numShouldBeOccupiedAndAreEmpty = 0;

	forI(numVoxelFirstIndex)
	{
		uint hashedValue = pVoxelFirstIndex[i];

		if (hashedValue != 4294967295)
		{
			float integerPart;
			float indexDecimalFloat = float(i) / 32.0f;
			float fractional        = glm::modf(indexDecimalFloat, integerPart);
			uint index              = uint(integerPart);
			uint bit                = uint(fractional * 32.0);
			bool result             = getVoxelOccupied(pVoxelOccupied[index], bit);

			if (!result)
			{
				if (m_outputAllInformationConsole)
				{
					cout << "ERROR: occupied flag not set at index " << i << " in BufferVerificationHelper::verifyVoxelOccupiedBuffer" << endl;
				}
				if (m_outputAllInformationFile)
				{
					outFile << "ERROR: occupied flag not set at index " << i << " in BufferVerificationHelper::verifyVoxelOccupiedBuffer" << endl;
				}
				numShouldBeOccupiedAndAreEmpty++;
				anyError = true;
			}
		}
	}

	cout << ">>>>> The number of elements that are empty and should be occupied is " << numShouldBeOccupiedAndAreEmpty << endl;
	if (m_outputAllInformationFile)
	{
		outFile << ">>>>> The number of elements that are empty and should be occupied is " << numShouldBeOccupiedAndAreEmpty << endl;
	}
	assert(anyError == false);

	if (m_outputAllInformationConsole)
	{
		cout << "*************************************" << endl;
		cout << endl;
	}
	if (m_outputAllInformationFile)
	{
		outFile << "*************************************" << endl;
		outFile << endl;
	}

	if (m_outputAllInformationFile)
	{
		outFile.close();
	}
}

/////////////////////////////////////////////////////////////////////////////////////////////

void BufferVerificationHelper::verifyPrefixSumData()
{
	ofstream outFile;
	if (m_outputAllInformationFile)
	{
		outFile.open("verify.txt", ofstream::app);
	}

	Buffer* prefixSumPlanarBuffer = bufferM->getElement(move(string("prefixSumPlanarBuffer")));
	vectorUint8 vectorPrefixSumPlanarBuffer;
	prefixSumPlanarBuffer->getContentCopy(vectorPrefixSumPlanarBuffer);
	uint numPrefixSumPlanar       = uint(prefixSumPlanarBuffer->getDataSize()) / sizeof(uint);
	uint* pPrefixSumPlanar        = (uint*)(vectorPrefixSumPlanarBuffer.data());

	m_accumulatedReductionLevelBase = 0;
	m_accumulatedReductionLevel0    = 0;
	m_accumulatedReductionLevel1    = 0;
	m_accumulatedReductionLevel2    = 0;
	m_accumulatedReductionLevel3    = 0;

	BufferPrefixSumTechnique* technique = static_cast<BufferPrefixSumTechnique*>(gpuPipelineM->getRasterTechniqueByName(move(string("BufferPrefixSumTechnique"))));
	vectorUint vectorPrefixSumNumElement = technique->getVectorPrefixSumNumElement();

	forI(vectorPrefixSumNumElement[0])
	{
		m_accumulatedReductionLevelBase += pPrefixSumPlanar[i];
	}

	forI(vectorPrefixSumNumElement[1])
	{
		m_accumulatedReductionLevel0 += pPrefixSumPlanar[i + vectorPrefixSumNumElement[0]];
	}

	forI(vectorPrefixSumNumElement[2])
	{
		m_accumulatedReductionLevel1 += pPrefixSumPlanar[i + vectorPrefixSumNumElement[0] + vectorPrefixSumNumElement[1]];
	}

	forI(vectorPrefixSumNumElement[3])
	{
		m_accumulatedReductionLevel2 += pPrefixSumPlanar[i + vectorPrefixSumNumElement[0] + vectorPrefixSumNumElement[1] + vectorPrefixSumNumElement[2]];
	}

	forI(vectorPrefixSumNumElement[4])
	{
		m_accumulatedReductionLevel3 += pPrefixSumPlanar[i + vectorPrefixSumNumElement[0] + vectorPrefixSumNumElement[1] + vectorPrefixSumNumElement[2] + vectorPrefixSumNumElement[3]];
	}

	if (m_outputAllInformationConsole)
	{
		cout << endl;
		cout << "*************************************" << endl;
		cout << "Prefix sum data" << endl;
	}
	if (m_outputAllInformationFile)
	{
		outFile << endl;
		outFile << "*************************************" << endl;
		outFile << "Prefix sum data" << endl;
	}

	cout << ">>>>> Accumulated per-level verification value for accumulatedValueLevelBase is " << m_accumulatedReductionLevelBase << endl;
	cout << ">>>>> Accumulated per-level verification value for accumulatedValueLevel0 is " << m_accumulatedReductionLevel0 << endl;
	cout << ">>>>> Accumulated per-level verification value for accumulatedValueLevel1 is " << m_accumulatedReductionLevel1 << endl;
	cout << ">>>>> Accumulated per-level verification value for accumulatedValueLevel2 is " << m_accumulatedReductionLevel2 << endl;
	cout << ">>>>> Accumulated per-level verification value for accumulatedValueLevel3 is " << m_accumulatedReductionLevel3 << endl;
	if (m_outputAllInformationFile)
	{
		outFile << ">>>>> Accumulated per-level verification value for accumulatedValueLevelBase is " << m_accumulatedReductionLevelBase << endl;
		outFile << ">>>>> Accumulated per-level verification value for accumulatedValueLevel0 is " << m_accumulatedReductionLevel0 << endl;
		outFile << ">>>>> Accumulated per-level verification value for accumulatedValueLevel1 is " << m_accumulatedReductionLevel1 << endl;
		outFile << ">>>>> Accumulated per-level verification value for accumulatedValueLevel2 is " << m_accumulatedReductionLevel2 << endl;
		outFile << ">>>>> Accumulated per-level verification value for accumulatedValueLevel3 is " << m_accumulatedReductionLevel3 << endl;
	}

	vectorUint vectorAccumulated =
	{
		m_accumulatedReductionLevelBase,
		m_accumulatedReductionLevel0,
		m_accumulatedReductionLevel1,
		m_accumulatedReductionLevel2,
		m_accumulatedReductionLevel3
	};

	sort(vectorAccumulated.begin(), vectorAccumulated.end());
	uint maxIndex = uint(vectorAccumulated.size());
	uint initialIndex = 0;

	forI(maxIndex)
	{
		if (vectorAccumulated[i] > 0)
		{
			initialIndex = i;
			break;
		}
	}

	bool anyValueDifferent = false;
	forI(maxIndex - initialIndex - 1)
	{
		if (vectorAccumulated[i + initialIndex] != vectorAccumulated[i + initialIndex + 1])
		{
			anyValueDifferent = true;
			break;
		}
	}

	assert(anyValueDifferent == false);

	if (m_outputAllInformationConsole)
	{
		cout << "*************************************" << endl;
		cout << endl;
	}
	if (m_outputAllInformationFile)
	{
		outFile << "*************************************" << endl;
		outFile << endl;
	}

	if (m_outputAllInformationFile)
	{
		outFile.close();
	}
}

/////////////////////////////////////////////////////////////////////////////////////////////

uint BufferVerificationHelper::getHashedIndex(uvec3 texcoord, uint voxelSize)
{
	return texcoord.x * voxelSize * voxelSize + texcoord.y * voxelSize + texcoord.z;
}

/////////////////////////////////////////////////////////////////////////////////////////////

uvec3 BufferVerificationHelper::unhashValue(uint value, uint voxelizationWidth)
{
	float sizeFloat = float(voxelizationWidth);
	float number    = float(value) / sizeFloat;

	uvec3 result;
	float integerPart;
	float fractional = glm::modf(number, integerPart);
	result.z         = uint(fractional * sizeFloat);
	number          /= sizeFloat;
	fractional       = glm::modf(number, integerPart);
	result.y         = uint(fractional * sizeFloat);
	result.x         = uint(integerPart);

	return result;
}

/////////////////////////////////////////////////////////////////////////////////////////////

void BufferVerificationHelper::verifyVoxelFirstIndexBuffer()
{
	ofstream outFile;
	if (m_outputAllInformationFile)
	{
		outFile.open("verify.txt", ofstream::app);
	}

	vectorUint8 vectorFirstIndex;
	Buffer* voxelFirstIndexBuffer = bufferM->getElement(move(string("voxelFirstIndexBuffer")));
	voxelFirstIndexBuffer->getContentCopy(vectorFirstIndex);
	uint numFirstIndex            = uint(voxelFirstIndexBuffer->getDataSize()) / sizeof(uint);
	uint* pFirstIndex             = (uint*)(vectorFirstIndex.data());

	uint tempValueFirstIndex;
	uint numNonNull = 0;

	forI(numFirstIndex)
	{
		tempValueFirstIndex = pFirstIndex[i];
		if (tempValueFirstIndex != maxValue)
		{
			numNonNull++;
		}
	}

	if (m_outputAllInformationConsole)
	{
		cout << endl;
		cout << "*************************************" << endl;
		cout << "Voxel First Index Buffer verification" << endl;
	}
	if (m_outputAllInformationFile)
	{
		outFile << endl;
		outFile << "*************************************" << endl;
		outFile << "Voxel First Index Buffer verification" << endl;
	}

	cout << ">>>>> Number of non null elements in voxelFirstIndexBuffer is " << numNonNull << endl;
	if (m_outputAllInformationFile)
	{
		outFile << ">>>>> Number of non null elements in voxelFirstIndexBuffer is " << numNonNull << endl;
	}

	bool anyValueDifferent = false;
	if (((m_accumulatedReductionLevelBase > 0) && (m_accumulatedReductionLevelBase != numNonNull)) ||
		((m_accumulatedReductionLevel0    > 0) && (m_accumulatedReductionLevel0 != numNonNull)) ||
		((m_accumulatedReductionLevel1    > 0) && (m_accumulatedReductionLevel1 != numNonNull)) ||
		((m_accumulatedReductionLevel2    > 0) && (m_accumulatedReductionLevel2 != numNonNull)) ||
		((m_accumulatedReductionLevel3    > 0) && (m_accumulatedReductionLevel3 != numNonNull)))
	{
		anyValueDifferent = true;
	}

	assert(anyValueDifferent == false);

	if (m_outputAllInformationConsole)
	{
		cout << "*************************************" << endl;
		cout << endl;
	}
	if (m_outputAllInformationFile)
	{
		outFile << "*************************************" << endl;
		outFile << endl;
	}

	if (m_outputAllInformationFile)
	{
		outFile.close();
	}
}

/////////////////////////////////////////////////////////////////////////////////////////////

void BufferVerificationHelper::verifyVoxelFirstIndexAndVoxelFirstIndexCompacted()
{
	ofstream outFile;
	if (m_outputAllInformationFile)
	{
		outFile.open("verify.txt", ofstream::app);
	}

	vectorUint8 vectorFirstIndex;
	Buffer* voxelFirstIndexBuffer = bufferM->getElement(move(string("voxelFirstIndexBuffer")));
	voxelFirstIndexBuffer->getContentCopy(vectorFirstIndex);
	uint numFirstIndex            = uint(voxelFirstIndexBuffer->getDataSize()) / sizeof(uint);
	uint* pFirstIndex             = (uint*)(vectorFirstIndex.data());

	vectorUint8 vecorCompacted;
	Buffer* voxelFirstIndexCompactedBuffer = bufferM->getElement(move(string("voxelFirstIndexCompactedBuffer")));
	voxelFirstIndexCompactedBuffer->getContentCopy(vecorCompacted);
	uint numCompacted                      = uint(voxelFirstIndexCompactedBuffer->getDataSize()) / sizeof(uint);
	uint* pCompacted                       = (uint*)(vecorCompacted.data());

	uint counter       = 0;
	uint numNonNull    = 0;
	bool anyIndexError = false;

	uint tempCompacted;
	uint tempValueFirstIndex;

	forI(numFirstIndex)
	{
		tempValueFirstIndex = pFirstIndex[i];
		if (tempValueFirstIndex != maxValue)
		{
			tempCompacted = pCompacted[counter];
			if (tempValueFirstIndex != tempCompacted)
			{
				anyIndexError = true;
			}
			counter++;
		}
	}

	if (anyIndexError)
	{
		cout << "ERROR: not compacted buffer value match at BufferPrefixSumTechnique::verifyVoxelFirstIndexAndVoxelFirstIndexCompacted" << endl;
		if (m_outputAllInformationFile)
		{
			outFile << "ERROR: not compacted buffer value match at BufferPrefixSumTechnique::verifyVoxelFirstIndexAndVoxelFirstIndexCompacted" << endl;
		}
	}

	assert(anyIndexError == false);

	if (m_outputAllInformationFile)
	{
		outFile.close();
	}
}

/////////////////////////////////////////////////////////////////////////////////////////////

void BufferVerificationHelper::outputHashedPositionCompactedBufferInfo()
{
	// Unhash the values in voxelHashedPositionCompactedBuffer
	BufferPrefixSumTechnique* technique = static_cast<BufferPrefixSumTechnique*>(gpuPipelineM->getRasterTechniqueByName(move(string("BufferPrefixSumTechnique"))));
	uint voxelizationWidth              = technique->getVoxelizationWidth();

	vectorUint8 vecorHashedCompacted;
	Buffer* voxelHashedPositionCompactedBuffer = bufferM->getElement(move(string("voxelHashedPositionCompactedBuffer")));
	voxelHashedPositionCompactedBuffer->getContentCopy(vecorHashedCompacted);
	uint numHashedCompacted                    = uint(voxelHashedPositionCompactedBuffer->getDataSize()) / sizeof(uint);
	uint* pHashedCompacted                     = (uint*)(vecorHashedCompacted.data());

	uint hashed;
	uvec3 unhashed;

	ofstream outFile;
	outFile.open("verify.txt", ofstream::app);

	if (m_outputAllInformationConsole)
	{
		cout << endl;
		cout << "*************************************" << endl;
		cout << "Hashed position compacted buffer info" << endl;
	}
	if (m_outputAllInformationFile)
	{
		outFile << endl;
		outFile << "*************************************" << endl;
		outFile << "Hashed position compacted buffer info" << endl;
	}

	forI(numHashedCompacted)
	{
		hashed   = pHashedCompacted[i];
		unhashed = unhashValue(hashed, voxelizationWidth);

		if (m_outputAllInformationConsole)
		{
			cout << "Position " << i << " of buffer voxelHashedPositionCompacted has unhashed values (" << unhashed.x << "," << unhashed.y << "," << unhashed.z << ")" << endl;
		}
		if (m_outputAllInformationFile)
		{
			outFile << "Position " << i << " of buffer voxelHashedPositionCompacted has unhashed values (" << unhashed.x << "," << unhashed.y << "," << unhashed.z << ")" << endl;
		}
	}

	if (m_outputAllInformationConsole)
	{
		cout << "*************************************" << endl;
		cout << endl;
	}
	if (m_outputAllInformationFile)
	{
		outFile << "*************************************" << endl;
		outFile << endl;
	}

	if (m_outputAllInformationFile)
	{
		outFile.close();
	}
}

/////////////////////////////////////////////////////////////////////////////////////////////

void BufferVerificationHelper::outputIndirectionBuffer()
{
	ofstream outFile;
	if (m_outputAllInformationFile)
	{
		outFile.open("verify.txt", ofstream::app);
	}

	if (m_outputAllInformationConsole)
	{
		cout << endl;
		cout << "*************************************" << endl;
		cout << "Indirection Buffer output" << endl;
	}
	if (m_outputAllInformationFile)
	{
		outFile << endl;
		outFile << "*************************************" << endl;
		outFile << "Indirection Buffer output" << endl;
	}

	vectorUint8 vectorFirstIndexCompacted;
	Buffer* voxelFirstIndexCompactedBuffer = bufferM->getElement(move(string("voxelFirstIndexCompactedBuffer")));
	voxelFirstIndexCompactedBuffer->getContentCopy(vectorFirstIndexCompacted);
	uint numFirstIndexCompacted            = uint(voxelFirstIndexCompactedBuffer->getDataSize()) / sizeof(uint);
	uint* pFirstIndexCompacted             = (uint*)(vectorFirstIndexCompacted.data());

	vectorUint8 vectorHashedPositionCompactedBuffer;
	Buffer* voxelHashedPositionCompactedBuffer = bufferM->getElement(move(string("voxelHashedPositionCompactedBuffer")));
	voxelHashedPositionCompactedBuffer->getContentCopy(vectorHashedPositionCompactedBuffer);
	uint numHashedPositionCompactedBuffer      = uint(voxelHashedPositionCompactedBuffer->getDataSize()) / sizeof(uint);
	uint* pHashedPositionCompactedBuffer       = (uint*)(vectorHashedPositionCompactedBuffer.data());

	BufferPrefixSumTechnique* technique = static_cast<BufferPrefixSumTechnique*>(gpuPipelineM->getRasterTechniqueByName(move(string("BufferPrefixSumTechnique"))));
	uint voxelizationWidth              = technique->getVoxelizationWidth();

	forI(numHashedPositionCompactedBuffer)
	{
		uvec3 unhashed = unhashValue(pHashedPositionCompactedBuffer[i], voxelizationWidth);
		if (m_outputAllInformationConsole)
		{
			cout << "First index compacted at index " << i << " is pFirstIndexCompacted[i]=" << pFirstIndexCompacted[i] << " and hashed value pHashedPositionCompactedBuffer[i]=" << pHashedPositionCompactedBuffer[i] << " being a hashed position=(" << unhashed.x << "," << unhashed.y << "," << unhashed.z << ")" << endl;
		}
		if (m_outputAllInformationFile)
		{
			outFile << "First index compacted at index " << i << " is pFirstIndexCompacted[i]=" << pFirstIndexCompacted[i] << " and hashed value pHashedPositionCompactedBuffer[i]=" << pHashedPositionCompactedBuffer[i] << " being a hashed position=(" << unhashed.x << "," << unhashed.y << "," << unhashed.z << ")" << endl;
		}
	}

	if (m_outputAllInformationConsole)
	{
		cout << "*************************************" << endl;
		cout << endl;
	}
	if (m_outputAllInformationFile)
	{
		outFile << "*************************************" << endl;
		outFile << endl;
	}

	if (m_outputAllInformationFile)
	{
		outFile.close();
	}
}

/////////////////////////////////////////////////////////////////////////////////////////////

void BufferVerificationHelper::outputIndirectionBufferData()
{
	ofstream outFile;
	if (m_outputAllInformationFile)
	{
		outFile.open("verify.txt", ofstream::app);
	}

	vectorUint8 vectorIndirectionBuffer;
	Buffer* IndirectionIndexBuffer = bufferM->getElement(move(string("IndirectionIndexBuffer")));
	IndirectionIndexBuffer->getContentCopy(vectorIndirectionBuffer);
	uint numIndirectionBuffer      = uint(IndirectionIndexBuffer->getDataSize()) / sizeof(uint);
	uint* pIndirectionBuffer       = (uint*)(vectorIndirectionBuffer.data());

	// Retrieve data from indirection buffer for rank data
	vectorUint8 vectorIndirectionRankBuffer;
	Buffer* IndirectionRankBuffer = bufferM->getElement(move(string("IndirectionRankBuffer")));
	IndirectionRankBuffer->getContentCopy(vectorIndirectionRankBuffer);
	uint numIndirectionRankBuffer = uint(IndirectionRankBuffer->getDataSize()) / sizeof(uint);
	uint* pIndirectionRankBuffer  = (uint*)(vectorIndirectionRankBuffer.data());

	BufferPrefixSumTechnique* technique = static_cast<BufferPrefixSumTechnique*>(gpuPipelineM->getRasterTechniqueByName(move(string("BufferPrefixSumTechnique"))));
	uint voxelizationWidth              = technique->getVoxelizationWidth();

	if (m_outputAllInformationConsole)
	{
		cout << endl;
		cout << "*************************************" << endl;
		cout << "Indirection buffer data" << endl;
	}
	if (m_outputAllInformationFile)
	{
		outFile << endl;
		outFile << "*************************************" << endl;
		outFile << "Indirection buffer data" << endl;
	}

	forI(numIndirectionBuffer)
	{
		float integerPart;
		float sizeFloat  = float(voxelizationWidth);
		float number     = float(i) / sizeFloat;
		float fractional = glm::modf(number, integerPart);

		uvec3 unhashed;
		unhashed.x = uint(integerPart);
		unhashed.y = uint(fractional * sizeFloat);
		if (m_outputAllInformationConsole)
		{
			cout << "index " << i << " at indirection buffer, pIndirectionBuffer[i]=" << pIndirectionBuffer[i] << ", pIndirectionRankBuffer[i]=" << pIndirectionRankBuffer[i] << " being index " << i << " the hashed position=(" << unhashed.x << "," << unhashed.y << "," << unhashed.z << ")" << endl;
		}
		if (m_outputAllInformationFile)
		{
			outFile << "index " << i << " at indirection buffer, pIndirectionBuffer[i]=" << pIndirectionBuffer[i] << ", pIndirectionRankBuffer[i]=" << pIndirectionRankBuffer[i] << " being index " << i << " the hashed position=(" << unhashed.x << "," << unhashed.y << "," << unhashed.z << ")" << endl;
		}
	}

	if (m_outputAllInformationConsole)
	{
		cout << "*************************************" << endl;
		cout << endl;
	}
	if (m_outputAllInformationFile)
	{
		outFile << "*************************************" << endl;
		outFile << endl;
	}

	if (m_outputAllInformationFile)
	{
		outFile.close();
	}
}

/////////////////////////////////////////////////////////////////////////////////////////////

void BufferVerificationHelper::verifyIndirectionBuffers()
{
	ofstream outFile;
	if (m_outputAllInformationFile)
	{
		outFile.open("verify.txt", ofstream::app);
	}

	// Retrieve data from indirection buffer for index data
	vectorUint8 vectorIndirectionBuffer;
	Buffer* IndirectionIndexBuffer = bufferM->getElement(move(string("IndirectionIndexBuffer")));
	IndirectionIndexBuffer->getContentCopy(vectorIndirectionBuffer);
	uint numIndirectionBuffer      = uint(IndirectionIndexBuffer->getDataSize()) / sizeof(uint);
	uint* pIndirectionBuffer       = (uint*)(vectorIndirectionBuffer.data());

	// Retrieve data from indirection buffer for rank data
	vectorUint8 vectorIndirectionRankBuffer;
	Buffer* IndirectionRankBuffer = bufferM->getElement(move(string("IndirectionRankBuffer")));
	IndirectionRankBuffer->getContentCopy(vectorIndirectionRankBuffer);
	uint numIndirectionRankBuffer = uint(IndirectionRankBuffer->getDataSize()) / sizeof(uint);
	uint* pIndirectionRankBuffer  = (uint*)(vectorIndirectionRankBuffer.data());

	// Retrieve data from first index buffer already compacted
	vectorUint8 vectorFirstIndexCompacted;
	Buffer* voxelFirstIndexCompactedBuffer = bufferM->getElement(move(string("voxelFirstIndexCompactedBuffer")));
	voxelFirstIndexCompactedBuffer->getContentCopy(vectorFirstIndexCompacted);
	uint numFirstIndexCompacted            = uint(voxelFirstIndexCompactedBuffer->getDataSize()) / sizeof(uint);
	uint* pFirstIndexCompacted             = (uint*)(vectorFirstIndexCompacted.data());

	// Retrieve data from hashed buffer compacted
	vectorUint8 vectorHashedPositionCompactedBuffer;
	Buffer* voxelHashedPositionCompactedBuffer = bufferM->getElement(move(string("voxelHashedPositionCompactedBuffer")));
	voxelHashedPositionCompactedBuffer->getContentCopy(vectorHashedPositionCompactedBuffer);
	uint numHashedPositionCompactedBuffer      = uint(voxelHashedPositionCompactedBuffer->getDataSize()) / sizeof(uint);
	uint* pHashedPositionCompactedBuffer       = (uint*)(vectorHashedPositionCompactedBuffer.data());

	// Retrieve data from next fragment index buffer
	vectorUint8 vectorNextFragmentIndex;
	Buffer* nextFragmentIndexBuffer = bufferM->getElement(move(string("nextFragmentIndexBuffer")));
	nextFragmentIndexBuffer->getContentCopy(vectorNextFragmentIndex);
	uint numNextFragmentIndex       = uint(nextFragmentIndexBuffer->getDataSize()) / sizeof(uint);
	uint* pNextFragmentIndex        = (uint*)(vectorNextFragmentIndex.data());

	BufferPrefixSumTechnique* technique = static_cast<BufferPrefixSumTechnique*>(gpuPipelineM->getRasterTechniqueByName(move(string("BufferPrefixSumTechnique"))));
	uint voxelizationWidth              = technique->getVoxelizationWidth();

	vectorUint vectorFirstIndexCompactedUsed;

	uint numCompactedBufferAnaylized = 0;
	uint numIndirectionAnalyzed      = 0;
	uint numNextFragmentAnalyzed     = 0;

	if (m_outputAllInformationConsole)
	{
		cout << endl;
		cout << "*************************************" << endl;
		cout << "Indirection Buffers verification" << endl;
	}
	if (m_outputAllInformationFile)
	{
		outFile << endl;
		outFile << "*************************************" << endl;
		outFile << "Indirection Buffers verification" << endl;
	}

	forI(numIndirectionBuffer)
	{
		uint temp = pIndirectionBuffer[i];
		uint rank = pIndirectionRankBuffer[i];

		if (temp != maxValue)
		{
			numIndirectionAnalyzed++;

			if (m_outputAllInformationConsole)
			{
				cout << "In indirection buffer, at index " << i << " there is an entry with value " << temp << " and rank " << rank << endl;
			}
			if (m_outputAllInformationFile)
			{
				outFile << "In indirection buffer, at index " << i << " there is an entry with value " << temp << " and rank " << rank << endl;
			}

			forJ(rank)
			{
				if (m_outputAllInformationConsole)
				{
					cout << "\t Analyzing now the element number " << j << " in the rank" << endl;
				}
				if (m_outputAllInformationFile)
				{
					outFile << "\t Analyzing now the element number " << j << " in the rank" << endl;
				}
				uint firstIndex     = pFirstIndexCompacted[temp + j];
				uint hashedPosition = pHashedPositionCompactedBuffer[temp + j];
				uvec3 unhashed      = unhashValue(hashedPosition, voxelizationWidth);

				int currentIndex = int(pNextFragmentIndex[firstIndex]);
				numNextFragmentAnalyzed++;

				uint counter = 0;
				if (m_outputAllInformationConsole)
				{
					cout << "\t Emitted fragments for hashed position " << hashedPosition << " (" << unhashed.x << ", " << unhashed.y << ", " << unhashed.z << "):" << endl;
					cout << "\t\t Fragment number " << counter << " at index " << currentIndex << endl;
				}
				if (m_outputAllInformationFile)
				{
					outFile << "\t Emitted fragments for hashed position " << hashedPosition << " (" << unhashed.x << ", " << unhashed.y << ", " << unhashed.z << "):" << endl;
					outFile << "\t\t Fragment number " << counter << " at index " << currentIndex << endl;
				}

				assert(find(vectorFirstIndexCompactedUsed.begin(), vectorFirstIndexCompactedUsed.end(), hashedPosition) == vectorFirstIndexCompactedUsed.end());
				vectorFirstIndexCompactedUsed.push_back(hashedPosition);

				while (currentIndex != -1)
				{
					counter++;
					currentIndex = int(pNextFragmentIndex[currentIndex]);
					if (m_outputAllInformationConsole)
					{
						cout << "\t\t Fragment number " << counter << " at index " << currentIndex << endl;
					}
					if (m_outputAllInformationFile)
					{
						outFile << "\t\t Fragment number " << counter << " at index " << currentIndex << endl;
					}
					numNextFragmentAnalyzed++;
				}

				numCompactedBufferAnaylized++;
			}
		}
	}

	uint firstIndexOccupiedElement = technique->getFirstIndexOccupiedElement();
	cout << "The number of elements analyzed for the pFirstIndexCompacted buffer is " << numCompactedBufferAnaylized << ". The number of elements of the pFirstIndexCompacted buffer is " << firstIndexOccupiedElement << endl;
	cout << "The number of non-null entries in indirection buffer is " << numIndirectionAnalyzed << endl;
	cout << "The number of elements analyzed in pNextFragmentIndex is " << numNextFragmentAnalyzed << endl;
	if (m_outputAllInformationFile)
	{
		outFile << "The number of elements analyzed for the pFirstIndexCompacted buffer is " << numCompactedBufferAnaylized << ". The number of elements of the pFirstIndexCompacted buffer is " << firstIndexOccupiedElement << endl;
		outFile << "The number of non-null entries in indirection buffer is " << numIndirectionAnalyzed << endl;
		outFile << "The number of elements analyzed in pNextFragmentIndex is " << numNextFragmentAnalyzed << endl;
	}

	StaticSceneVoxelizationTechnique* techniqueVoxelization = static_cast<StaticSceneVoxelizationTechnique*>(gpuPipelineM->getRasterTechniqueByName(move(string("StaticSceneVoxelizationTechnique"))));

	assert(firstIndexOccupiedElement == numCompactedBufferAnaylized);

	if (m_outputAllInformationConsole)
	{
		cout << "*************************************" << endl;
		cout << endl;
	}
	if (m_outputAllInformationFile)
	{
		outFile << "*************************************" << endl;
		outFile << endl;
	}

	if (m_outputAllInformationFile)
	{
		outFile.close();
	}
}

/////////////////////////////////////////////////////////////////////////////////////////////

void BufferVerificationHelper::verifyVoxelizationProcessData()
{
	verifyFragmentData();

	if (m_outputAllInformationConsole)
	{
		outputVoxelOccupiedBuffer();
	}

	verifyVoxelFirstIndexBuffer();
	verifyVoxelFirstIndexAndVoxelFirstIndexCompacted();

	static bool doStoreInFile = true;
	if (doStoreInFile)
	{
		outputHashedPositionCompactedBufferInfo();
	}

	if (m_outputAllInformationConsole)
	{
		outputIndirectionBuffer();
		outputIndirectionBufferData();
	}

	verifyIndirectionBuffers();
}

/////////////////////////////////////////////////////////////////////////////////////////////

void BufferVerificationHelper::verifySummedAreaResult(Texture* initialTexture, Texture* finalTexture)
{
	vectorVectorFloat vectorInitial;
	vectorVectorFloat vectorResult;
	vectorVectorVec4 temp;
	VulkanStructInitializer::getTextureData(initialTexture, VK_FORMAT_R32_SFLOAT, temp, vectorInitial);
	VulkanStructInitializer::getTextureData(finalTexture,   VK_FORMAT_R32_SFLOAT, temp, vectorResult);

	vectorVectorFloat vectorInitialSummedArea = computeSummedArea(vectorInitial, -1);
	bool result                               = verifyAreEqual(vectorResult, vectorInitialSummedArea, 0.001f);

	assert(result == true);
}

/////////////////////////////////////////////////////////////////////////////////////////////

vectorVectorFloat BufferVerificationHelper::computeSummedArea(vectorVectorFloat& vecData, int maxStep)
{
	// For now, square areas are assumed
	uint size = uint(vecData.size());

	// Offset data is the same for rows and columns step since square areas are assumed
	vectorUint vectorOffset;
	uint perDimensionStep = uint(ceil(log2(float(size))));
	vectorOffset.resize(perDimensionStep);

	forI(perDimensionStep)
	{
		vectorOffset[i]   = uint(pow(2.0, float(i)));
	}

	vectorVectorFloat vecSource      = vecData;
	vectorVectorFloat vecDestination = vecData;

	uint offset;
	uint row;
	uint column;
	float valueSource;
	float valueDestination;

	uint finalMaxStep = (maxStep == -1) ? perDimensionStep : glm::min(uint(maxStep), perDimensionStep);

	// Rows first
	forI(finalMaxStep)
	{
		offset = vectorOffset[i];
		forJ(size - offset)
		{
			row = j;
			forK(size)
			{
				column                               = k;
				valueSource                          = vecSource     [row         ][column];
				valueDestination                     = vecDestination[row + offset][column];
				vecDestination[row + offset][column] = valueSource + valueDestination;
			}
		}

		vecSource = vecDestination;
	}

	// Columns
	forI(finalMaxStep)
	{
		offset = vectorOffset[i];
		forJ(size - offset)
		{
			column = j;
			forK(size)
			{
				row                                  = k;
				valueSource                          = vecSource     [row][column         ];
				valueDestination                     = vecDestination[row][column + offset];
				vecDestination[row][column + offset] = valueSource + valueDestination;
			}
		}

		vecSource = vecDestination;
	}

	return vecDestination;
}

/////////////////////////////////////////////////////////////////////////////////////////////

bool BufferVerificationHelper::verifyAreEqual(const vectorVectorFloat& vecData0, const vectorVectorFloat& vecData1, float eps)
{
	uint sizeData0 = uint(vecData0.size());
	uint sizeData1 = uint(vecData1.size());

	if (sizeData0 != sizeData1)
	{
		cout << "ERROR: different size parameters in SummedAreaTextureTechnique::verifyAreEqual" << endl;
		return false;
	}

	forI(sizeData0)
	{
		if (vecData0[i].size() != vecData1[i].size())
		{
			cout << "ERROR: different size parameters in SummedAreaTextureTechnique::verifyAreEqual" << endl;
			return false;
		}
	}

	uint numDifferent = 0;
	uint numElement;
	forI(sizeData0)
	{
		numElement = uint(vecData0[i].size());

		forJ(numElement)
		{
			if (abs(vecData0[i][j] - vecData1[i][j]) > eps)
			{
				float data0 = vecData0[i][j];
				float data1 = vecData1[i][j];
				float dif = abs(data0 - data1);
				numDifferent++;
			}
		}
	}

	if (numDifferent > 0)
	{
		cout << "ERROR: wrong values in summed-area texture results for threshold " << eps << ", the number of elements with error above the threshold is " << numDifferent << endl;
		return false;
	}

	return true;
}

/////////////////////////////////////////////////////////////////////////////////////////////

bool BufferVerificationHelper::verifyAddUpTextureValue(Texture* texture, float value, float threshold)
{
	vectorVectorFloat vectorData;
	vectorVectorVec4 temp;
	VulkanStructInitializer::getTextureData(texture, VK_FORMAT_R32_SFLOAT, temp, vectorData);

	const uint maxWidth = texture->getWidth();
	const uint maxHeigt = texture->getHeight();

	double accumulated = 0.0;
	forI(maxWidth)
	{
		forJ(maxHeigt)
		{
			accumulated += double(vectorData[i][j]);

			if (double(vectorData[i][j]) < 0.0)
			{
				int a = 0;
			}
		}
	}

	if (abs(accumulated - double(value)) < double(threshold))
	{
		return true;
	}

	return false;
}

/////////////////////////////////////////////////////////////////////////////////////////////

vec3 BufferVerificationHelper::voxelSpaceToWorld(uvec3 coordinates, vec3 voxelSize, vec3 sceneExtent, vec3 sceneMin)
{
	vec3 result = vec3(float(coordinates.x), float(coordinates.y), float(coordinates.z));
	result     /= vec3(voxelSize.x, voxelSize.y, voxelSize.z);
	result     *= sceneExtent;
	result     += sceneMin;
	result     += ((vec3(sceneExtent) / vec3(voxelSize.x, voxelSize.y, voxelSize.z)) * 0.5f);
	return result;
}

/////////////////////////////////////////////////////////////////////////////////////////////

uvec3 BufferVerificationHelper::worldToVoxelSpace(vec3 coordinates, vec3 voxelSize, vec3 sceneExtent, vec3 sceneMin)
{
	vec3 result = coordinates;
	result     -= sceneMin;
	result     /= sceneExtent;
	result     *= vec3(voxelSize);

	return uvec3(uint(result.x), uint(result.y), uint(result.z));
}

/////////////////////////////////////////////////////////////////////////////////////////////

bool BufferVerificationHelper::intervalIntersection(int minX0, int maxX0, int minX1, int maxX1)
{
	if ((((minX0 >= minX1) && (minX0 <= maxX1)) || ((maxX0 >= minX1) && (maxX0 <= maxX1))) ||
		(((minX1 >= minX0) && (minX1 <= maxX0)) || ((maxX1 >= minX0) && (maxX1 <= maxX0))))
	{
		return true;
	}

	return false;
}

/////////////////////////////////////////////////////////////////////////////////////////////

void BufferVerificationHelper::verifyVoxelVisibilityIndices()
{
	vectorUint8 vectorVoxelVisibilityDebugBuffer;
	Buffer* voxelVisibilityDebugBuffer = bufferM->getElement(move(string("voxelVisibilityDebugBuffer")));
	voxelVisibilityDebugBuffer->getContentCopy(vectorVoxelVisibilityDebugBuffer);
	uint numVoxelVisibilityDebugBuffer = uint(voxelVisibilityDebugBuffer->getDataSize()) / sizeof(uint);
	uint* pVoxelVisibilityDebugBuffer = (uint*)(vectorVoxelVisibilityDebugBuffer.data());

	uint localWorkGroupIndex = 0;
	uint numberThread = numVoxelVisibilityDebugBuffer / 5;
	bool bufferVerificationFailed = false;

	for (uint i = 0; i < numberThread; ++i)
	{
		uint idxBuffer                     = pVoxelVisibilityDebugBuffer[5 * i + 0];
		uint startIndexBuffer              = pVoxelVisibilityDebugBuffer[5 * i + 1];
		uint voxelIndexBuffer              = pVoxelVisibilityDebugBuffer[5 * i + 2];
		uint faceIndexBuffer               = pVoxelVisibilityDebugBuffer[5 * i + 3];
		uint g_sharedNumVisibleVoxelBuffer = pVoxelVisibilityDebugBuffer[5 * i + 4];

		float integerPart;
		float voxelIndexFloat = float(idxBuffer) / (128.0f * 6.0f);
		float fractionalPart = glm::modf(voxelIndexFloat, integerPart);
		int voxelIndex = int(integerPart);
		voxelIndexFloat = float(localWorkGroupIndex) / 6.0f;
		fractionalPart = glm::modf(voxelIndexFloat, integerPart);
		int faceIndex = int(round(fractionalPart * 6.0));
		int startIndex = (voxelIndex * int(128) * 6) + faceIndex * int(128);

		if ((startIndexBuffer != startIndex) || (voxelIndexBuffer != voxelIndex) || (faceIndexBuffer != faceIndex))
		{
			bufferVerificationFailed = true;
		}

		localWorkGroupIndex++;
		localWorkGroupIndex = localWorkGroupIndex % 128;
	}

	if (bufferVerificationFailed)
	{
		cout << "ERROR: buffer verification failed at BufferVerificationHelper::verifyVoxelVisibilityIndices()" << endl;
	}
}

/////////////////////////////////////////////////////////////////////////////////////////////

void BufferVerificationHelper::verifyVoxelVisibilityFirstIndexBuffer()
{
	return;

	vectorUint8 vectorVoxelVisibilityFirstIndexBuffer;
	Buffer* voxelVisibilityFirstIndexBuffer = bufferM->getElement(move(string("voxelVisibilityFirstIndexBuffer")));
	voxelVisibilityFirstIndexBuffer->getContentCopy(vectorVoxelVisibilityFirstIndexBuffer);
	uint numVoxelVisibilityFirstIndexBuffer = uint(voxelVisibilityFirstIndexBuffer->getDataSize()) / sizeof(uint);
	uint* pVoxelVisibilityFirstIndexBuffer = (uint*)(vectorVoxelVisibilityFirstIndexBuffer.data());

	vectorUint8 vectorVoxelVisibilityCompactedBuffer;
	Buffer* voxelVisibilityCompactedBuffer = bufferM->getElement(move(string("voxelVisibilityCompactedBuffer")));
	voxelVisibilityCompactedBuffer->getContentCopy(vectorVoxelVisibilityCompactedBuffer);
	uint numVoxelVisibilityCompactedBuffer = uint(voxelVisibilityCompactedBuffer->getDataSize()) / sizeof(uint);
	uint* pVoxelVisibilityCompactedBuffer = (uint*)(vectorVoxelVisibilityCompactedBuffer.data());

	vectorUint8 vectorVoxelVisibility4BytesBuffer;
	Buffer* voxelVisibility4BytesBuffer = bufferM->getElement(move(string("voxelVisibility4BytesBuffer"))); // TODO: This buffer changed, adapt to new information
	voxelVisibility4BytesBuffer->getContentCopy(vectorVoxelVisibility4BytesBuffer);
	uint numVoxelVisibility4BytesBuffer = uint(voxelVisibility4BytesBuffer->getDataSize()) / sizeof(uint);
	uint* pVoxelVisibility4BytesBuffer = (uint*)(vectorVoxelVisibility4BytesBuffer.data());

	vectorUint8 vectorVoxelVisibilityNumberBuffer;
	Buffer* voxelVisibilityNumberBuffer = bufferM->getElement(move(string("voxelVisibilityNumberBuffer")));
	voxelVisibilityNumberBuffer->getContentCopy(vectorVoxelVisibilityNumberBuffer);
	uint numVoxelVisibilityNumberBuffer = uint(voxelVisibilityNumberBuffer->getDataSize()) / sizeof(uint);
	uint* pVoxelVisibilityNumberBuffer = (uint*)(vectorVoxelVisibilityNumberBuffer.data());

	// Now do a verification of voxelVisibilityFirstIndexBuffer
	for (uint i = 0; i < numVoxelVisibilityNumberBuffer; ++i)
	{
		uint numVisible = pVoxelVisibilityNumberBuffer[i];

		if (numVisible > 0)
		{
			uint firstIndex = pVoxelVisibilityFirstIndexBuffer[i];
			vectorUint vectorFromCompacted;
			for (uint j = firstIndex; j < firstIndex + numVisible; ++j)
			{
				vectorFromCompacted.push_back(pVoxelVisibilityCompactedBuffer[j]);
			}

			vectorUint vectorFromNotCompacted;
			for (uint j = i * 128; j < i * 128 + 128; ++j)
			{
				if (pVoxelVisibility4BytesBuffer[j] != 4294967295)
				{
					vectorFromNotCompacted.push_back(pVoxelVisibility4BytesBuffer[j]);
				}
			}

			if (vectorFromCompacted.size() != vectorFromNotCompacted.size())
			{
				cout << "ERROR: There is not the same size in compacted and not compacted vectors for index" << i << endl;
				cout << "Start index for vector not compacted is " << i * 128 << ", end index " << i * 128 + 128 << endl;
				cout << "Start index for vector compacted is " << firstIndex << ", end index is " << firstIndex + numVisible << endl;
				assert(1);
			}

			for (uint j = 0; j < vectorFromCompacted.size(); ++j)
			{
				if (vectorFromCompacted[j] != vectorFromNotCompacted[j])
				{
					cout << "ERROR: There is not the same value in compacted and not compacted for index" << i << endl;
					cout << "Start index for vector not compacted is " << i * 128 << ", end index " << i * 128 + 128 << endl;
					cout << "Start index for vector compacted is " << firstIndex << ", end index is " << firstIndex + numVisible << endl;
					assert(1);
				}
			}
		}
	}

	cout << "The verification of voxelVisibilityFirstIndexBuffer finished with no errors detected" << endl;
}

/////////////////////////////////////////////////////////////////////////////////////////////

void BufferVerificationHelper::verifyCameraVisibleAndLightBounceVoxels()
{

}

/////////////////////////////////////////////////////////////////////////////////////////////

bool BufferVerificationHelper::printVisibilityInfoForVoxelCoordinates(uvec3 voxelCoordinates, uint voxelizationSize)
{
	vectorUint8 vecorHashedCompacted;
	Buffer* voxelHashedPositionCompactedBuffer = bufferM->getElement(move(string("voxelHashedPositionCompactedBuffer")));
	voxelHashedPositionCompactedBuffer->getContentCopy(vecorHashedCompacted);
	uint numHashedCompacted = uint(voxelHashedPositionCompactedBuffer->getDataSize()) / sizeof(uint);
	uint* pHashedCompacted  = (uint*)(vecorHashedCompacted.data());
	bool foundElement       = false;
	int indexUnhashed       = -1;

	forI(numHashedCompacted)
	{
		uvec3 unhashed = unhashValue(pHashedCompacted[i], voxelizationSize);

		if (unhashed == voxelCoordinates)
		{
			cout << "The index of the voxel with coordinates (" << voxelCoordinates.x << ", " << voxelCoordinates.y << ", " << voxelCoordinates.z << ") in voxelHashedPositionCompactedBuffer is " << i << endl;
			indexUnhashed = i;
			break;
		}
	}

	if (indexUnhashed == -1)
	{
		return false;
	}

	uint voxelFace = 5;

	vectorUint8 vecorVoxelVisibilityNumberBuffer;
	Buffer* voxelVisibilityNumberBuffer = bufferM->getElement(move(string("voxelVisibilityNumberBuffer")));
	voxelVisibilityNumberBuffer->getContentCopy(vecorVoxelVisibilityNumberBuffer);
	uint numVoxelVisibilityNumberBuffer = uint(voxelVisibilityNumberBuffer->getDataSize()) / sizeof(uint);
	uint* pVoxelVisibilityNumberBuffer  = (uint*)(vecorVoxelVisibilityNumberBuffer.data());

	vectorUint8 vecorVoxelVisibilityDynamicNumber;
	Buffer* voxelVisibilityDynamicNumber = bufferM->getElement(move(string("voxelVisibilityDynamicNumberBuffer")));
	voxelVisibilityDynamicNumber->getContentCopy(vecorVoxelVisibilityDynamicNumber);
	uint numVoxelVisibilityDynamicNumber = uint(voxelVisibilityDynamicNumber->getDataSize()) / sizeof(uint);
	uint* pVoxelVisibilityDynamicNumber  = (uint*)(vecorVoxelVisibilityDynamicNumber.data());

	uint numberVisibleVoxelPosY        = pVoxelVisibilityNumberBuffer[indexUnhashed * 6 + voxelFace];
	uint numberVisibleVoxelDynamicPosY = pVoxelVisibilityDynamicNumber[indexUnhashed * 6 + voxelFace];

	vectorUint8 vecorVoxelVisibilityFirstIndexBuffer;
	Buffer* voxelVisibilityFirstIndexBuffer = bufferM->getElement(move(string("voxelVisibilityFirstIndexBuffer")));
	voxelVisibilityFirstIndexBuffer->getContentCopy(vecorVoxelVisibilityFirstIndexBuffer);
	uint numVoxelVisibilityFirstIndexBuffer = uint(voxelVisibilityFirstIndexBuffer->getDataSize()) / sizeof(uint);
	uint* pVoxelVisibilityFirstIndexBuffer  = (uint*)(vecorVoxelVisibilityFirstIndexBuffer.data());

	uint visibleFirstIndex = pVoxelVisibilityFirstIndexBuffer[indexUnhashed * 6 + voxelFace]; // +z voxel face

	vectorUint8 vecorVoxelVisibilityCompactedBuffer;
	Buffer* voxelVisibilityCompactedBuffer      = bufferM->getElement(move(string("voxelVisibilityCompactedBuffer")));
	voxelVisibilityCompactedBuffer->getContentCopy(vecorVoxelVisibilityCompactedBuffer);
	uint numVecorVoxelVisibilityCompactedBuffer = uint(voxelVisibilityCompactedBuffer->getDataSize()) / sizeof(uint);
	uint* pVoxelVisibilityCompactedBuffer       = (uint*)(vecorVoxelVisibilityCompactedBuffer.data());

	vectorUint vectorElement(numberVisibleVoxelPosY + numberVisibleVoxelDynamicPosY);

	uint offset = visibleFirstIndex;
	forI(numberVisibleVoxelPosY + numberVisibleVoxelDynamicPosY)
	{
		vectorElement[i] = pVoxelVisibilityCompactedBuffer[i + offset];
		uvec3 indexAndRealTime = BufferVerificationHelper::decodeIndexAndIsRealTime(vectorElement[i]);

		if (indexAndRealTime.x > 0)
		{
			uint hashedIndex = pHashedCompacted[indexAndRealTime.x];
			uvec3 coordinates = unhashValue(hashedIndex, voxelizationSize);
			cout << "(" << coordinates.x << ", " << coordinates.y << ", " << coordinates.z << ")" << endl;
		}
	}

	return false;
}

/////////////////////////////////////////////////////////////////////////////////////////////

bool BufferVerificationHelper::printVisibilityInfoForVoxelCoordinatesNotCompacted(uvec3 voxelCoordinates, uint voxelizationSize)
{
	vectorUint8 vecorHashedCompacted;
	Buffer* voxelHashedPositionCompactedBuffer = bufferM->getElement(move(string("voxelHashedPositionCompactedBuffer")));
	voxelHashedPositionCompactedBuffer->getContentCopy(vecorHashedCompacted);
	uint numHashedCompacted = uint(voxelHashedPositionCompactedBuffer->getDataSize()) / sizeof(uint);
	uint* pHashedCompacted  = (uint*)(vecorHashedCompacted.data());
	bool foundElement       = false;
	int indexUnhashed       = -1;

	forI(numHashedCompacted)
	{
		uvec3 unhashed = unhashValue(pHashedCompacted[i], voxelizationSize);

		if (unhashed == voxelCoordinates)
		{
			cout << "The index of the voxel with coordinates (" << voxelCoordinates.x << ", " << voxelCoordinates.y << ", " << voxelCoordinates.z << ") in voxelHashedPositionCompactedBuffer is " << i << endl;
			indexUnhashed = i;
			break;
		}
	}

	if (indexUnhashed == -1)
	{
		return false;
	}

	vectorUint8 vectorVoxelVisibility4BytesBuffer;
	Buffer* voxelVisibility4BytesBuffer = bufferM->getElement(move(string("voxelVisibility4BytesBuffer"))); // TODO: This buffer changed, adapt to new information structure
	voxelVisibility4BytesBuffer->getContentCopy(vectorVoxelVisibility4BytesBuffer);
	uint numVecorVoxelVisibility4BytesBuffer = uint(voxelVisibility4BytesBuffer->getDataSize()) / sizeof(uint);
	uint* pVoxelVisibility4BytesBuffer = (uint*)(vectorVoxelVisibility4BytesBuffer.data());

	uint offset = 36 * indexUnhashed * 6;
	forI(6)
	{
		cout << "Elements for face" << i << ":" << endl;
		//forJ(37)
		forJ(20)
		{
			uint elementIndex = pVoxelVisibility4BytesBuffer[offset + i * 20 + j];
			if (elementIndex != 4294967295)
			{
				uint hashedIndex  = pHashedCompacted[elementIndex];
				uvec3 coordinates = unhashValue(hashedIndex, voxelizationSize);
				cout << "(" << coordinates.x << ", " << coordinates.y << ", " << coordinates.z << ")" << endl;
			}
		}
	}

	return true;
}

/////////////////////////////////////////////////////////////////////////////////////////////

uvec3 BufferVerificationHelper::decodeIndexAndIsRealTime(uint encodedValue)
{
	// result.x = decoded voxel index value
	// result.y = decoded ray direction index
	// result.z = decoded is real time ray flag value
	uvec3 result;
	uint isRealTimeValue = (encodedValue & 0x80000000) >> 31;
	result.y             = (encodedValue & 0x7F000000) >> 24;
	result.z             = uint(bool(isRealTimeValue == 1));
	result.x             = (encodedValue & 0x00FFFFFF);
	return result;
}

/////////////////////////////////////////////////////////////////////////////////////////////

void BufferVerificationHelper::verifyDynamicVoxelBuffer(int voxelizationSize)
{
	vectorUint8 vectorDynamicVoxelBuffer;
	Buffer* dynamicVoxelBuffer                   = bufferM->getElement(move(string("dynamicVoxelBuffer")));
	dynamicVoxelBuffer->getContentCopy(vectorDynamicVoxelBuffer);
	uint* pHashedCompacted                       = (uint*)(vectorDynamicVoxelBuffer.data());	
	DynamicVoxelCopyToBufferTechnique* technique = static_cast<DynamicVoxelCopyToBufferTechnique*>(gpuPipelineM->getRasterTechniqueByName(move(string("DynamicVoxelCopyToBufferTechnique"))));
	uint numDynamicVoxelBuffer                   = uint(technique->getDynamicVoxelCounter());

	cout << "---------------------------------------------------------" << endl;

	forI(numDynamicVoxelBuffer)
	{
		uint hashed = pHashedCompacted[i];
		uvec3 unhashed = BufferVerificationHelper::unhashValue(hashed, voxelizationSize);

		cout << "Voxel " << i << " has coordinates (" << unhashed.x << ", " << unhashed.y << ", " << unhashed.z << ")" << endl;
	}

	cout << "---------------------------------------------------------" << endl;
}

/////////////////////////////////////////////////////////////////////////////////////////////

void BufferVerificationHelper::verifyDynamicVisibleVoxel(int voxelizationSize, uvec3 voxelCoordinates)
{
	vectorUint8 vecorHashedCompacted;
	Buffer* voxelHashedPositionCompactedBuffer = bufferM->getElement(move(string("voxelHashedPositionCompactedBuffer")));
	voxelHashedPositionCompactedBuffer->getContentCopy(vecorHashedCompacted);
	uint numHashedCompacted = uint(voxelHashedPositionCompactedBuffer->getDataSize()) / sizeof(uint);
	uint* pHashedCompacted  = (uint*)(vecorHashedCompacted.data());
	bool foundElement       = false;
	int indexUnhashed       = -1;

	forI(numHashedCompacted)
	{
		uvec3 unhashed = unhashValue(pHashedCompacted[i], voxelizationSize);

		if (unhashed == voxelCoordinates)
		{
			cout << "The index of the voxel with coordinates (" << voxelCoordinates.x << ", " << voxelCoordinates.y << ", " << voxelCoordinates.z << ") in voxelHashedPositionCompactedBuffer is " << i << endl;
			indexUnhashed = i;
			break;
		}
	}

	if (indexUnhashed == -1)
	{
		return;
	}

	vectorUint8 vectorDynamicVoxelVisibilityFlagBuffer;
	Buffer* dynamicVoxelVisibilityFlagBuffer       = bufferM->getElement(move(string("dynamicVoxelVisibilityFlagBuffer")));
	dynamicVoxelVisibilityFlagBuffer->getContentCopy(vectorDynamicVoxelVisibilityFlagBuffer);
	uint numVectorDynamicVoxelVisibilityFlagBuffer = uint(dynamicVoxelVisibilityFlagBuffer->getDataSize()) / sizeof(uint);
	uint* pVectorDynamicVoxelVisibilityFlagBuffer  = (uint*)(vectorDynamicVoxelVisibilityFlagBuffer.data());

	uint faceIndex = 1;

	uint index0                                    = indexUnhashed * 4 * 6 + faceIndex * 4 + 0;
	uint index1                                    = indexUnhashed * 4 * 6 + faceIndex * 4 + 1;
	uint index2                                    = indexUnhashed * 4 * 6 + faceIndex * 4 + 2;
	uint index3                                    = indexUnhashed * 4 * 6 + faceIndex * 4 + 3;

	uint bitFlags0                                 = pVectorDynamicVoxelVisibilityFlagBuffer[index0];
	uint bitFlags1                                 = pVectorDynamicVoxelVisibilityFlagBuffer[index1];
	uint bitFlags2                                 = pVectorDynamicVoxelVisibilityFlagBuffer[index2];
	uint bitFlags3                                 = pVectorDynamicVoxelVisibilityFlagBuffer[index3];

	vectorUint8 vectorDynamicVoxelVisibility4BytesBuffer;
	Buffer* dynamicVoxelVisibility4BytesBuffer       = bufferM->getElement(move(string("voxelVisibilityDynamic4BytesBuffer")));
	dynamicVoxelVisibility4BytesBuffer->getContentCopy(vectorDynamicVoxelVisibility4BytesBuffer);
	uint numVectorDynamicVoxelVisibility4BytesBuffer = uint(dynamicVoxelVisibility4BytesBuffer->getDataSize()) / sizeof(uint);
	uint* pVectorDynamicVoxelVisibility4BytesBuffer  = (uint*)(vectorDynamicVoxelVisibility4BytesBuffer.data());

	for (int i = 0; i < 32; ++i)
	{
		if ((bitFlags0 & (1 << i)) > 0)
		{
			uint offset           = indexUnhashed * 6 * 36 + faceIndex * 36 + i; /* 36 is the amout of rays per voxel face */
			uint voxelCoordinates = pVectorDynamicVoxelVisibility4BytesBuffer[offset];
			uvec3 unhashed        = unhashValue(voxelCoordinates, voxelizationSize);
			cout << "Ray with index " << i << " sees dynamic voxel with coordinates (" << unhashed.x << ", " << unhashed.y << ", " << unhashed.z << ")" << endl;
		}
	}

	for (int i = 0; i < 32; ++i)
	{
		if ((bitFlags1 & (1 << i)) > 0)
		{
			uint offset           = indexUnhashed * 6 * 36 + faceIndex * 36 + 32 + i; /* 36 is the amout of rays per voxel face */
			uint voxelCoordinates = pVectorDynamicVoxelVisibility4BytesBuffer[offset];
			uvec3 unhashed        = unhashValue(voxelCoordinates, voxelizationSize);
			cout << "Ray with index " << 32 + i << " sees dynamic voxel with coordinates (" << unhashed.x << ", " << unhashed.y << ", " << unhashed.z << ")" << endl;
		}
	}
}

/////////////////////////////////////////////////////////////////////////////////////////////