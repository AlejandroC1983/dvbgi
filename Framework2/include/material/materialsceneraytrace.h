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

#ifndef _MATERIALSCENERAYTRACE_H_
#define _MATERIALSCENERAYTRACE_H_

// GLOBAL INCLUDES

// PROJECT INCLUDES
#include "../../include/material/materialmanager.h"
#include "../../include/material/material.h"
#include "../../include/shader/shadermanager.h"
#include "../../include/shader/shader.h"
#include "../../include/node/node.h"
#include "../../include/buffer/buffer.h"
#include "../../include/buffer/buffermanager.h"
#include "../../include/component/basecomponent.h"
#include "../../include/component/rendercomponent.h"

// CLASS FORWARDING

// NAMESPACE

// DEFINES

/////////////////////////////////////////////////////////////////////////////////////////////

class MaterialSceneRayTrace: public Material
{
	friend class MaterialManager;

protected:
	/** Parameter constructor
	* @param [in] shader's name
	* @return nothing */
	MaterialSceneRayTrace(string &&name) : Material(move(name), move(string("MaterialSceneRayTrace")))
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
		void* rayGenShaderCode  = InputOutput::readFile("../data/vulkanshaders/sceneraytraceraygeneration.rgen", &shaderSize);
		void* rayMissShaderCode = InputOutput::readFile("../data/vulkanshaders/sceneraytraceraymiss.rmiss",      &shaderSize);
		void* rayHitShaderCode  = InputOutput::readFile("../data/vulkanshaders/sceneraytracerayhit.rchit",       &shaderSize);

		assert(rayGenShaderCode  != nullptr);
		assert(rayMissShaderCode != nullptr);
		assert(rayHitShaderCode  != nullptr);

		string stringRayGenShaderCode  = string((const char*)rayGenShaderCode);
		string stringRayMissShaderCode = string((const char*)rayMissShaderCode);
		string stringRayHitShaderCode  = string((const char*)rayHitShaderCode);

		m_shaderResourceName = "sceneraytrace";
		m_shader             = shaderM->buildShaderRayGenMissHit(move(string(m_shaderResourceName)), stringRayGenShaderCode.c_str(), stringRayMissShaderCode.c_str(), stringRayHitShaderCode.c_str(), m_materialSurfaceType);

		return true;
	}

	/** Exposes the variables that will be modified in the corresponding material data uniform buffer used in the scene,
	* allowing to work from variables from C++ without needing to update the values manually (error-prone)
	* @return nothing */
	void exposeResources()
	{
		assignImageToSampler(move(string("raytracingoffscreen")), move(string("raytracingoffscreen")), VK_DESCRIPTOR_TYPE_STORAGE_IMAGE);

		assignAccelerationStructure(move(string("raytracedaccelerationstructurecamera")), move(string("raytracedaccelerationstructurecamera")), VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR);

		vector<string> vectorIndexBufferName(m_vectorNode.size());
		vector<string> vectorVertexBufferName(m_vectorNode.size());

		forI(m_vectorNode.size())
		{
			const RenderComponent* renderComponent = m_vectorNode[i]->getRenderComponent();
			vectorIndexBufferName[i]               = renderComponent->getIndexBuffer()->getName();
			vectorVertexBufferName[i]              = renderComponent->getVertexBuffer()->getName();
		}

		Buffer* temp = bufferM->getElement(move(string("sceneDescriptionBufferCamera")));

		assert(temp != nullptr);

		assignShaderStorageBuffer(move(string("vertices")),               move(vectorVertexBufferName),           VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);
		assignShaderStorageBuffer(move(string("indices")),                move(vectorIndexBufferName),            VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);
		assignShaderStorageBuffer(move(string("sceneDescriptionBufferCamera")), move(string("sceneDescriptionBufferCamera")), VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);
	}

	SET(vectorNodePtr, m_vectorNode, VectorNode)

protected:
	vectorNodePtr m_vectorNode; //!< Vector with pointers to the nodes to be used in the ray tracing technique that uses this shader
};

/////////////////////////////////////////////////////////////////////////////////////////////

#endif _MATERIALSCENERAYTRACE_H_
