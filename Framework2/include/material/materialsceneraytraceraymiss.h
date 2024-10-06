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

#ifndef _MATERIALSCENERAYTRACEMISS_H_
#define _MATERIALSCENERAYTRACEMISS_H_

// GLOBAL INCLUDES

// PROJECT INCLUDES
#include "../../include/material/materialmanager.h"
#include "../../include/material/material.h"
#include "../../include/shader/shadermanager.h"
#include "../../include/shader/shader.h"

// CLASS FORWARDING

// NAMESPACE

// DEFINES

/////////////////////////////////////////////////////////////////////////////////////////////

class MaterialSceneRayTraceMiss : public Material
{
	friend class MaterialManager;

protected:
	/** Parameter constructor
	* @param [in] shader's name
	* @return nothing */
	MaterialSceneRayTraceMiss(string&& name) : Material(move(name), move(string("MaterialSceneRayTraceMiss")))
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
		void* rayMissShaderCode = InputOutput::readFile("../data/vulkanshaders/sceneraytraceraymiss.rmiss", &shaderSize);
		assert(rayMissShaderCode != nullptr);

		string shaderCode = string((const char*)rayMissShaderCode);
		m_shaderResourceName = "sceneraytracemiss";
		m_shader = shaderM->buildShaderRayMiss(move(string(m_shaderResourceName)), shaderCode.c_str(), m_materialSurfaceType);

		return true;
	}

	/** Exposes the variables that will be modified in the corresponding material data uniform buffer used in the scene,
	* allowing to work from variables from C++ without needing to update the values manually (error-prone)
	* @return nothing */
	void exposeResources()
	{

	}

protected:
};

/////////////////////////////////////////////////////////////////////////////////////////////

#endif _MATERIALSCENERAYTRACEMISS_H_
