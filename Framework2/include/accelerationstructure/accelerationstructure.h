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

#ifndef _ACCELERATIONSTRUCTURE_H_
#define _ACCELERATIONSTRUCTURE_H_

// GLOBAL INCLUDES

// PROJECT INCLUDES
#include "../headers.h"
#include "../../include/util/genericresource.h"
#include "../../include/node/node.h"

// CLASS FORWARDING

// NAMESPACE

// DEFINES

/////////////////////////////////////////////////////////////////////////////////////////////

class AccelerationStructure : public GenericResource
{
	friend class AccelerationStructureManager;

protected:
	/** Parameter constructor
	* @param name       [in] acceleration structure's name
	* @param vectorNode [in] vector with the nodes that will form this acceleration structure
	* @return nothing */
	AccelerationStructure(string &&name, vectorNodePtr&& vectorNode);

	/** Destructor
	* @return nothing */
	virtual ~AccelerationStructure();

public:
	/** Calls buildTLAS(true) to update the TLAS
	* @return nothing */
	//void updateTLAS();

	/** Update TLAS with only the information for the nodes given as parameter
	* @param vectorNode [in] pointer to the vector with the nodes to update
	* @return nothing */
	void updateTLAS(const vectorNodePtr* vectorUpdateNode);

	/** Update the BLAS nodes with index given as parameter. The index / vetex buffers of the nodes to update 
	* (in RenderComponent::m_indexBuffer and RenderComponent::m_vertexBuffer) are suppposed to be updated by the time this call is done
	* @param commandBuffer [in] command buffer to record the BLAS update commands to
	* @param vectorNode    [in] vector with the nodes to update its BLAS with the contents of RenderComponent::m_indexBuffer and RenderComponent::m_vertexBuffer
	* @return nothing */
	void updateBLAS(VkCommandBuffer commandBuffer, const vectorNodePtr& vectorNode);

	REF(VkAccelerationStructureKHR, m_tlas, Tlas)
	GET(vectorNodePtr, m_vectorNode, VectorNode)

protected:
	/** Build the Bottom Level Acceleration Structure for the elements in m_vectorNode
	* @return nothing */
	void buildBLAS();

	/** Build the Top Level Acceleration Structure for the elements in m_vectorNode
	* @param update           [in] flag to know when to update the top level acceleration structure
	* @param vectorUpdateNode [in] pointer to the vector with the nodes to update in case update is true
	* @return nothing */
	void buildTLAS(bool update, const vectorNodePtr* vectorUpdateNode);

	/** Destroys m_framebuffer and cleans m_arrayAttachment and m_renderPass
	* @return nothing */
	void destroyResources();

	VkAccelerationStructureKHR         m_tlas;                  //!< Top Level Acceleration Structure
	vector<VkAccelerationStructureKHR> m_blas;                  //!< Vector with all the Bottom Level Acceleration Structures
	vector<Buffer*>                    m_blasBuffer;            //!< Vector with all the Bottom Level Acceleration Structures buffers
	vector<VkDeviceAddress>            m_blasBufferDeviceAddres;//!< Vector with the device address of each BLAS buffer element at the same index in m_blasBuffer
	vectorNodePtr                      m_vectorNode;            //!< Vector with pointers to the nodes that form this acceleration structure
	mapNodePrtUint                     m_mapNodeInternalIndex;  //!< Helper map to know, for each Node element in vectorNodePtr, what is the index in vectorNodePtr if that node (used for BLAS updates)
	Buffer*                            m_hostVisibleTLASBuffer; //!< Host visible buffer with the TLAS information, done initially with all the TLAS information and kept for partial updates of those scene elements that have their information changed
	Buffer*                            m_deviceLocalTLASBuffer; //!< One of the two device local buffers with the TLAS information, to be updated only with those changes from the buffer m_hostVisibleTLASBuffer. It seesm the call to 
	Buffer*                            m_tlasScratchBuffer;     //!< Scratch buffer used to build the TLAS
	bool                               m_usingDeviceLocalBuffer0; //!< Flag to alternate between m_deviceLocalTLASBuffer0 and m_deviceLocalTLASBuffer1
	static uint                        m_instanceCounter;       //!< Static variable to count how many acceleration strucutres have been built
};

/////////////////////////////////////////////////////////////////////////////////////////////

#endif _ACCELERATIONSTRUCTURE_H_
