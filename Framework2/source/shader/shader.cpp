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
#include "../../include/shader/shader.h"
#include "../../include/core/coremanager.h"
#include "../../include/texture/texture.h"
#include "../../include/texture/texturemanager.h"
#include "../../include/shader/sampler.h"
#include "../../include/shader/shaderstoragebuffer.h"
#include "../../include/buffer/buffer.h"
#include "../../include/buffer/buffermanager.h"
#include "../../include/util/containerutilities.h"
#include "../../include/shader/atomiccounterunit.h"
#include "../../include/shader/uniformBase.h"
#include "../../include/shader/shadertoplevelaccelerationstructure.h"
#include "../../include/accelerationstructure/accelerationstructure.h"
#include "../../include/accelerationstructure/accelerationstructuremanager.h"

// NAMESPACE

// DEFINES

// STATIC MEMBER INITIALIZATION

/////////////////////////////////////////////////////////////////////////////////////////////

Shader::Shader(string &&name, ShaderStageFlag shaderStageFlag) : GenericResource(move(name), move(string("Shader")), GenericResourceType::GRT_SHADER)
	, m_isCompute(false)
	, m_isRayTracing(false)
	, m_shaderStage(shaderStageFlag)
	, m_GLSLShaderVersion(GLSLShaderVersion::GLSLSV_4_5)
{

}

/////////////////////////////////////////////////////////////////////////////////////////////

Shader::~Shader()
{
	destroySamplers();
	deleteVectorInstances(m_vecUniformBase);
	deleteVectorInstances(m_vecDirtyUniform);
	deleteVectorInstances(m_vecAtomicCounterUnit);
	deleteVectorInstances(m_vecShaderStruct);
	deleteVectorInstances(m_vectorShaderStorageBuffer);
	deleteVectorInstances(m_vectorTLAS);

	destroyShaderStages();
}

/////////////////////////////////////////////////////////////////////////////////////////////

void Shader::init()
{

}

/////////////////////////////////////////////////////////////////////////////////////////////

void Shader::destroyShaderStages()
{
	forIT(m_arrayShaderStages)
	{
		vkDestroyShaderModule(coreM->getLogicalDevice(), it->module, NULL);
	}

	m_arrayShaderStages.clear();
}

/////////////////////////////////////////////////////////////////////////////////////////////

bool Shader::initializeSamplerHandlers()
{
	bool result = true;

	forIT(m_vecTextureSampler)
	{
		if ((*it)->getVectorTexture()[0] == nullptr)
		{
			result &= false;
		}

		(*it)->setSamplerHandle(textureM->buildSampler(0.0f, (float)((*it)->getVectorTexture()[0]->getMipMapLevels()), (*it)->getMipmapMode(), (*it)->getMinMagFilter()));
	}

	return result;
}
/////////////////////////////////////////////////////////////////////////////////////////////

bool Shader::initializeImageHandlers()
{
	bool result = true;

	forIT(m_vecImageSampler)
	{
		if ((*it)->getVectorTexture()[0] == nullptr)
		{
			cout << "ERROR no texture in Shader::initializeImageHandlers() for shader with name " << m_name << endl;
			result = false;
		}

		(*it)->setSamplerHandle(textureM->buildSampler(0.0f, (float)((*it)->getVectorTexture()[0]->getMipMapLevels()), (*it)->getMipmapMode(), (*it)->getMinMagFilter()));
	}

	return result;
}

/////////////////////////////////////////////////////////////////////////////////////////////

bool Shader::setTextureToSample(string &&textureSamplerName, string&& textureResourceName, VkDescriptorType descriptorType)
{
	return setTextureToSample(move(textureSamplerName), move(textureResourceName), descriptorType, VK_FILTER_LINEAR);
}

/////////////////////////////////////////////////////////////////////////////////////////////

bool Shader::setTextureToSample(string &&textureSamplerName, string&& textureResourceName, VkDescriptorType descriptorType, VkFilter filter)
{
	// TODO: refactor and merge with setImageToSample
	forIT(m_vecTextureSampler)
	{
		if ((*it)->getName() == textureSamplerName)
		{
			(*it)->setTextureToSample(move(string(textureResourceName)));
			(*it)->setDescriptorType(descriptorType);
			if (filter != VkFilter::VK_FILTER_MAX_ENUM)
			{
				(*it)->setMipmapMode(filter == VkFilter::VK_FILTER_LINEAR ? VkSamplerMipmapMode::VK_SAMPLER_MIPMAP_MODE_LINEAR : VkSamplerMipmapMode::VK_SAMPLER_MIPMAP_MODE_NEAREST);
				(*it)->setMinMagFilter(filter);
			}
			Texture* texture = textureM->getElement(move(textureResourceName));
			if (texture != nullptr)
			{
				(*it)->setTextureToSample(texture);
				(*it)->setReady(true);
				setReady(resourceIsReady());
			}
			else
			{
				(*it)->setReady(false);
				setReady(false);
			}
			return true;
		}
	}

	return false;
}


/////////////////////////////////////////////////////////////////////////////////////////////

bool Shader::setImageToSample(string &&imageSamplerName, string&& textureResourceName, VkDescriptorType descriptorType)
{
	return setImageToSample(move(imageSamplerName), move(textureResourceName), descriptorType, VkFilter::VK_FILTER_MAX_ENUM);
}

/////////////////////////////////////////////////////////////////////////////////////////////

bool Shader::setImageToSample(string &&imageSamplerName, string&& textureResourceName, VkDescriptorType descriptorType, VkFilter filter)
{
	// TODO: refactor and merge using the vector as parameter
	forIT(m_vecImageSampler)
	{
		if ((*it)->getName() == imageSamplerName)
		{
			(*it)->setTextureToSample(move(string(textureResourceName)));
			(*it)->setDescriptorType(descriptorType);
			if (filter != VkFilter::VK_FILTER_MAX_ENUM)
			{
				(*it)->setMipmapMode(filter == VkFilter::VK_FILTER_LINEAR ? VkSamplerMipmapMode::VK_SAMPLER_MIPMAP_MODE_LINEAR : VkSamplerMipmapMode::VK_SAMPLER_MIPMAP_MODE_NEAREST);
				(*it)->setMinMagFilter(filter);
			}
			Texture* texture = textureM->getElement(move(textureResourceName));
			if (texture != nullptr)
			{
				(*it)->setTextureToSample(texture);
				(*it)->setReady(true);
				setReady(resourceIsReady());
			}
			else
			{
				(*it)->setReady(false);
				setReady(false);
			}
			return true;
		}
	}

	return false;
}

/////////////////////////////////////////////////////////////////////////////////////////////

bool Shader::setCombinedImageSampler(string&& combinedImageSamplerName, vectorString&& vectorTextureResourceName)
{
	// TODO: refactor and merge using the vector as parameter
	// TODO: Start using IDs as the vector with texture names is taken from the textures searched again here
	forIT(m_vecTextureSampler)
	{
		if ((*it)->getSamplerType() != ResourceInternalType::RIT_COMBINED_IMAGE_SAMPLER)
		{
			continue;
		}

		(*it)->setDescriptorType(VkDescriptorType::VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);

		int numElement                  = int(vectorTextureResourceName.size());
		vectorTexturePtr& vectorTexture = (*it)->refVectorTexturePtr();

		vectorTexture.resize(numElement);
		
		forI(numElement)
		{
			vectorTexture[i] = textureM->getElement(move(string(vectorTextureResourceName[i])));

			if (vectorTexture[i] == nullptr)
			{
				(*it)->setReady(false);
				setReady(false);
				return false;
			}
		}

		(*it)->setVectorTextureToSampleName(move(vectorTextureResourceName));

		return true;
	}

	return false;
}

/////////////////////////////////////////////////////////////////////////////////////////////

const Sampler* Shader::getSamplerByName(string&& name) const
{
	forIT(m_vecTextureSampler)
	{
		if ((*it)->getName() == name)
		{
			return *it;
		}
	}

	return nullptr;
}

/////////////////////////////////////////////////////////////////////////////////////////////

bool Shader::setShaderStorageBuffer(string &&storageBufferName, string&& bufferResourceName, VkDescriptorType descriptorType)
{
	// TODO: refactor and merge using the vector as parameter
	forIT(m_vectorShaderStorageBuffer)
	{
		if ((*it)->getName() == storageBufferName)
		{
			(*it)->refVectorBufferName().push_back(move(string(bufferResourceName)));
			(*it)->setDescriptorType(descriptorType);
			Buffer* buffer = bufferM->getElement(move(string(bufferResourceName)));
			if (buffer != nullptr)
			{
				(*it)->refVectorBufferPtr().push_back(buffer);
				(*it)->setReady(true);
				setReady(resourceIsReady());
			}
			else
			{
				(*it)->setReady(false);
				setReady(false);
			}
			return true;
		}
	}

	return false;
}

/////////////////////////////////////////////////////////////////////////////////////////////

bool Shader::setShaderStorageBuffer(string&& storageBufferName, vector<string>&& vectorBufferResourceName, VkDescriptorType descriptorType)
{
	// TODO: refactor and merge using the vector as parameter
	forIT(m_vectorShaderStorageBuffer)
	{
		if ((*it)->getVariableName() == storageBufferName)
		{
			vectorBufferPtr& vectorBuffer = (*it)->refVectorBufferPtr();
			vectorString& vectorBufferName = (*it)->refVectorBufferName();
			vectorBuffer.resize(vectorBufferResourceName.size());
			vectorBufferName.resize(vectorBufferResourceName.size());
	
			bool anyBufferIsNull = false;
			forJ(vectorBufferResourceName.size())
			{
				vectorBufferName[j] = vectorBufferResourceName[j];
				Buffer* buffer = bufferM->getElement(move(string(vectorBufferResourceName[j])));

				if (buffer == nullptr)
				{
					vectorBuffer.clear();
					vectorBufferName.clear();
					anyBufferIsNull = true;
					break;
				}

				vectorBuffer[j] = buffer;
			}

			(*it)->setDescriptorType(descriptorType);
			if (!anyBufferIsNull)
			{
				(*it)->setReady(true);
				setReady(resourceIsReady());
			}
			else
			{
				(*it)->setReady(false);
				setReady(false);
			}
			return true;
		}
	}

	return false;
}

/////////////////////////////////////////////////////////////////////////////////////////////

bool Shader::setAccelerationStructureToRayTrace(string&& TLASName, string&& accelerationStructureResourceName, VkDescriptorType descriptorType)
{
	forIT(m_vectorTLAS)
	{
		if ((*it)->getName() == TLASName)
		{
			(*it)->setAccelerationStructureName(move(accelerationStructureResourceName));
			(*it)->setDescriptorType(descriptorType);
			AccelerationStructure* accelerationStructure = accelerationStructureM->getElement(move(accelerationStructureResourceName));
			if (accelerationStructure != nullptr)
			{
				(*it)->setAccelerationStructure(accelerationStructure);
				(*it)->setReady(true);
				setReady(resourceIsReady());
			}
			else
			{
				(*it)->setReady(false);
				setReady(false);
			}
			return true;
		}
	}

	return false;
}

/////////////////////////////////////////////////////////////////////////////////////////////

void Shader::destroySamplers()
{
	forI(m_vecTextureSampler.size())
	{
		if (m_vecTextureSampler[i]->getSamplerHandle() != VK_NULL_HANDLE)
		{
			vkDestroySampler(coreM->getLogicalDevice(), m_vecTextureSampler[i]->getSamplerHandle(), nullptr);
		}
	}

	forI(m_vecImageSampler.size())
	{
		if (m_vecImageSampler[i]->getSamplerHandle() != VK_NULL_HANDLE)
		{
			vkDestroySampler(coreM->getLogicalDevice(), m_vecImageSampler[i]->getSamplerHandle(), nullptr);
		}
	}
}

/////////////////////////////////////////////////////////////////////////////////////////////

void Shader::addHeaderSourceCode(string&& code)
{
	m_headerSourceCode += code;
}

/////////////////////////////////////////////////////////////////////////////////////////////

void Shader::buildShaderGroupInfo()
{
	uint numElement = static_cast<uint>(m_arrayShaderStages.size());
	m_vectorRayTracingShaderGroupCreateInfo.resize(numElement);

	forI(numElement)
	{
		VkRayTracingShaderGroupTypeKHR rayTracingShaderGroupType;
		switch (m_arrayShaderStages[i].stage)
		{
			case VK_SHADER_STAGE_RAYGEN_BIT_KHR:
			case VK_SHADER_STAGE_MISS_BIT_KHR:
			{
				rayTracingShaderGroupType = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR;
				break;
			}
			case VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR:
			{
				rayTracingShaderGroupType = VK_RAY_TRACING_SHADER_GROUP_TYPE_TRIANGLES_HIT_GROUP_KHR;
				break;
			}
			default:
			{
				cout << "ERROR: No stage identified in Shader::buildShaderGroupInfo" << endl;
				break;
			}
		}

		m_vectorRayTracingShaderGroupCreateInfo[i].sType                           = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR;
		m_vectorRayTracingShaderGroupCreateInfo[i].pNext                           = nullptr;
		m_vectorRayTracingShaderGroupCreateInfo[i].type                            = rayTracingShaderGroupType;
		m_vectorRayTracingShaderGroupCreateInfo[i].generalShader                   = (rayTracingShaderGroupType == VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR)             ? i : VK_SHADER_UNUSED_KHR;
		m_vectorRayTracingShaderGroupCreateInfo[i].closestHitShader                = (rayTracingShaderGroupType == VK_RAY_TRACING_SHADER_GROUP_TYPE_TRIANGLES_HIT_GROUP_KHR) ? i : VK_SHADER_UNUSED_KHR;
		m_vectorRayTracingShaderGroupCreateInfo[i].intersectionShader              = VK_SHADER_UNUSED_KHR;
		m_vectorRayTracingShaderGroupCreateInfo[i].anyHitShader                    = VK_SHADER_UNUSED_KHR;
		m_vectorRayTracingShaderGroupCreateInfo[i].pShaderGroupCaptureReplayHandle = nullptr;
	}
}

/////////////////////////////////////////////////////////////////////////////////////////////

bool Shader::resourceIsReady()
{
	// TODO: refactor and merge using the vector as parameter
	uint maxIndex = uint(m_vecTextureSampler.size());

	forI(maxIndex)
	{
		if (!m_vecTextureSampler[i]->getReady())
		{
			setReady(false);
			return false;
		}
	}

	maxIndex = uint(m_vecImageSampler.size());
	forI(maxIndex)
	{
		if (!m_vecImageSampler[i]->getReady())
		{
			setReady(false);
			return false;
		}
	}

	maxIndex = uint(m_vectorShaderStorageBuffer.size());
	forI(maxIndex)
	{
		if (!m_vectorShaderStorageBuffer[i]->getReady())
		{
			setReady(false);
			return false;
		}
	}

	return true;
}

/////////////////////////////////////////////////////////////////////////////////////////////

bool Shader::textureResourceNotification(string&& textureResourceName, ManagerNotificationType notificationType)
{
	bool resultTexture = textureResourceNotification(m_vecTextureSampler, move(string(textureResourceName)), notificationType);
	bool resultImage   = textureResourceNotification(m_vecImageSampler, move(textureResourceName), notificationType);

	return (resultTexture || resultImage);
}

/////////////////////////////////////////////////////////////////////////////////////////////

bool Shader::textureResourceNotification(vectorSamplerPtr& vectorResource, string&& textureResourceName, ManagerNotificationType notificationType)
{
	// TODO: find a way to refector with bufferResourceNotification
	
	bool result = false;
	bool builtCondition;

	switch (notificationType)
	{
		case ManagerNotificationType::MNT_ADDED:
		{
			builtCondition = false;
			break;
		}
		case ManagerNotificationType::MNT_REMOVED:
		{
			builtCondition = true;
			break;
		}
	}

	uint maxIndex = uint(vectorResource.size());
	Texture* texture;
	forI(maxIndex)
	{
		texture = vectorResource[i]->getVectorTexture()[0];

		if ((texture != nullptr) &&
			(vectorResource[i]->getReady() == builtCondition) &&
			(texture->getName() == textureResourceName))
		{
			switch (notificationType)
			{
				case ManagerNotificationType::MNT_ADDED:
				{
					vectorResource[i]->setTextureToSample(texture);
					vectorResource[i]->setReady(true);
					setReady(resourceIsReady());
					break;
				}
				case ManagerNotificationType::MNT_REMOVED:
				{
					vectorResource[i]->setTextureToSample(nullptr);
					vectorResource[i]->setReady(false);
					setReady(false);
					break;
				}
			}
			result = true;
		}
	}

	return result;
}

/////////////////////////////////////////////////////////////////////////////////////////////

bool Shader::bufferResourceNotification(string&& bufferResourceName, ManagerNotificationType notificationType)
{
	// TODO: find a way to refector with textureResourceNotification

	bool result = false;
	bool builtCondition;

	switch (notificationType)
	{
		case ManagerNotificationType::MNT_ADDED:
		{
			builtCondition = false;
			break;
		}
		case ManagerNotificationType::MNT_CHANGED:
		case ManagerNotificationType::MNT_REMOVED:
		{
			builtCondition = true;
			break;
		}
		default:
		{
			builtCondition = false;
			break;
		}
	}

	uint maxIndex = uint(m_vectorShaderStorageBuffer.size());

	Buffer* buffer;
	forI(maxIndex)
	{
		vectorBufferPtr& vectorBuffer = m_vectorShaderStorageBuffer[i]->refVectorBufferPtr();
		forJ(vectorBuffer.size())
		{
			buffer = vectorBuffer[j];

			if ((buffer != nullptr) &&
				(m_vectorShaderStorageBuffer[i]->getReady() == builtCondition) &&
				(buffer->getName() == bufferResourceName))
			{
				switch (notificationType)
				{
				case ManagerNotificationType::MNT_ADDED:
				case ManagerNotificationType::MNT_CHANGED:
				{
					vectorBuffer[j] = buffer;
					m_vectorShaderStorageBuffer[i]->setReady(true);
					setReady(resourceIsReady());
					break;
				}
				case ManagerNotificationType::MNT_REMOVED:
				{
					vectorBuffer[j] = nullptr;
					m_vectorShaderStorageBuffer[i]->setReady(false);
					setReady(false);
					break;
				}
				}

				result = true;
			}
		}
	}

	return result;
}

/////////////////////////////////////////////////////////////////////////////////////////////

void Shader::addShaderHeaderSourceCode(MaterialSurfaceType surfaceType)
{
	if (surfaceType == MaterialSurfaceType::MST_ALPHATESTED)
	{
		addHeaderSourceCode(move(string("#define MATERIAL_TYPE_ALPHATESTED 1\n")));
	}
}

/////////////////////////////////////////////////////////////////////////////////////////////
