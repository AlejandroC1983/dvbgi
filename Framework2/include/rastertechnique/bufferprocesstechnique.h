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

#ifndef _BUFFERPROCESSTECHNIQUE_H_
#define _BUFFERPROCESSTECHNIQUE_H_

// GLOBAL INCLUDES

// PROJECT INCLUDES
#include "../../include/rastertechnique/rastertechnique.h"

// CLASS FORWARDING
class Material;
class Buffer;

// NAMESPACE

// DEFINES

/////////////////////////////////////////////////////////////////////////////////////////////

class BufferProcessTechnique: public RasterTechnique
{
	DECLARE_FRIEND_REGISTERER(BufferProcessTechnique)

protected:
	/** Parameter constructor
	* @param name      [in] technique's name
	* @param className [in] class name
	* @return nothing */
	BufferProcessTechnique(string &&name, string&& className);

	/** Default destructor
	* @return nothing */
	virtual ~BufferProcessTechnique();

public:
	/** New shaders, textures, etc initialization should be done here, called when the tehnique is instantiated
	* @return nothing */
	virtual void init();

	/** Called in record method after start recording command buffer, to allow any image / memory barriers
	* needed for the resources being used
	* @param commandBuffer [in] command buffer to record to
	* @return nothing */
	virtual void recordBarriers(VkCommandBuffer* commandBuffer);

	/** Record command buffer
	* @param currentImage      [in] current screen image framebuffer drawing to (in case it's needed)
	* @param commandBufferID   [in] Unique identifier of the command buffer returned as parameter
	* @param commandBufferType [in] Queue to submit the command buffer recorded in the call
	* @return command buffer the technique has recorded to */
	virtual VkCommandBuffer* record(int currentImage, uint& commandBufferID, CommandBufferType& commandBufferType);

	/** Called in record method before end recording command buffer, to allow any image / memory barriers
	* needed for the resources being used
	* @param commandBuffer [in] command buffer to record to
	* @return nothing */
	virtual void recordBarriersEnd(VkCommandBuffer* commandBuffer);

	SET_PTR(Buffer, m_buffer, Buffer)
	GETCOPY(uint, m_numElementPerLocalWorkgroupThread, NumElementPerLocalWorkgroupThread)
	GETCOPY(uint, m_numThreadPerLocalWorkgroup, NumThreadPerLocalWorkgroup)
	GETCOPY(uint, m_bufferNumElement, BufferNumElement)

protected:
	/** Set in m_computeShaderThreadMapping the compute shader code for mapping the execution threads
	* @return nothing */
	void buildShaderThreadMapping();

	/** Set the number of compute workgroups to dispatch in x and y dimension (m_localWorkGroupsXDimension and
	* m_localWorkGroupsYDimension) taking into account m_bufferNumElement, m_numElementPerLocalWorkgroupThread
	* and m_numThreadPerLocalWorkgroup values (and physical device limits)
	* @return nothing */
	void obtainDispatchWorkGroupCount();

	/** Utility static method to compute the number of workgroup elements to dispatch i nthe x and y dimension
	* @param numThreadPerLocalWorkgroup        [in]    number of thread per local workgroup
	* @param numElementPerLocalWorkgroupThread [in]    number element per local workgroup
	* @param bufferNumElement                  [in]    number of elements to process
	* @param localWorkGroupsXDimension         [inout] number of local workgroups to dispatch in the x dimension
	* @param localWorkGroupsYDimension         [inout] number of local workgroups to dispatch in the y dimension
	* @return nothing */
	static void obtainDispatchWorkGroupCount(uint  numThreadPerLocalWorkgroup,
											 uint  numElementPerLocalWorkgroupThread,
											 uint  bufferNumElement,
											 uint& localWorkGroupsXDimension,
											 uint& localWorkGroupsYDimension);

	Buffer*   m_buffer;                            //!< Buffer to process
	uint      m_bufferNumElement;                  //!< Number of elements to process in m_buffer
	uint      m_numElementPerLocalWorkgroupThread; //!< Number of elements per thread in each local workgroup
	uint      m_numThreadPerLocalWorkgroup;        //!< Number of threads per local workgroup
	uint      m_localWorkGroupsXDimension;         //!< Number of dispatched local workgroups in the x dimension
	uint      m_localWorkGroupsYDimension;         //!< Number of dispatched local workgroups in the y dimension
	uint      m_numThreadExecuted;                 //!< Number of threads executed by the whole dispatch
	Material* m_material;                          //!< Material used for the technique
	bool      m_newPassRequested;                  //!< If true, new pass was requested to process the assigned buffer
	string    m_computeShaderThreadMapping;        //!< String with the thread mapping taking into account m_bufferNumElement, m_numElementPerLocalWorkgroupThread and m_numThreadPerLocalWorkgroup values (and physical device limits)
	int       m_localSizeX;                        //!< Compute shader value for local_size_x
	int       m_localSizeY;                        //!< Compute shader value for local_size_x
};

/////////////////////////////////////////////////////////////////////////////////////////////

#endif _BUFFERPROCESSTECHNIQUE_H_
