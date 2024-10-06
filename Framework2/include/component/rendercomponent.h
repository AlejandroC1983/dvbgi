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

#ifndef _RENDERCOMPONENT_H_
#define _RENDERCOMPONENT_H_

// GLOBAL INCLUDES

// PROJECT INCLUDES
#include "../../include/component/basecomponent.h"

// CLASS FORWARDING

// NAMESPACE

// DEFINES

/////////////////////////////////////////////////////////////////////////////////////////////

class RenderComponent: public BaseComponent
{
	friend class ComponentManager;
	friend class Node;

protected:
	/** Default constructor
	* @return nothing */
	//RenderComponent();

	/** Paremeter constructor
	* @param name         [in] component resource name
	* @param className    [in] component class name
	* @param resourceType [in] component resource type
	* @return nothing */
	RenderComponent(string&& name, string&& className, GenericResourceType resourceType);

	/** Generates node's normals
	* @return nothing */
	void generateNormals();

	/** Generates node's tangents
	* @return nothing */
	void generateTangents();

	/** Compress node's tangents
	* @return nothing */
	void compressTangents();

	/** Compress node's normals
	* @return nothing */
	void compressNormals();

	/** Sends to GPU the node's per vertex data
	* @return nothing */
	void bindData();

	/** Build the index and vertex buffers used for the ray tracing acceleration structure (m_indexBuffer and m_vertexBuffer)
	* @return nothing */
	void buildAccelerationStructureBuffers();

	/** Fills m_vertexData with all the info for this model taken from m_vertices, m_normals, m_texCoord and m_tangents
	* building an array of structures for best performance when doing draw calls
	* @return nothing */
	void buildPerVertexInfo();

public:
	/** Default destructor
	* @return nothing */
	virtual ~RenderComponent();

	/** Initialize the resource
	* @return nothing */
	virtual void init();

	REF(vectorUint, m_indices, Indices)
	GET(vectorUint, m_indices, Indices)
	REF(vectorVec3, m_vertices, Vertices)
	GET(vectorVec3, m_vertices, Vertices)
	REF(vectorVec3, m_normals, Normals)
	GET(vectorVec3, m_normals, Normals)
	REF(vectorVec2, m_texCoord, TexCoord)
	GET(vectorVec2, m_texCoord, TexCoord)
	REF(vectorVec3, m_tangents, Tangents)
	GET(vectorVec3, m_tangents, Tangents)
	REF(vectorUint8, m_vertexData, VertexData)
	GET(vectorUint8, m_vertexData, VertexData)
	GET_SET(eMeshType, m_meshType, MeshType)
	GET_SET(uint, m_startIndex, StartIndex)
	GET_SET(uint, m_endIndex, EndIndex)
	GET_SET(uint, m_indexOffset, IndexOffset)
	GET_SET(uint, m_indexSize, IndexSize)
	GET_PTR(Material, m_material, Material)
	REF_PTR(Material, m_material, Material)
	GET_PTR(Material, m_materialInstanced, MaterialInstanced)
	REF_PTR(Material, m_materialInstanced, MaterialInstanced)
	GET_PTR(Buffer, m_indexBuffer, IndexBuffer)
	GET_PTR(Buffer, m_vertexBuffer, VertexBuffer)
	REF_PTR(Buffer, m_indexBuffer, IndexBuffer)
	REF_PTR(Buffer, m_vertexBuffer, VertexBuffer)
	SET(float, m_instanceCounter, InstanceCounter)
	SET(eNodeType, m_nodeType, NodeType)
	
protected:
	vectorUint    m_indices;           //!< vector of indices of this node that will be sent to GPU
	vectorVec3    m_vertices;          //!< vector of vertices of this node that will be sent to GPU
	vectorVec2    m_texCoord;          //!< vector of texture coordinates of this node that will be sent to GPU
	vectorVec3    m_normals;           //!< vector of normals of this node that will be sent to GPU
	vectorVec3    m_tangents;          //!< vector of tangents of this node that will be sent to GPU
	vectorUint8	  m_vertexData;        //!< All the previous per - vertex information stored as an array of structures with all the info for each vertex
	eGeometryType m_geomType;          //!< Geometry type of the mesh (triangles, triangle strip, triangle fan for now)
	eMeshType	  m_meshType;          //!< To clasify each type of mesh: light volume / no light volume, torus light volume, etc
	uint		  m_startIndex;        //!< Start index in the scene index buffer for render this model
	uint		  m_endIndex;          //!< End index in the scene index buffer for render this model
	uint		  m_indexOffset;       //!< Offset inside the scene index buffer where all the meshes geometry is loaded, to find the indices corresponding to this mesh
	uint		  m_indexSize;         //!< Size of the index list for this mesh
	bool		  m_generateNormal;    //!< If true, then normals will be generated from the given geometry
	bool		  m_generateTangent;   //!< If true, then tangents will be generated from the given geometry
	Material*     m_material;          //!< Material to use for this node for scene (main purpose) rasterization
	Material*     m_materialInstanced; //!< Material to use for this node for instanced rasterization
	Buffer*       m_indexBuffer;       //!< Buffer with device address flag with the indices of this node, used for the acceleration structure
	Buffer*       m_vertexBuffer;      //!< Buffer with device address flag with the verices in the final vertex format (position, UVID, normal, tangent) of this node, used for the acceleration structure
	float         m_instanceCounter;   //!< Instance counter variable value from the node owning this component
	eNodeType     m_nodeType;          //!< Enum describing the type of node (static / dynamic), copy from the node this component is part of
};

/////////////////////////////////////////////////////////////////////////////////////////////

#endif _RENDERCOMPONENT_H_
