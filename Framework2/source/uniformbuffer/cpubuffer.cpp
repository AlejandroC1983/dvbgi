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
#include "../../include/uniformbuffer/cpubuffer.h"
#include "../../include/core/coremanager.h"
#include "../../include/buffer/buffermanager.h"
#include "../../include/buffer/buffer.h"
#include "../../include/shader/uniformBase.h"
#include "../../include/shader/shadermanager.h"
#include "../../include/util/mathutil.h"

// NAMESPACE

// DEFINES

// STATIC MEMBER INITIALIZATION

/////////////////////////////////////////////////////////////////////////////////////////////

CPUBuffer::CPUBuffer():
	  m_dynamicAllignment(0)
	, m_UBHostMemory(nullptr)
	, m_UBHostMemorySize(0)
	, m_minCellSize(0)
	, m_numCells(0)
{
	
}

/////////////////////////////////////////////////////////////////////////////////////////////

CPUBuffer::~CPUBuffer()
{
	destroyResources();
}

/////////////////////////////////////////////////////////////////////////////////////////////

void CPUBuffer::buildCPUBuffer()
{
	size_t minUniformBufferObjectOffsetAlignment = coreM->getPhysicalDeviceProperties().limits.minUniformBufferOffsetAlignment;
	if (minUniformBufferObjectOffsetAlignment > 0)
	{
		m_dynamicAllignment = int32_t(ceil(float(m_minCellSize) / float(minUniformBufferObjectOffsetAlignment)) * float(minUniformBufferObjectOffsetAlignment));

		if (!MathUtil::isPowerOfTwo(uint(m_dynamicAllignment)))
		{
			m_dynamicAllignment = MathUtil::getNextPowerOfTwo(uint(m_dynamicAllignment));
		}

		m_UBHostMemorySize  = m_numCells * m_dynamicAllignment;
		m_UBHostMemory      = (void*)alignedMemoryAllocation(m_UBHostMemorySize, m_dynamicAllignment);

		m_arrayCellStartPointer.resize(m_numCells);
		m_arrayCellCurrentPointer.resize(m_numCells);
		m_arrayCellOffset.resize(m_numCells);

		// Initialize pointers to the starting byte of each cell and offset in bytes inside each cell
		forI(uint(m_numCells))
		{
			m_arrayCellStartPointer[i]   = (void*)(((uint64_t)m_UBHostMemory + (i * m_dynamicAllignment)));
			m_arrayCellCurrentPointer[i] = m_arrayCellStartPointer[i];
		}
		memset(&m_arrayCellOffset[0], 0, sizeof(int) * m_arrayCellOffset.size());
	}
}

/////////////////////////////////////////////////////////////////////////////////////////////

void CPUBuffer::buildCPUBuffer(const vectorUniformBasePtr& vecUniformBase)
{
	m_UBHostMemorySize  = shaderM->getMaxPushConstantsSize();
	m_UBHostMemorySize  = size_t(pow(2.0f, ceil(log(float(m_UBHostMemorySize) - 1.0f) / log(2.0f)))); // Take closer power of two value for max push constant size (>= than current value)
	m_UBHostMemory      = (void*)alignedMemoryAllocation(m_UBHostMemorySize, m_UBHostMemorySize);
	m_numCells          = int(vecUniformBase.size());
	m_dynamicAllignment = m_UBHostMemorySize; // NOTE: considering the push constant CPU mapped buffer as a single piece of memory

	m_arrayCellStartPointer.resize(m_numCells);
	m_arrayCellCurrentPointer.resize(m_numCells);
	m_arrayCellOffset.resize(m_numCells);


	uint numCellsUint = uint(m_numCells);
	int result        = 0;

	forI(numCellsUint)
	{
		m_arrayCellStartPointer[i]   = (void*)(((uint64_t)m_UBHostMemory + result));
		m_arrayCellCurrentPointer[i] = m_arrayCellStartPointer[i];
		result                      += resourceenum::getResourceInternalTypeSizeInBytes(vecUniformBase[i]->getResourceInternalType());
	}
}

/////////////////////////////////////////////////////////////////////////////////////////////

void* CPUBuffer::alignedMemoryAllocation(size_t size, size_t alignment)
{
	void *data = nullptr;
#if defined(_MSC_VER) || defined(__MINGW32__)
	data = _aligned_malloc(size, alignment);
#else 
	int res = posix_memalign(&data, alignment, size);
	if (res != 0)
	{
		data = nullptr;
	}
#endif
	return data;
}

/////////////////////////////////////////////////////////////////////////////////////////////

void CPUBuffer::alignedMemoryFree(void* data)
{
#if	defined(_MSC_VER) || defined(__MINGW32__)
	_aligned_free(data);
#else 
	free(data);
#endif
}

/////////////////////////////////////////////////////////////////////////////////////////////

void CPUBuffer::destroyResources()
{
	alignedMemoryFree(m_UBHostMemory);
	m_dynamicAllignment = 0;
	m_UBHostMemory      = nullptr;
	m_UBHostMemorySize  = 0;
	m_minCellSize       = 0;
	m_numCells          = 0;
	m_arrayCellStartPointer.clear();
	m_arrayCellCurrentPointer.clear();
	m_arrayCellOffset.clear();
}

/////////////////////////////////////////////////////////////////////////////////////////////

void CPUBuffer::resetDataAtCell(int index)
{
	memset(m_arrayCellStartPointer[index], 0, m_arrayCellOffset[index]);
	m_arrayCellOffset[index] = 0;
	m_arrayCellCurrentPointer[index] = m_arrayCellStartPointer[index];
}

/////////////////////////////////////////////////////////////////////////////////////////////

int CPUBuffer::getAvailableRoomAtCell(int index)
{
	return int(m_dynamicAllignment) - int(m_arrayCellOffset[index]);
}

/////////////////////////////////////////////////////////////////////////////////////////////
