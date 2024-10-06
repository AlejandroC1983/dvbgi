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
#include "../../include/shader/shadermanager.h"
#include "../../include/shader/shader.h"
#include "../../include/core/coremanager.h"
#include "../../include/shader/shaderreflection.h"
#include "../../include/parameter/attributedefines.h"
#include "../../include/texture/texture.h"
#include "../../include/texture/texturemanager.h"
#include "../../include/buffer/buffer.h"
#include "../../include/buffer/buffermanager.h"

// NAMESPACE
using namespace attributedefines;

// DEFINES

// STATIC MEMBER INITIALIZATION
uint ShaderManager::m_nextInstanceSuffix = 0;

/////////////////////////////////////////////////////////////////////////////////////////////

ShaderManager::ShaderManager()
{
	m_managerName = g_shaderManager;
}

/////////////////////////////////////////////////////////////////////////////////////////////

ShaderManager::~ShaderManager()
{
	destroyResources();
}

/////////////////////////////////////////////////////////////////////////////////////////////

void ShaderManager::destroyResources()
{
	forIT(m_mapElement)
	{
		delete it->second;
		it->second = nullptr;
	}
}

/////////////////////////////////////////////////////////////////////////////////////////////

void ShaderManager::obtainMaxPushConstantsSize()
{
	const VkPhysicalDeviceLimits& physicalDeviceLimits = coreM->getPhysicalDeviceProperties().limits;
	m_maxPushConstantsSize                             = physicalDeviceLimits.maxPushConstantsSize;
}

/////////////////////////////////////////////////////////////////////////////////////////////

void ShaderManager::addGlobalHeaderSourceCode(string&& code)
{
	m_globalHeaderSourceCode += code;
}

/////////////////////////////////////////////////////////////////////////////////////////////

uint ShaderManager::getNextInstanceSuffix()
{
	return m_nextInstanceSuffix++;
}

/////////////////////////////////////////////////////////////////////////////////////////////

Shader* ShaderManager::buildShaderV(string&& instanceName, const char *vertexShaderText, MaterialSurfaceType surfaceType)
{
	assert(vertexShaderText != nullptr);
	vectorCharPtr arrayShader = { vertexShaderText, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr };
	return buildShader(move(instanceName), arrayShader, surfaceType, ShaderStageFlag::SSF_VERTEX);
}

/////////////////////////////////////////////////////////////////////////////////////////////

Shader* ShaderManager::buildShaderVG(string&& instanceName, const char *vertexShaderText, const char *geometryShaderText, MaterialSurfaceType surfaceType)
{
	assert((vertexShaderText != nullptr) || (geometryShaderText != nullptr));
	vectorCharPtr arrayShader = { vertexShaderText, geometryShaderText, nullptr, nullptr, nullptr, nullptr, nullptr };
	return buildShader(move(instanceName), arrayShader, surfaceType, ShaderStageFlag::SSF_VERTEX | ShaderStageFlag::SSF_GEOMETRY);
}

/////////////////////////////////////////////////////////////////////////////////////////////

Shader* ShaderManager::buildShaderVF(string&& instanceName, const char *vertexShaderText, const char *fragmentShaderText, MaterialSurfaceType surfaceType)
{
	assert((vertexShaderText != nullptr) || (fragmentShaderText != nullptr));
	vectorCharPtr arrayShader = { vertexShaderText, nullptr, fragmentShaderText, nullptr, nullptr, nullptr, nullptr };
	return buildShader(move(instanceName), arrayShader, surfaceType, ShaderStageFlag::SSF_VERTEX | ShaderStageFlag::SSF_FRAGMENT);
}

/////////////////////////////////////////////////////////////////////////////////////////////

Shader* ShaderManager::buildShaderVGF(string&& instanceName, const char *vertexShaderText, const char *geometryShaderText, const char *fragmentShaderText, MaterialSurfaceType surfaceType)
{
	assert((vertexShaderText != nullptr) || (geometryShaderText != nullptr) || (fragmentShaderText != nullptr));
	vectorCharPtr arrayShader = { vertexShaderText, geometryShaderText, fragmentShaderText, nullptr, nullptr, nullptr, nullptr };
	return buildShader(move(instanceName), arrayShader, surfaceType, ShaderStageFlag::SSF_VERTEX | ShaderStageFlag::SSF_GEOMETRY | ShaderStageFlag::SSF_FRAGMENT);
}

/////////////////////////////////////////////////////////////////////////////////////////////

Shader* ShaderManager::buildShaderC(string&& instanceName, const char *computeShaderText, MaterialSurfaceType surfaceType)
{
	assert(computeShaderText != nullptr);
	vectorCharPtr arrayShader = { nullptr, nullptr, nullptr, computeShaderText, nullptr, nullptr, nullptr };
	return buildShader(move(instanceName), arrayShader, surfaceType, ShaderStageFlag::SSF_COMPUTE);
}

/////////////////////////////////////////////////////////////////////////////////////////////

Shader* ShaderManager::buildShaderRayGen(string&& instanceName, const char* rayGenShaderText, MaterialSurfaceType surfaceType)
{
	assert(rayGenShaderText != nullptr);
	vectorCharPtr arrayShader = { nullptr, nullptr, nullptr, nullptr, rayGenShaderText, nullptr, nullptr };
	return buildShader(move(instanceName), arrayShader, surfaceType, ShaderStageFlag::SSF_RAYGENERATION);
}

/////////////////////////////////////////////////////////////////////////////////////////////

Shader* ShaderManager::buildShaderRayHit(string&& instanceName, const char* rayHitShaderText, MaterialSurfaceType surfaceType)
{
	assert(rayHitShaderText != nullptr);
	vectorCharPtr arrayShader = { nullptr, nullptr, nullptr, nullptr, nullptr, rayHitShaderText, nullptr };
	return buildShader(move(instanceName), arrayShader, surfaceType, ShaderStageFlag::SSF_RAYHIT);
}

/////////////////////////////////////////////////////////////////////////////////////////////

Shader* ShaderManager::buildShaderRayMiss(string&& instanceName, const char* rayMissShaderText, MaterialSurfaceType surfaceType)
{
	assert(rayMissShaderText != nullptr);
	vectorCharPtr arrayShader = { nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, rayMissShaderText };
	return buildShader(move(instanceName), arrayShader, surfaceType, ShaderStageFlag::SSF_RAYMISS);
}

/////////////////////////////////////////////////////////////////////////////////////////////

Shader* ShaderManager::buildShaderRayGenMissHit(string&& instanceName, const char* rayGenShaderText, const char* rayMissShaderText, const char* rayHitShaderText, MaterialSurfaceType surfaceType)
{
	assert(rayGenShaderText != nullptr);
	assert(rayHitShaderText != nullptr);
	assert(rayMissShaderText != nullptr);
	vectorCharPtr arrayShader = { nullptr, nullptr, nullptr, nullptr, rayGenShaderText, rayMissShaderText, rayHitShaderText };
	return buildShader(move(instanceName), arrayShader, surfaceType, ShaderStageFlag::SSF_RAYGENERATION | ShaderStageFlag::SSF_RAYMISS | ShaderStageFlag::SSF_RAYHIT );
}

/////////////////////////////////////////////////////////////////////////////////////////////

Shader* ShaderManager::buildShader(string&& instanceName, vectorCharPtr arrayShader, MaterialSurfaceType surfaceType, ShaderStageFlag shaderStageFlag)
{
	if (existsElement(move(instanceName)))
	{
		return getElement(move(instanceName));
	}

	Shader* shader = new Shader(move(string(instanceName)), shaderStageFlag);
	shader->addShaderHeaderSourceCode(surfaceType);
	vector<vector<uint>> arraySPVShaderStage;
	shader->m_arrayShaderStages = buildShaderStages(arrayShader, shader->getHeaderSourceCode(), arraySPVShaderStage);

	if (shaderStageFlag == ShaderStageFlag::SSF_COMPUTE)
	{
		shader->m_isCompute = true;
	}

	if ((shaderStageFlag & ShaderStageFlag::SSF_RAYGENERATION) ||
		(shaderStageFlag & ShaderStageFlag::SSF_RAYMISS)       ||
		(shaderStageFlag & ShaderStageFlag::SSF_RAYHIT))
	{
		shader->m_isRayTracing = true;
	}
	
	forI(arraySPVShaderStage.size())
	{
		ShaderReflection::extractResources(move(arraySPVShaderStage[i]),
			shader->m_vecUniformBase,
			shader->m_vecTextureSampler,
			shader->m_vecImageSampler,
			shader->m_vecAtomicCounterUnit,
			shader->m_vecShaderStruct,
			shader->m_vectorShaderStorageBuffer,
			shader->m_vectorTLAS,
			&shader->m_pushConstant);
	}

	shader->init();
	if (shader->m_pushConstant.m_vecUniformBase.size() > 0)
	{
		shader->m_pushConstant.m_CPUBuffer.buildCPUBuffer(shader->m_pushConstant.m_vecUniformBase);
	}

	addElement(move(string(instanceName)), shader);
	shader->m_name = move(instanceName);

	return shader;
}

/////////////////////////////////////////////////////////////////////////////////////////////

bool ShaderManager::GLSLtoSPV(const VkShaderStageFlagBits shaderType, const char *pShader, vector<unsigned int> &spirv)
{
	glslang::TProgram* program = new glslang::TProgram;
	const char *shaderStrings[1];
	TBuiltInResource Resources;
	initializeResources(Resources);

	// Enable SPIR-V and Vulkan rules when parsing GLSL
	EShMessages messages = (EShMessages)(EShMsgSpvRules | EShMsgVulkanRules);

	EShLanguage stage = getLanguage(shaderType);
	glslang::TShader* shader = new glslang::TShader(stage);

	shaderStrings[0] = pShader;
	shader->setStrings(shaderStrings, 1);
	
	if ((shaderType == VkShaderStageFlagBits::VK_SHADER_STAGE_RAYGEN_BIT_KHR) ||
		(shaderType == VkShaderStageFlagBits::VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR) ||
		(shaderType == VkShaderStageFlagBits::VK_SHADER_STAGE_MISS_BIT_KHR))
	{
		shader->setEnvTarget(glslang::EShTargetLanguage::EShTargetSpv, glslang::EShTargetLanguageVersion::EShTargetSpv_1_5);
	}

	if (!shader->parse(&Resources, 100, false, messages))
	{
		puts(shader->getInfoLog());
		puts(shader->getInfoDebugLog());
		return false;
	}

	program->addShader(shader);

	// Link the program and report if errors...
	if (!program->link(messages))
	{
		puts(shader->getInfoLog());
		puts(shader->getInfoDebugLog());
		return false;
	}

	//-----------------------------------------
	// Pass this parameter to GlslangToSpv for debug shaders
	//glslang::SpvOptions spvOptions;
	//spvOptions.generateDebugInfo = true;
	//spvOptions.stripDebugInfo    = false;
	//spvOptions.disableOptimizer  = true;
	//spvOptions.optimizeSize      = true;
	//spvOptions.disassemble       = true;
	//spvOptions.validate          = true;
	//-----------------------------------------

	//glslang::GlslangToSpv(*program->getIntermediate(stage), spirv, &spvOptions);
	glslang::GlslangToSpv(*program->getIntermediate(stage), spirv);
	delete program;
	delete shader;
	return true;
}

/////////////////////////////////////////////////////////////////////////////////////////////

EShLanguage ShaderManager::getLanguage(const VkShaderStageFlagBits shaderType)
{
	switch (shaderType)
	{
		case VK_SHADER_STAGE_VERTEX_BIT:
		{
			return EShLangVertex;
		}
		case VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT:
		{
			return EShLangTessControl;
		}
		case VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT:
		{
			return EShLangTessEvaluation;
		}
		case VK_SHADER_STAGE_GEOMETRY_BIT:
		{
			return EShLangGeometry;
		}
		case VK_SHADER_STAGE_FRAGMENT_BIT:
		{
			return EShLangFragment;
		}
		case VK_SHADER_STAGE_COMPUTE_BIT:
		{
			return EShLangCompute;
		}
		case VK_SHADER_STAGE_RAYGEN_BIT_KHR:
		{
			return EShLangRayGen;
		}
		case VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR:
		{
			return EShLangClosestHit;
		}
		case VK_SHADER_STAGE_MISS_BIT_KHR:
		{
			return EShLangMiss;
		}
		default:
		{
			printf("Unknown shader type specified: %d. Exiting!", shaderType);
			exit(1);
		}
	}
}

/////////////////////////////////////////////////////////////////////////////////////////////

void ShaderManager::initializeResources(TBuiltInResource &Resources)
{
	// TODO: update with real data
	Resources.maxLights                                   = 32;
	Resources.maxClipPlanes                               = 6;
	Resources.maxTextureUnits                             = 32;
	Resources.maxTextureCoords                            = 32;
	Resources.maxVertexAttribs                            = 64;
	Resources.maxVertexUniformComponents                  = 4096;
	Resources.maxVaryingFloats                            = 64;
	Resources.maxVertexTextureImageUnits                  = 32;
	Resources.maxCombinedTextureImageUnits                = 80;
	Resources.maxTextureImageUnits                        = 32;
	Resources.maxFragmentUniformComponents                = 4096;
	Resources.maxDrawBuffers                              = 32;
	Resources.maxVertexUniformVectors                     = 128;
	Resources.maxVaryingVectors                           = 8;
	Resources.maxFragmentUniformVectors                   = 16;
	Resources.maxVertexOutputVectors                      = 16;
	Resources.maxFragmentInputVectors                     = 15;
	Resources.minProgramTexelOffset                       = -8;
	Resources.maxProgramTexelOffset                       = 7;
	Resources.maxClipDistances                            = 8;
	Resources.maxComputeWorkGroupCountX                   = 65535;
	Resources.maxComputeWorkGroupCountY                   = 65535;
	Resources.maxComputeWorkGroupCountZ                   = 65535;
	Resources.maxComputeWorkGroupSizeX                    = 1024;
	Resources.maxComputeWorkGroupSizeY                    = 1024;
	Resources.maxComputeWorkGroupSizeZ                    = 64;
	Resources.maxComputeUniformComponents                 = 1024;
	Resources.maxComputeTextureImageUnits                 = 16;
	Resources.maxComputeImageUniforms                     = 8;
	Resources.maxComputeAtomicCounters                    = 8;
	Resources.maxComputeAtomicCounterBuffers              = 1;
	Resources.maxVaryingComponents                        = 60;
	Resources.maxVertexOutputComponents                   = 64;
	Resources.maxGeometryInputComponents                  = 64;
	Resources.maxGeometryOutputComponents                 = 128;
	Resources.maxFragmentInputComponents                  = 128;
	Resources.maxImageUnits                               = 8;
	Resources.maxCombinedImageUnitsAndFragmentOutputs     = 8;
	Resources.maxCombinedShaderOutputResources            = 8;
	Resources.maxImageSamples                             = 0;
	Resources.maxVertexImageUniforms                      = 0;
	Resources.maxTessControlImageUniforms                 = 0;
	Resources.maxTessEvaluationImageUniforms              = 0;
	Resources.maxGeometryImageUniforms                    = 0;
	Resources.maxFragmentImageUniforms                    = 8;
	Resources.maxCombinedImageUniforms                    = 8;
	Resources.maxGeometryTextureImageUnits                = 16;
	Resources.maxGeometryOutputVertices                   = 256;
	Resources.maxGeometryTotalOutputComponents            = 1024;
	Resources.maxGeometryUniformComponents                = 1024;
	Resources.maxGeometryVaryingComponents                = 64;
	Resources.maxTessControlInputComponents               = 128;
	Resources.maxTessControlOutputComponents              = 128;
	Resources.maxTessControlTextureImageUnits             = 16;
	Resources.maxTessControlUniformComponents             = 1024;
	Resources.maxTessControlTotalOutputComponents         = 4096;
	Resources.maxTessEvaluationInputComponents            = 128;
	Resources.maxTessEvaluationOutputComponents           = 128;
	Resources.maxTessEvaluationTextureImageUnits          = 16;
	Resources.maxTessEvaluationUniformComponents          = 1024;
	Resources.maxTessPatchComponents                      = 120;
	Resources.maxPatchVertices                            = 32;
	Resources.maxTessGenLevel                             = 64;
	Resources.maxViewports                                = 16;
	Resources.maxVertexAtomicCounters                     = 0;
	Resources.maxTessControlAtomicCounters                = 0;
	Resources.maxTessEvaluationAtomicCounters             = 0;
	Resources.maxGeometryAtomicCounters                   = 0;
	Resources.maxFragmentAtomicCounters                   = 8;
	Resources.maxCombinedAtomicCounters                   = 8;
	Resources.maxAtomicCounterBindings                    = 1;
	Resources.maxVertexAtomicCounterBuffers               = 0;
	Resources.maxTessControlAtomicCounterBuffers          = 0;
	Resources.maxTessEvaluationAtomicCounterBuffers       = 0;
	Resources.maxGeometryAtomicCounterBuffers             = 0;
	Resources.maxFragmentAtomicCounterBuffers             = 1;
	Resources.maxCombinedAtomicCounterBuffers             = 1;
	Resources.maxAtomicCounterBufferSize                  = 16384;
	Resources.maxTransformFeedbackBuffers                 = 4;
	Resources.maxTransformFeedbackInterleavedComponents   = 64;
	Resources.maxCullDistances                            = 8;
	Resources.maxCombinedClipAndCullDistances             = 8;
	Resources.maxSamples                                  = 4;
	Resources.limits.nonInductiveForLoops                 = 1;
	Resources.limits.whileLoops                           = 1;
	Resources.limits.doWhileLoops                         = 1;
	Resources.limits.generalUniformIndexing               = 1;
	Resources.limits.generalAttributeMatrixVectorIndexing = 1;
	Resources.limits.generalVaryingIndexing               = 1;
	Resources.limits.generalSamplerIndexing               = 1;
	Resources.limits.generalVariableIndexing              = 1;
	Resources.limits.generalConstantMatrixVectorIndexing  = 1;
}

/////////////////////////////////////////////////////////////////////////////////////////////

bool ShaderManager::isRayTracingShader(VkShaderStageFlagBits shaderStageFlagBits)
{
	switch (shaderStageFlagBits)
	{
		case VK_SHADER_STAGE_RAYGEN_BIT_KHR:
		case VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR:
		case VK_SHADER_STAGE_MISS_BIT_KHR:
		{
			return true;
		}
	}

	return false;	
}

/////////////////////////////////////////////////////////////////////////////////////////////

VkShaderStageFlagBits ShaderManager::getShaderStage(uint index)
{
	switch (index)
	{
		case 0:
		{
			return VK_SHADER_STAGE_VERTEX_BIT;
		}
		case 1:
		{
			return VK_SHADER_STAGE_GEOMETRY_BIT;
		}
		case 2:
		{
			return VK_SHADER_STAGE_FRAGMENT_BIT;
		}
		case 3:
		{
			return VK_SHADER_STAGE_COMPUTE_BIT;
		}
		case 4:
		{
			return VK_SHADER_STAGE_RAYGEN_BIT_KHR;
		}
		case 5:
		{
			return VK_SHADER_STAGE_MISS_BIT_KHR;
		}
		case 6:
		{
			return VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR;
		}
	}

	return VK_SHADER_STAGE_FLAG_BITS_MAX_ENUM;
}

/////////////////////////////////////////////////////////////////////////////////////////////

vector<VkPipelineShaderStageCreateInfo> ShaderManager::buildShaderStages(const vectorCharPtr& arrayShader, const string& shaderHeaderSourceCode, vector<vector<uint>>& arraySPVShaderStage)
{
	//Index 0 is the vertex shader information
	//Index 1 is the geometry shader information
	//Index 2 is the fragment shader information
	//Index 3 is the compute shader information
	//Index 4 is the ray generation shader information
	//Index 5 is the ray miss shader information
	//Index 6 is the ray closest hit shader information
	vector<VkPipelineShaderStageCreateInfo> arrayShaderStage;

	string tempFullShaderSource;

	glslang::InitializeProcess();

	forI(arrayShader.size())
	{
		if (arrayShader[i] != nullptr)
		{
			vector<uint> arrayShaderStageSPV;
			VkPipelineShaderStageCreateInfo shaderStage = {};

			shaderStage.sType               = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
			shaderStage.pNext               = NULL;
			shaderStage.pSpecializationInfo = NULL;
			shaderStage.flags               = 0;
			shaderStage.stage               = getShaderStage(i); //(i == 0) ? VK_SHADER_STAGE_VERTEX_BIT : (i == 1) ? VK_SHADER_STAGE_GEOMETRY_BIT : (i == 2) ? VK_SHADER_STAGE_FRAGMENT_BIT : VK_SHADER_STAGE_COMPUTE_BIT;
			shaderStage.pName               = "main"; // Assuming the entry function is always "main()"

			tempFullShaderSource		    = "#version 460\n\n";
			tempFullShaderSource           += m_globalHeaderSourceCode;
			tempFullShaderSource           += shaderHeaderSourceCode;
			tempFullShaderSource           += arrayShader[i];
			bool retVal                     = GLSLtoSPV(shaderStage.stage, tempFullShaderSource.c_str(), arrayShaderStageSPV);

			if (!retVal)
			{
				outputSourceWithLineNumber(tempFullShaderSource);
			}

			assert(retVal);

			VkShaderModuleCreateInfo moduleCreateInfo;
			moduleCreateInfo.sType    = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
			moduleCreateInfo.pNext    = NULL;
			moduleCreateInfo.flags    = 0;
			moduleCreateInfo.codeSize = arrayShaderStageSPV.size() * sizeof(unsigned int);
			moduleCreateInfo.pCode    = arrayShaderStageSPV.data();

			VkResult result = vkCreateShaderModule(coreM->getLogicalDevice(), &moduleCreateInfo, NULL, &shaderStage.module);
			assert(result == VK_SUCCESS);

			arrayShaderStage.push_back(shaderStage);
			arraySPVShaderStage.push_back(arrayShaderStageSPV);
		}
	}

	glslang::FinalizeProcess();

	return arrayShaderStage;
}

/////////////////////////////////////////////////////////////////////////////////////////////

void ShaderManager::assignSlots()
{
	textureM->refElementSignal().connect<ShaderManager, &ShaderManager::slotElement>(this);
	bufferM->refElementSignal().connect<ShaderManager, &ShaderManager::slotElement>(this);
}

/////////////////////////////////////////////////////////////////////////////////////////////

void ShaderManager::slotElement(const char* managerName, string&& elementName, ManagerNotificationType notificationType)
{
	if (!gpuPipelineM->getPipelineInitialized())
	{
		return;
	}

	vectorShaderPtr vectorModifiedShader;

	if (managerName == g_textureManager)
	{
		map<string, Shader*>::iterator it = m_mapElement.begin();
		for (it; it != m_mapElement.end(); ++it)
		{
			if (it->second->textureResourceNotification(move(string(elementName)), notificationType))
			{
				vectorModifiedShader.push_back(it->second);
			}
		}
	}

	if (managerName == g_bufferManager)
	{
		map<string, Shader*>::iterator it = m_mapElement.begin();
		for (it; it != m_mapElement.end(); ++it)
		{
			if (it->second->bufferResourceNotification(move(string(elementName)), notificationType))
			{
				vectorModifiedShader.push_back(it->second);
			}
		}
	}

	if (gpuPipelineM->getPipelineInitialized())
	{
		forIT(vectorModifiedShader)
		{
			emitSignalElement(move(string((*it)->getName())), notificationType);
		}
	}
}

/////////////////////////////////////////////////////////////////////////////////////////////

void ShaderManager::outputSourceWithLineNumber(const string& sourceCode)
{
	string delimiter       = "\n";
	string::size_type pos  = 0;
	string::size_type prev = 0;
	uint counter           = 1;

	cout << "-----------------------------------------------------------\n";
	while ((pos = sourceCode.find(delimiter, prev)) != string::npos)
	{
		cout << counter;
		if (counter < 100)
		{
			cout << "   ";
		}
		else
		{
			cout << "  ";
		}

		cout << sourceCode.substr(prev, pos - prev) << endl;
		counter++;
		prev = pos + 1;
	}
	cout << "-----------------------------------------------------------\n";
}

/////////////////////////////////////////////////////////////////////////////////////////////
