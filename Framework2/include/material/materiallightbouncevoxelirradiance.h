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

#ifndef _MATERIALLIGHTBOUNCEVOXELIRRADIANCE_H_
#define _MATERIALLIGHTBOUNCEVOXELIRRADIANCE_H_

// GLOBAL INCLUDES

// PROJECT INCLUDES
#include "../../include/material/materialmanager.h"
#include "../../include/material/material.h"
#include "../../include/shader/shadermanager.h"
#include "../../include/shader/shader.h"
#include "../../include/buffer/buffermanager.h"
#include "../../include/buffer/buffer.h"
#include "../../include/core/gpupipeline.h"

// CLASS FORWARDING

// NAMESPACE

// DEFINES

/////////////////////////////////////////////////////////////////////////////////////////////

class MaterialLightBounceVoxelIrradiance : public Material
{
	friend class MaterialManager;

protected:
	/** Parameter constructor
	* @param [in] shader's name
	* @return nothing */
	MaterialLightBounceVoxelIrradiance(string &&name) : Material(move(name), move(string("MaterialLightBounceVoxelIrradiance")))
		, m_localWorkGroupsXDimension(0)
		, m_localWorkGroupsYDimension(0)
		, m_numThreadExecuted(0)
		, m_numberThreadPerElement(0)
		, m_lightPosition(vec4(0.0f, 0.0f, 0.0f, float(gpuPipelineM->getRasterFlagValue(move(string("FORM_FACTOR_VOXEL_TO_VOXEL_ADDED"))))))
		//, m_formFactorVoxelToVoxelAdded()
	{
		m_resourcesUsed = MaterialBufferResource::MBR_MATERIAL;
	}

public:
	/** Loads the shader to be used. This could be substituted
	* by reading from a file, where the shader details would be present
	* @return true if the shader was loaded successfully, and false otherwise */
	bool loadShader()
	{
		size_t sizeFirstPart;
		size_t sizeSecondPart;

		void* shaderFirstPartCode  = InputOutput::readFile("../data/vulkanshaders/lightbouncevoxelirradiancefirstpart.comp", &sizeFirstPart);
		void* shaderSecondPartCode = InputOutput::readFile("../data/vulkanshaders/lightbouncevoxelirradiancesecondpart.comp", &sizeSecondPart);

		assert(shaderFirstPartCode  != nullptr);
		assert(shaderSecondPartCode != nullptr);

		string firstPart     = string((const char*)shaderFirstPartCode);
		string secondPart    = string((const char*)shaderSecondPartCode);
		string shaderCode    = firstPart;
		shaderCode          += m_computeShaderThreadMapping;
		shaderCode          += secondPart;
		m_shaderResourceName = "lightbouncevoxelirradiance";
		m_shader             = shaderM->buildShaderC(move(string(m_shaderResourceName)), shaderCode.c_str(), m_materialSurfaceType);

		return true;
	}

	/** Exposes the variables that will be modified in the corresponding material data uniform buffer used in the scene,
	* allowing to work from variables from C++ without needing to update the values manually (error-prone)
	* @return nothing */
	void exposeResources()
	{
		exposeStructField(ResourceInternalType::RIT_UNSIGNED_INT, (void*)(&m_localWorkGroupsXDimension),   move(string("myMaterialData")), move(string("localWorkGroupsXDimension")));
		exposeStructField(ResourceInternalType::RIT_UNSIGNED_INT, (void*)(&m_localWorkGroupsYDimension),   move(string("myMaterialData")), move(string("localWorkGroupsYDimension")));
		exposeStructField(ResourceInternalType::RIT_UNSIGNED_INT, (void*)(&m_numThreadExecuted),           move(string("myMaterialData")), move(string("numThreadExecuted")));
		exposeStructField(ResourceInternalType::RIT_UNSIGNED_INT, (void*)(&m_numberThreadPerElement),      move(string("myMaterialData")), move(string("numberThreadPerElement")));
		exposeStructField(ResourceInternalType::RIT_FLOAT_VEC4,   (void*)(&m_sceneMinAndNumberVoxel),      move(string("myMaterialData")), move(string("sceneMinAndNumberVoxel")));
		exposeStructField(ResourceInternalType::RIT_FLOAT_VEC4,   (void*)(&m_sceneExtentAndVoxelSize),     move(string("myMaterialData")), move(string("sceneExtentAndVoxelSize")));
		exposeStructField(ResourceInternalType::RIT_FLOAT_VEC4,   (void*)(&m_lightPosition),               move(string("myMaterialData")), move(string("lightPosition")));
		exposeStructField(ResourceInternalType::RIT_FLOAT_VEC4,   (void*)(&m_lightForwardEmitterRadiance), move(string("myMaterialData")), move(string("lightForwardEmitterRadiance")));
	
		assignShaderStorageBuffer(move(string("voxelOccupiedBuffer")),                move(string("voxelOccupiedBuffer")),                VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);
		assignShaderStorageBuffer(move(string("lightBounceVoxelDebugBuffer")),        move(string("lightBounceVoxelDebugBuffer")),        VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);
		assignShaderStorageBuffer(move(string("lightBounceVoxelIrradianceBuffer")),   move(string("lightBounceVoxelIrradianceBuffer")),   VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);
		assignShaderStorageBuffer(move(string("cameraVisibleVoxelCompactedBuffer")),  move(string("cameraVisibleVoxelCompactedBuffer")),  VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);
		assignShaderStorageBuffer(move(string("lightBounceProcessedVoxelBuffer")),    move(string("lightBounceProcessedVoxelBuffer")),    VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);
		assignShaderStorageBuffer(move(string("voxelOccupiedDynamicBuffer")),         move(string("voxelOccupiedDynamicBuffer")),         VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);
		assignShaderStorageBuffer(move(string("dynamicVoxelVisibilityFlagBuffer")),   move(string("dynamicVoxelVisibilityFlagBuffer")),   VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);
		assignShaderStorageBuffer(move(string("litTestVoxelPerByteBuffer")),          move(string("litTestVoxelPerByteBuffer")),          VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);
		assignShaderStorageBuffer(move(string("voxelVisibilityDynamic4ByteBuffer")),  move(string("voxelVisibilityDynamic4ByteBuffer")),  VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);
		assignShaderStorageBuffer(move(string("rayDirectionBuffer")),                 move(string("rayDirectionBuffer")),                 VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);
		assignShaderStorageBuffer(move(string("voxelVisibility4BytesBuffer")),        move(string("voxelVisibility4BytesBuffer")),        VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);
		assignShaderStorageBuffer(move(string("triangulatedIndicesBuffer")),          move(string("triangulatedIndicesBuffer")),          VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);
		assignShaderStorageBuffer(move(string("neighbourLitVoxelInformationBuffer")), move(string("neighbourLitVoxelInformationBuffer")), VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);
		assignShaderStorageBuffer(move(string("debugCounterBuffer")),                 move(string("debugCounterBuffer")),                 VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);
		assignShaderStorageBuffer(move(string("staticIrradianceTrackingBuffer")),     move(string("staticIrradianceTrackingBuffer")),     VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);

		assignImageToSampler(move(string("voxelizationReflectance")),               move(string("voxelizationReflectance")),               VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_FILTER_LINEAR);
		assignImageToSampler(move(string("dynamicVoxelizationReflectanceTexture")), move(string("dynamicVoxelizationReflectanceTexture")), VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_FILTER_LINEAR);
		assignImageToSampler(move(string("irradiance3DStaticNegativeX")),           move(string("irradiance3DStaticNegativeX")),           VK_DESCRIPTOR_TYPE_STORAGE_IMAGE);
		assignImageToSampler(move(string("irradiance3DStaticPositiveX")),           move(string("irradiance3DStaticPositiveX")),           VK_DESCRIPTOR_TYPE_STORAGE_IMAGE);
		assignImageToSampler(move(string("irradiance3DStaticNegativeY")),           move(string("irradiance3DStaticNegativeY")),           VK_DESCRIPTOR_TYPE_STORAGE_IMAGE);
		assignImageToSampler(move(string("irradiance3DStaticPositiveY")),           move(string("irradiance3DStaticPositiveY")),           VK_DESCRIPTOR_TYPE_STORAGE_IMAGE);
		assignImageToSampler(move(string("irradiance3DStaticNegativeZ")),           move(string("irradiance3DStaticNegativeZ")),           VK_DESCRIPTOR_TYPE_STORAGE_IMAGE);
		assignImageToSampler(move(string("irradiance3DStaticPositiveZ")),           move(string("irradiance3DStaticPositiveZ")),           VK_DESCRIPTOR_TYPE_STORAGE_IMAGE);

		assignTextureToSampler(move(string("staticVoxelIndexTexture")),  move(string("staticVoxelIndexTexture")),  VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
		assignTextureToSampler(move(string("staticVoxelNeighbourInfo")), move(string("staticVoxelNeighbourInfo")), VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);

		assignAccelerationStructure(move(string("raytracedaccelerationstructure")), move(string("raytracedaccelerationstructure")), VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR);
	}

	SET(string, m_computeShaderThreadMapping, ComputeShaderThreadMapping)
	GETCOPY_SET(uint, m_localWorkGroupsXDimension, LocalWorkGroupsXDimension)
	GETCOPY_SET(uint, m_localWorkGroupsYDimension, LocalWorkGroupsYDimension)
	SET(uint, m_numThreadExecuted, NumThreadExecuted)
	SET(uint, m_numberThreadPerElement, NumberThreadPerElement)
	SET(vec4, m_sceneMinAndNumberVoxel, SceneMinAndNumberVoxel)
	SET(vec4, m_sceneExtentAndVoxelSize, SceneExtentAndVoxelSize)
	GETCOPY_SET(vec4, m_lightPosition, LightPosition)
	SET(vec4, m_lightForwardEmitterRadiance, LightForwardEmitterRadiance)

protected:
	string m_computeShaderThreadMapping;  //!< String with the thread mapping taking into account m_bufferNumElement, m_numElementPerLocalWorkgroupThread and m_numThreadPerLocalWorkgroup values (and physical device limits)
	uint   m_localWorkGroupsXDimension;   //!< Number of dispatched local workgroups in the x dimension
	uint   m_localWorkGroupsYDimension;   //!< Number of dispatched local workgroups in the y dimension
	uint   m_numThreadExecuted;           //!< Number of thread executed
	uint   m_numberThreadPerElement;      //!< Number of threads that process each element
	vec4   m_sceneMinAndNumberVoxel;      //!< Minimum value of the scene's aabb in xyz coordinates, number of occupied voxels
	vec4   m_sceneExtentAndVoxelSize;     //!< Extent of the scene in the xyz coordinates, voxelization texture size in the w coordinate
	vec4   m_lightPosition;               //!< Light position in world coordinates
	uint   m_dummyVariable;               //!< Variable to have four 4-byte aligned values
	vec4   m_lightForwardEmitterRadiance; //!< Light forward direction in xyz components, emitter radiance in w component
};

/////////////////////////////////////////////////////////////////////////////////////////////

#endif _MATERIALLIGHTBOUNCEVOXELIRRADIANCE_H_
