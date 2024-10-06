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

// GLOBAL INCLUDES

// PROJECT INCLUDES
#include "../../include/material/materialmanager.h"
#include "../../include/material/material.h"
#include "../../include/material/materialcolortexture.h"
#include "../../include/material/materialscenevoxelization.h"
#include "../../include/material/materialdynamicscenevoxelization.h"
#include "../../include/material/materialbufferprefixsum.h"
#include "../../include/material/materiallighting.h"
#include "../../include/material/materialvoxelrasterinscenario.h"
#include "../../include/material/materialshadowmappingvoxel.h"
#include "../../include/material/materiallitvoxel.h"
#include "../../include/material/materiallightbouncevoxelirradiance.h"
#include "../../include/material/materialdistanceshadowmapping.h"
#include "../../include/material/materialantialiasing.h"
#include "../../include/material/materialcomputefrustumculling.h"
#include "../../include/material/materialindirectcolortexture.h"
#include "../../include/material/materialsceneraytrace.h"
#include "../../include/material/materialvoxelvisibilityraytracing.h"
#include "../../include/material/materialvoxelfaceindexstreamcompaction.h"
#include "../../include/material/materialdynamicvoxelcopytobuffer.h"
#include "../../include/material/materialdynamicvoxelvisibilityraytracing.h"
#include "../../include/material/materialcameravisibleraytracing.h"
#include "../../include/material/materialprocesscameravisibleresults.h"
#include "../../include/material/materiallightbouncedynamicvoxelirradiance.h"
#include "../../include/material/materiallightbouncedynamiccopyirradiance.h"
#include "../../include/material/materiallightbouncevoxel2dgaussianfilter.h"
#include "../../include/material/materialstaticneighbourinformation.h"
#include "../../include/material/materiallightbouncestaticvoxelpadding.h"
#include "../../include/material/materiallightbouncestaticvoxelfiltering.h"
#include "../../include/material/materialskeletalanimationupdate.h"
#include "../../include/material/materialtagcameravisiblevoxelneighbourtile.h"
#include "../../include/material/materialscenelightingdeferred.h"
#include "../../include/material/materialgbuffer.h"
#include "../../include/material/materialraytracingdeferredshadows.h"
#include "../../include/material/materialneighbourlitvoxelinformation.h"
#include "../../include/material/materialbuildstaticvoxeltilebuffer.h"
#include "../../include/material/materiallightbouncedynamicvoxelirradiancecompute.h"
#include "../../include/material/materialtemporalfilteringcleandynamic3dtextures.h"
#include "../../include/parameter/attributedefines.h"
#include "../../include/parameter/attributedata.h"
#include "../../include/uniformbuffer/uniformbuffer.h"
#include "../../include/uniformbuffer/uniformbuffermanager.h"

// NAMESPACE
using namespace attributedefines;

// DEFINES

// STATIC MEMBER INITIALIZATION

/////////////////////////////////////////////////////////////////////////////////////////////

MaterialManager::MaterialManager():
	m_materialsInitialized(false)
{
	m_managerName = g_materialManager;
}

/////////////////////////////////////////////////////////////////////////////////////////////

MaterialManager::~MaterialManager()
{
	destroyResources();
}

/////////////////////////////////////////////////////////////////////////////////////////////

void MaterialManager::destroyResources()
{
	forIT(m_mapElement)
	{
		delete it->second;
		it->second = nullptr;
	}
}

/////////////////////////////////////////////////////////////////////////////////////////////

Material* MaterialManager::buildMaterial(string&& className, string&& instanceName, MultiTypeUnorderedMap* attributeData)
{
	if (existsElement(move(instanceName)))
	{
		return getElement(move(instanceName));
	}

	// TODO: improve the automatization of this process
	Material* material = nullptr;
	if (className == "MaterialColorTexture")
	{
		material = new MaterialColorTexture(move(string(instanceName)));

		if (attributeData != nullptr)
		{
			MaterialColorTexture* materialCasted = static_cast<MaterialColorTexture*>(material);

			if (attributeData->elementExists(g_reflectanceTextureResourceNameHashed))
			{
				AttributeData<string>* attribute = attributeData->getElement<AttributeData<string>*>(g_reflectanceTextureResourceNameHashed);
				materialCasted->setReflectanceTextureName(attribute->m_data);
			}

			if (attributeData->elementExists(g_normalTextureResourceNameHashed))
			{
				AttributeData<string>* attribute = attributeData->getElement<AttributeData<string>*>(g_normalTextureResourceNameHashed);
				materialCasted->setNormalTextureName(attribute->m_data);
			}

			if (attributeData->elementExists(g_materialSurfaceTypeChunkHashed))
			{
				AttributeData<MaterialSurfaceType>* attribute = attributeData->getElement<AttributeData<MaterialSurfaceType>*>(g_materialSurfaceTypeChunkHashed);
				materialCasted->setMaterialSurfaceType(attribute->m_data);
			}
		}
	}
	else if (className == "MaterialSceneVoxelization")
	{
		material = new MaterialSceneVoxelization(move(string(instanceName)));

		if (attributeData != nullptr)
		{
			MaterialSceneVoxelization* materialCasted = static_cast<MaterialSceneVoxelization*>(material);

			if (attributeData->elementExists(g_reflectanceTextureResourceNameHashed))
			{
				AttributeData<string>* attribute = attributeData->getElement<AttributeData<string>*>(g_reflectanceTextureResourceNameHashed);
				materialCasted->setReflectanceTextureName(attribute->m_data);
			}

			if (attributeData->elementExists(g_normalTextureResourceNameHashed))
			{
				AttributeData<string>* attribute = attributeData->getElement<AttributeData<string>*>(g_normalTextureResourceNameHashed);
				materialCasted->setNormalTextureName(attribute->m_data);
			}
		}
	}
	else if (className == "MaterialDynamicSceneVoxelization")
	{
		material = new MaterialDynamicSceneVoxelization(move(string(instanceName)));

		if (attributeData != nullptr)
		{
			MaterialDynamicSceneVoxelization* materialCasted = static_cast<MaterialDynamicSceneVoxelization*>(material);

			if (attributeData->elementExists(g_reflectanceTextureResourceNameHashed))
			{
				AttributeData<string>* attribute = attributeData->getElement<AttributeData<string>*>(g_reflectanceTextureResourceNameHashed);
				materialCasted->setReflectanceTextureName(attribute->m_data);
			}

			if (attributeData->elementExists(g_normalTextureResourceNameHashed))
			{
				AttributeData<string>* attribute = attributeData->getElement<AttributeData<string>*>(g_normalTextureResourceNameHashed);
				materialCasted->setNormalTextureName(attribute->m_data);
			}
		}
	}
	else if (className == "MaterialBufferPrefixSum")
	{
		material = new MaterialBufferPrefixSum(move(string(instanceName)));
	}
	else if (className == "MaterialLighting")
	{
		material = new MaterialLighting(move(string(instanceName)));

		if (attributeData != nullptr)
		{
			MaterialLighting* materialCasted = static_cast<MaterialLighting*>(material);

			if (attributeData->elementExists(g_reflectanceTextureResourceNameHashed))
			{
				AttributeData<string>* attribute = attributeData->getElement<AttributeData<string>*>(g_reflectanceTextureResourceNameHashed);
				materialCasted->setReflectanceTextureName(attribute->m_data);
			}

			if (attributeData->elementExists(g_normalTextureResourceNameHashed))
			{
				AttributeData<string>* attribute = attributeData->getElement<AttributeData<string>*>(g_normalTextureResourceNameHashed);
				materialCasted->setNormalTextureName(attribute->m_data);
			}

			if (attributeData->elementExists(g_materialSurfaceTypeChunkHashed))
			{
				AttributeData<MaterialSurfaceType>* attribute = attributeData->getElement<AttributeData<MaterialSurfaceType>*>(g_materialSurfaceTypeChunkHashed);
				materialCasted->setMaterialSurfaceType(attribute->m_data);
			}
		}
	}
	else if (className == "MaterialVoxelRasterInScenario")
	{
		material = new MaterialVoxelRasterInScenario(move(string(instanceName)));
	}
	else if (className == "MaterialShadowMappingVoxel")
	{
		material = new MaterialShadowMappingVoxel(move(string(instanceName)));
	}
	else if (className == "MaterialLitVoxel")
	{
		material = new MaterialLitVoxel(move(string(instanceName)));

		if (attributeData != nullptr)
		{
			MaterialLitVoxel* materialCasted = static_cast<MaterialLitVoxel*>(material);

			if (attributeData->elementExists(g_litVoxelCodeChunkHashed))
			{
				AttributeData<string>* attribute = attributeData->getElement<AttributeData<string>*>(g_litVoxelCodeChunkHashed);
				materialCasted->setComputeShaderThreadMapping(attribute->m_data);
			}
		}
	}
	else if (className == "MaterialLightBounceVoxelIrradiance")
	{
		material = new MaterialLightBounceVoxelIrradiance(move(string(instanceName)));

		if (attributeData != nullptr)
		{
			MaterialLightBounceVoxelIrradiance* materialCasted = static_cast<MaterialLightBounceVoxelIrradiance*>(material);

			if (attributeData->elementExists(g_lightBounceVoxelIrradianceCodeChunkHashed))
			{
				AttributeData<string>* attribute = attributeData->getElement<AttributeData<string>*>(g_lightBounceVoxelIrradianceCodeChunkHashed);
				materialCasted->setComputeShaderThreadMapping(attribute->m_data);
			}
		}
	}
	else if (className == "MaterialDistanceShadowMapping")
	{
		material = new MaterialDistanceShadowMapping(move(string(instanceName)));
	}
	else if (className == "MaterialAntialiasing")
	{
		material = new MaterialAntialiasing(move(string(instanceName)));
	}
	else if (className == "MaterialComputeFrustumCulling")
	{
		material = new MaterialComputeFrustumCulling(move(string(instanceName)));

		if (attributeData != nullptr)
		{
			MaterialComputeFrustumCulling* materialCasted = static_cast<MaterialComputeFrustumCulling*>(material);

			if (attributeData->elementExists(g_computeFrustumCullingCodeChunkHashed))
			{
				AttributeData<string>* attribute = attributeData->getElement<AttributeData<string>*>(g_computeFrustumCullingCodeChunkHashed);
				materialCasted->setComputeShaderThreadMapping(attribute->m_data);
			}
		}
	}
	else if (className == "MaterialIndirectColorTexture")
	{
		material = new MaterialIndirectColorTexture(move(string(instanceName)));

		if (attributeData != nullptr)
		{
			MaterialIndirectColorTexture* materialCasted = static_cast<MaterialIndirectColorTexture*>(material);

			if (attributeData->elementExists(g_reflectanceTextureResourceNameHashed))
			{
				AttributeData<string>* attribute = attributeData->getElement<AttributeData<string>*>(g_reflectanceTextureResourceNameHashed);
				materialCasted->setReflectanceTextureName(attribute->m_data);
			}

			if (attributeData->elementExists(g_normalTextureResourceNameHashed))
			{
				AttributeData<string>* attribute = attributeData->getElement<AttributeData<string>*>(g_normalTextureResourceNameHashed);
				materialCasted->setNormalTextureName(attribute->m_data);
			}

			if (attributeData->elementExists(g_materialSurfaceTypeChunkHashed))
			{
				AttributeData<MaterialSurfaceType>* attribute = attributeData->getElement<AttributeData<MaterialSurfaceType>*>(g_materialSurfaceTypeChunkHashed);
				materialCasted->setMaterialSurfaceType(attribute->m_data);
			}
		}
	}
	else if (className == "MaterialSceneRayTrace")
	{
		material = new MaterialSceneRayTrace(move(string(instanceName)));

		if (attributeData != nullptr)
		{
			MaterialSceneRayTrace* materialCasted = static_cast<MaterialSceneRayTrace*>(material);

			if (attributeData->elementExists(g_rayTraceVectorSceneNodeHashed))
			{
				AttributeData<vector<Node*>>* attribute = attributeData->getElement<AttributeData<vector<Node*>>*>(g_rayTraceVectorSceneNodeHashed);
				materialCasted->setVectorNode(attribute->m_data);
			}
		}
	}
	else if (className == "MaterialVoxelVisibilityRayTracing")
	{
		material = new MaterialVoxelVisibilityRayTracing(move(string(instanceName)));
	}
	else if (className == "MaterialVoxelFaceIndexStreamCompaction")
	{
		material = new MaterialVoxelFaceIndexStreamCompaction(move(string(instanceName)));

		if (attributeData != nullptr)
		{
			MaterialVoxelFaceIndexStreamCompaction* materialCasted = static_cast<MaterialVoxelFaceIndexStreamCompaction*>(material);

			if (attributeData->elementExists(g_voxelFaceIndexStreamCompactionCodeChunkHashed))
			{
				AttributeData<string>* attribute = attributeData->getElement<AttributeData<string>*>(g_voxelFaceIndexStreamCompactionCodeChunkHashed);
				materialCasted->setComputeShaderThreadMapping(attribute->m_data);
			}
		}
	}
	else if (className == "MaterialDynamicVoxelCopyToBuffer")
	{
		material = new MaterialDynamicVoxelCopyToBuffer(move(string(instanceName)));

		if (attributeData != nullptr)
		{
			MaterialDynamicVoxelCopyToBuffer* materialCasted = static_cast<MaterialDynamicVoxelCopyToBuffer*>(material);

			if (attributeData->elementExists(g_shaderCodeChunkHashed))
			{
				AttributeData<string>* attribute = attributeData->getElement<AttributeData<string>*>(g_shaderCodeChunkHashed);
				materialCasted->setComputeShaderThreadMapping(attribute->m_data);
			}
		}
	}
	else if (className == "MaterialDynamicVoxelVisibilityRayTracing")
	{
		material = new MaterialDynamicVoxelVisibilityRayTracing(move(string(instanceName)));

		/*if (attributeData != nullptr)
		{
			MaterialVoxelVisibilityRayTracing* materialCasted = static_cast<MaterialVoxelVisibilityRayTracing*>(material);

			if (attributeData->elementExists(g_rayTraceVectorSceneNodeHashed))
			{
				AttributeData<vector<Node*>>* attribute = attributeData->getElement<AttributeData<vector<Node*>>*>(g_rayTraceVectorSceneNodeHashed);
				materialCasted->setVectorNode(attribute->m_data);
			}
		}*/
	}
	else if (className == "MaterialCameraVisibleRayTracing")
	{
		material = new MaterialCameraVisibleRayTracing(move(string(instanceName)));
	}
	else if (className == "MaterialProcessCameraVisibleResults")
	{
		material = new MaterialProcessCameraVisibleResults(move(string(instanceName)));
	}
	else if (className == "MaterialLightBounceDynamicVoxelIrradiance")
	{
		material = new MaterialLightBounceDynamicVoxelIrradiance(move(string(instanceName)));
	}
	else if (className == "MaterialLightBounceDynamicCopyIrradiance")
	{
		material = new MaterialLightBounceDynamicCopyIrradiance(move(string(instanceName)));
	}
	else if (className == "MaterialLightBounceVoxel2DFilter")
	{
		material = new MaterialLightBounceVoxel2DFilter(move(string(instanceName)));
	}
	else if (className == "MaterialStaticNeighbourInformation")
	{
		material = new MaterialStaticNeighbourInformation(move(string(instanceName)));
	}
	else if (className == "MaterialLightBounceStaticVoxelPadding")
	{
		material = new MaterialLightBounceStaticVoxelPadding(move(string(instanceName)));
	}
	else if (className == "MaterialLightBounceStaticVoxelFiltering")
	{
		material = new MaterialLightBounceStaticVoxelFiltering(move(string(instanceName)));
	}
	else if (className == "MaterialSkeletalAnimationUpdate")
	{
		material = new MaterialSkeletalAnimationUpdate(move(string(instanceName)));

		if (attributeData != nullptr)
		{
			MaterialSkeletalAnimationUpdate* materialCasted = static_cast<MaterialSkeletalAnimationUpdate*>(material);

			if (attributeData->elementExists(g_shaderCodeChunkHashed))
			{
				AttributeData<string>* attribute = attributeData->getElement<AttributeData<string>*>(g_shaderCodeChunkHashed);
				materialCasted->setComputeShaderThreadMapping(attribute->m_data);
			}
		}
	}
	else if (className == "MaterialTagCameraVisibleVoxelNeighbourTile")
	{
		material = new MaterialTagCameraVisibleVoxelNeighbourTile(move(string(instanceName)));
	}
	else if (className == "MaterialSceneLightingDeferred")
	{
		material = new MaterialSceneLightingDeferred(move(string(instanceName)));
	}
	else if (className == "MaterialGBuffer")
	{
		material = new MaterialGBuffer(move(string(instanceName)));

		if (attributeData != nullptr)
		{
			MaterialGBuffer* materialCasted = static_cast<MaterialGBuffer*>(material);

			if (attributeData->elementExists(g_reflectanceTextureResourceNameHashed))
			{
				AttributeData<string>* attribute = attributeData->getElement<AttributeData<string>*>(g_reflectanceTextureResourceNameHashed);
				materialCasted->setReflectanceTextureName(attribute->m_data);
			}

			if (attributeData->elementExists(g_normalTextureResourceNameHashed))
			{
				AttributeData<string>* attribute = attributeData->getElement<AttributeData<string>*>(g_normalTextureResourceNameHashed);
				materialCasted->setNormalTextureName(attribute->m_data);
			}
		}
	}
	else if (className == "MaterialRayTracingDeferredShadows")
	{
		material = new MaterialRayTracingDeferredShadows(move(string(instanceName)));
	}
	else if (className == "MaterialNeighbourLitVoxelInformation")
	{
		material = new MaterialNeighbourLitVoxelInformation(move(string(instanceName)));
	}
	else if (className == "MaterialBuildStaticVoxelTileBuffer")
	{
		material = new MaterialBuildStaticVoxelTileBuffer(move(string(instanceName)));
	}
	else if (className == "MaterialLightBounceDynamicVoxelIrradianceCompute")
	{
		material = new MaterialLightBounceDynamicVoxelIrradianceCompute(move(string(instanceName)));
	}
	else if (className == "MaterialTemporalFilteringCleanDynamic3DTextures")
	{
		material = new MaterialTemporalFilteringCleanDynamic3DTextures(move(string(instanceName)));
	}

	addElement(move(string(instanceName)), material);
	material->m_name = move(instanceName);
	material->m_ready = true;

	return material;
}

/////////////////////////////////////////////////////////////////////////////////////////////

void MaterialManager::assignSlots()
{
	shaderM->refElementSignal().connect<MaterialManager, &MaterialManager::slotElement>(this);
}

/////////////////////////////////////////////////////////////////////////////////////////////

void MaterialManager::slotElement(const char* managerName, string&& elementName, ManagerNotificationType notificationType)
{
	if (!gpuPipelineM->getPipelineInitialized())
	{
		return;
	}

	vector<Material*> vectorMaterial;

	if (managerName == g_shaderManager)
	{
		Material* material;
		map<string, Material*>::iterator it = m_mapElement.begin();
		for (it; it != m_mapElement.end(); ++it)
		{
			material = it->second;
			if (material->shaderResourceNotification(move(string(elementName)), notificationType))
			{
				vectorMaterial.push_back(material);
			}
		}
	}

	if (gpuPipelineM->getPipelineInitialized())
	{
		forIT(vectorMaterial)
		{
			emitSignalElement(move(string((*it)->getName())), notificationType);
		}
	}
}

/////////////////////////////////////////////////////////////////////////////////////////////

void MaterialManager::buildMaterialUniformBuffer()
{
	int maxSize = -1;
	int counter = 0;
	forIT(m_mapElement)
	{
		if ((*it).second->getExposedStructFieldSize() > maxSize)
		{
			maxSize = (*it).second->getExposedStructFieldSize();
		}

		(*it).second->setMaterialUniformBufferIndex(counter);
		counter++;
	}

	m_materialUniformData         = uniformBufferM->buildUniformBuffer(move(string("materialUniformBuffer")), maxSize, int(m_mapElement.size()));
	m_materialUBDynamicAllignment = uint(m_materialUniformData->getDynamicAllignment());

	forIT(m_mapElement)
	{
		updateGPUBufferMaterialData((*it).second);
	}
}

/////////////////////////////////////////////////////////////////////////////////////////////

void MaterialManager::updateGPUBufferMaterialData(Material* materialToUpdate)
{
	void *data;
	uint32_t materialOffset = uint32_t(materialToUpdate->getMaterialUniformBufferIndex() * m_materialUBDynamicAllignment);
	VkResult result         = vkMapMemory(coreM->getLogicalDevice(), m_materialUniformData->refBufferInstance()->getMemory(), materialOffset, m_materialUBDynamicAllignment, 0, &data);

	assert(result == VK_SUCCESS);

	uint8_t* cpuBufferSourceData = static_cast<uint8_t*>(m_materialUniformData->refCPUBuffer().refUBHostMemory());
	cpuBufferSourceData         += materialOffset;
	memcpy(data, cpuBufferSourceData, m_materialUBDynamicAllignment);

	vkUnmapMemory(coreM->getLogicalDevice(), m_materialUniformData->refBufferInstance()->getMemory());
}

/////////////////////////////////////////////////////////////////////////////////////////////

void MaterialManager::initAllMaterials()
{
	forIT(m_mapElement)
	{
		it->second->init();
	}

	m_materialsInitialized = true;
}

/////////////////////////////////////////////////////////////////////////////////////////////
