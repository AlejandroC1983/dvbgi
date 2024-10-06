/*
Copyright 2014 Alejandro Cosin

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
#include "../../include/headers.h"
#include "../../include/util/loopmacrodefines.h"
#include "../../include/buffer/buffer.h"
#include "../../include/buffer/buffermanager.h"
#include "../../include/component/rendercomponent.h"

// NAMESPACE

// DEFINES

// STATIC MEMBER INITIALIZATION

/////////////////////////////////////////////////////////////////////////////////////////////

RenderComponent::RenderComponent(string&& name, string&& className, GenericResourceType resourceType) : BaseComponent(move(name), move(className), GenericResourceType::GRT_RENDERCOMPONENT)
	, m_geomType(E_GT_TRIANGLES)
	, m_meshType(eMeshType::E_MT_SIZE)
	, m_startIndex(0)
	, m_endIndex(0)
	, m_indexOffset(0)
	, m_indexSize(0)
	, m_generateNormal(true)
	, m_generateTangent(true)
	, m_material(nullptr)
	, m_materialInstanced(nullptr)
	, m_indexBuffer(nullptr)
	, m_vertexBuffer(nullptr)
{

}

/////////////////////////////////////////////////////////////////////////////////////////////

RenderComponent::~RenderComponent()
{
}

/////////////////////////////////////////////////////////////////////////////////////////////

void RenderComponent::init()
{
	if (m_normals.size() == m_vertices.size())
	{
		m_generateNormal = false;
	}

	if (m_tangents.size() == m_vertices.size())
	{
		m_generateTangent = false;
	}

	if (m_generateNormal)
	{
		generateNormals();
	}

	if (m_generateTangent)
	{
		generateTangents();
	}

	compressNormals();
	compressTangents();

	buildPerVertexInfo();

	bindData();

	buildAccelerationStructureBuffers();
}

/////////////////////////////////////////////////////////////////////////////////////////////

void RenderComponent::generateNormals()
{
	// For each set of three index, take the corresponding vertices v0, v1 and v2, and generate normals for the three by
	// making vectors v0v1 and v0v2, and the cross product between them, giving an orthonormal vector to the triangle v0-v1-v2
	// As each vertex can be used in multiple triangles, each time a normal is calculated for a particular vertex, it is added
	// to the previously obtained normals for that vertex. At the end, the resulting vector is divided by the amount of normal
	// vectors and renormalized, obtaining an averaged normal

	vectorVec3 faceNormals; //Temporary array to store the face normals
	vectorInt shareCount;

	m_normals.resize(m_vertices.size()); //We want a normal for each vertex
	shareCount.resize(m_vertices.size());

	forI(shareCount.size())
	{
		shareCount[i] = 0;
	}

	uint numTriangles = uint(m_indices.size()) / 3;
	faceNormals.resize(numTriangles); //One normal per triangle

	forI(numTriangles)
	{
		vec3 v0 = m_vertices.at(m_indices.at(i * 3));
		vec3 v1 = m_vertices.at(m_indices.at((i * 3) + 1));
		vec3 v2 = m_vertices.at(m_indices.at((i * 3) + 2));
		vec3 vec0 = v1 - v0;
		vec3 vec1 = v2 - v0;
		vec3 normal = faceNormals.at(i);

		normal = cross(vec0, vec1);
		normal = normalize(normal);

		forJ(3)
		{
			int index = m_indices.at((i * 3) + j);
			m_normals.at(index) += normal;
			shareCount.at(index)++;
		}
	}

	forI(m_vertices.size())
	{
		m_normals.at(i).x = m_normals.at(i).x / shareCount.at(i);
		m_normals.at(i).y = m_normals.at(i).y / shareCount.at(i);
		m_normals.at(i).z = m_normals.at(i).z / shareCount.at(i);
		m_normals.at(i) = normalize(m_normals.at(i));
	}
}

/////////////////////////////////////////////////////////////////////////////////////////////

void RenderComponent::generateTangents()
{
	vectorVec3 tan1;
	tan1.resize(m_vertices.size() * 2);
	vectorVec3 tan2;
	tan2.resize(m_vertices.size() * 2);
	forIT(tan1)
	{
		it->x = 0.0f;
		it->y = 0.0f;
		it->z = 0.0f;
	}
	forIT(tan2)
	{
		it->x = 0.0f;
		it->y = 0.0f;
		it->z = 0.0f;
	}

	uint numTriangles = uint(m_indices.size()) / 3;

	forI(numTriangles)
	{
		vec3 v1 = m_vertices.at(m_indices.at(i * 3));
		vec3 v2 = m_vertices.at(m_indices.at((i * 3) + 1));
		vec3 v3 = m_vertices.at(m_indices.at((i * 3) + 2));

		vec2 w1 = m_texCoord.at(m_indices.at(i * 3));
		vec2 w2 = m_texCoord.at(m_indices.at((i * 3) + 1));
		vec2 w3 = m_texCoord.at(m_indices.at((i * 3) + 2));

		float x1 = v2.x - v1.x;
		float x2 = v3.x - v1.x;
		float y1 = v2.y - v1.y;
		float y2 = v3.y - v1.y;
		float z1 = v2.z - v1.z;
		float z2 = v3.z - v1.z;

		float s1 = w2.x - w1.x;
		float s2 = w3.x - w1.x;
		float t1 = w2.y - w1.y;
		float t2 = w3.y - w1.y;

		float r = 1.0F / (s1 * t2 - s2 * t1);
		vec3 sdir = vec3((t2 * x1 - t1 * x2) * r, (t2 * y1 - t1 * y2) * r, (t2 * z1 - t1 * z2) * r);
		vec3 tdir = vec3((s1 * x2 - s2 * x1) * r, (s1 * y2 - s2 * y1) * r, (s1 * z2 - s2 * z1) * r);

		// If there are bad tex coords, do not add to the tan1 and tan2 vectors
		bool b1 = (isnan(r) || isnan(sdir.x) || isnan(sdir.y) || isnan(sdir.z) || isnan(tdir.x) || isnan(tdir.y) || isnan(tdir.z));
		if (!b1)
		{
			tan1.at(m_indices.at(i * 3)) += sdir;
			tan1.at(m_indices.at(i * 3 + 1)) += sdir;
			tan1.at(m_indices.at(i * 3 + 2)) += sdir;

			tan2.at(m_indices.at(i * 3)) += tdir;
			tan2.at(m_indices.at(i * 3 + 1)) += tdir;
			tan2.at(m_indices.at(i * 3 + 2)) += tdir;
		}
	}

	m_tangents.clear();
	m_tangents.resize(m_vertices.size());

	forA(m_vertices.size())
	{
		vec3 n = m_normals.at(a);
		vec3 t = tan1.at(a);
		vec3 vTemp = t;
		// Gram-Schmidt orthogonalize: only if dot product between n and t is != 0, otherwise (0,0,0) vector will be generated
		if (dot(n, t) != 0.0f)
		{
			vTemp = (t - n) * dot(n, t);
		}
		vTemp = normalize(vTemp);
		m_tangents.at(a) = vTemp;
	}

	tan1.clear();
	tan2.clear();

	// taken from http://www.terathon.com/code/tangent.html for generating correctly the tangent "T" vector
	// (must be aligned with the increasing "u" direction of the texture coordinates, as the v direction corresponds
	// to the B or binormal vector)
}

/////////////////////////////////////////////////////////////////////////////////////////////

void RenderComponent::compressTangents()
{
	const vec3 vectorToAdd = vec3(0.5);
	forI(m_tangents.size())
	{
		m_tangents[i] *= 0.5f;
		m_tangents[i] += vectorToAdd;
		m_tangents[i] = vec3(floor(m_tangents[i].x * 255.0f), floor(m_tangents[i].y * 255.0f), floor(m_tangents[i].z * 255.0f));
	}
}

/////////////////////////////////////////////////////////////////////////////////////////////

void RenderComponent::compressNormals()
{
	const vec3 vectorToAdd = vec3(0.5);
	forI(m_normals.size())
	{
		m_normals[i] *= 0.5f;
		m_normals[i] += vectorToAdd;
		m_normals[i] = vec3(floor(m_normals[i].x * 255.0f), floor(m_normals[i].y * 255.0f), floor(m_normals[i].z * 255.0f));
	}
}

/////////////////////////////////////////////////////////////////////////////////////////////

void RenderComponent::bindData()
{
	// Once the per vertex data has been uploaded, the texture coordinate, normal and tangent data per vertex can be
	// deleted as it won't be used again. The indices and vertices arrays can be used to obtain information such as
	// the point in polyhedron test
	m_texCoord.clear();
	m_normals.clear();
	m_tangents.clear();
}

/////////////////////////////////////////////////////////////////////////////////////////////

void RenderComponent::buildAccelerationStructureBuffers()
{
	m_indexBuffer = bufferM->buildBuffer(move(m_name + string("indexBuffer")),
		(void*)m_indices.data(),
		sizeof(uint) * m_indices.size(),
		VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

	m_vertexBuffer = bufferM->buildBuffer(move(m_name + string("vertexBuffer")),
		(void*)m_vertexData.data(),
		sizeof(uint8_t) * m_vertexData.size(),
		VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
}

/////////////////////////////////////////////////////////////////////////////////////////////

void RenderComponent::buildPerVertexInfo()
{
	m_vertexData.resize(m_vertices.size() * 24); // TODO: Set parameter
	uint8_t* pData   = (uint8_t*)(m_vertexData.data());
	uint counter     = 0;
	uint8_t nodeType = ((m_nodeType == eNodeType::E_NT_DYNAMIC_ELEMENT) || (m_nodeType == eNodeType::E_NT_SKINNEDMESH_ELEMENT)) ? 1 : 0;

	forI(m_vertices.size())
	{
		uint8_t* pDataTemp = (uint8_t* )(&m_vertices[i].x);
		pData[counter++] = pDataTemp[0];
		pData[counter++] = pDataTemp[1];
		pData[counter++] = pDataTemp[2];
		pData[counter++] = pDataTemp[3];

		pDataTemp        = (uint8_t*)(&m_vertices[i].y);
		pData[counter++] = pDataTemp[0];
		pData[counter++] = pDataTemp[1];
		pData[counter++] = pDataTemp[2];
		pData[counter++] = pDataTemp[3];

		pDataTemp        = (uint8_t*)(&m_vertices[i].z);
		pData[counter++] = pDataTemp[0];
		pData[counter++] = pDataTemp[1];
		pData[counter++] = pDataTemp[2];
		pData[counter++] = pDataTemp[3];

		uint uv          = glm::packHalf2x16(vec2(m_texCoord[i].x, m_texCoord[i].y));
		pDataTemp        = (uint8_t*)(&uv);
		pData[counter++] = pDataTemp[0];
		pData[counter++] = pDataTemp[1];
		pData[counter++] = pDataTemp[2];
		pData[counter++] = pDataTemp[3];

		pData[counter++] = uint8_t(m_normals[i].x);
		pData[counter++] = uint8_t(m_normals[i].y);
		pData[counter++] = uint8_t(m_normals[i].z);
		pData[counter++] = 0;

		pData[counter++] = uint8_t(m_tangents[i].x);
		pData[counter++] = uint8_t(m_tangents[i].y);
		pData[counter++] = uint8_t(m_tangents[i].z);
		pData[counter++] = nodeType;
	}
}

/////////////////////////////////////////////////////////////////////////////////////////////
