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

#ifndef _MATERIALSCENERAYTRACEHIT_H_
#define _MATERIALSCENERAYTRACEHIT_H_

// GLOBAL INCLUDES

// PROJECT INCLUDES
#include "../../include/material/materialmanager.h"
#include "../../include/material/material.h"
#include "../../include/shader/shadermanager.h"
#include "../../include/shader/shader.h"
#include "../../include/node/node.h"
#include "../../include/buffer/buffer.h"
#include "../../include/buffer/buffermanager.h"

// CLASS FORWARDING

// NAMESPACE

// DEFINES

/////////////////////////////////////////////////////////////////////////////////////////////

class MaterialSceneRayTraceHit: public Material
{
	friend class MaterialManager;

protected:
	/** Parameter constructor
	* @param [in] shader's name
	* @return nothing */
	MaterialSceneRayTraceHit(string&& name) : Material(move(name), move(string("MaterialSceneRayTraceHit")))
	{
		m_resourcesUsed = MaterialBufferResource::MBR_CAMERA;
	}

public:
	/** Loads the shader to be used. This could be substituted
	* by reading from a file, where the shader details would be present
	* @return true if the shader was loaded successfully, and false otherwise */
	bool loadShader()
	{
		size_t shaderSize;
		void* rayHitShaderCode = InputOutput::readFile("../data/vulkanshaders/sceneraytracerayhit.rchit", &shaderSize);
		assert(rayHitShaderCode != nullptr);

		string shaderCode    = string((const char*)rayHitShaderCode);
		m_shaderResourceName = "sceneraytracehit";
		m_shader             = shaderM->buildShaderRayHit(move(string(m_shaderResourceName)), shaderCode.c_str(), m_materialSurfaceType);

		return true;
	}

	/** Exposes the variables that will be modified in the corresponding material data uniform buffer used in the scene,
	* allowing to work from variables from C++ without needing to update the values manually (error-prone)
	* @return nothing */
	void exposeResources()
	{
		vector<string> vectorIndexBufferName(m_vectorNode.size());
		vector<string> vectorVertexBufferName(m_vectorNode.size());

		forI(m_vectorNode.size())
		{
			vectorIndexBufferName[i]  = m_vectorNode[i]->getVertexBuffer()->getName();
			vectorVertexBufferName[i] = m_vectorNode[i]->getVertexBuffer()->getName();
		}

		Buffer* temp = bufferM->getElement(move(string("sceneDescriptorStaticBuffer")));

		assert(temp == nullptr);

		assignShaderStorageBuffer(move(string("vertices")),                    move(vectorIndexBufferName),                 VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);
		assignShaderStorageBuffer(move(string("indices")),                     move(vectorVertexBufferName),                VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);
		assignShaderStorageBuffer(move(string("sceneDescriptorStaticBuffer")), move(string("sceneDescriptorStaticBuffer")), VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);
	}

	SET(vectorNodePtr, m_vectorNode, VectorNode)

protected:
	vectorNodePtr m_vectorNode; //!< Vector with pointers to the nodes to be used in the ray tracing technique that uses this shader
};

/////////////////////////////////////////////////////////////////////////////////////////////

#endif _MATERIALSCENERAYTRACEHIT_H_
