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

#ifndef _SHADER_REFLECTION_H_
#define _SHADER_REFLECTION_H_

// GLOBAL INCLUDES
#include "../../spirv-cross/spirv.hpp"
#include "../../spirv-cross/spirv_glsl.hpp"
#include "../../spirv-cross/spirv_common.hpp"

// PROJECT INCLUDES
#include "../headers.h"
#include "../../include/shader/resourceenum.h"
#include "../../include/material/exposedstructfield.h"

// CLASS FORWARDING
class UniformBase;
class ExposedStructField;
class UniformBuffer;
class CPUBuffer;
class PushConstant;

// NAMESPACE
using namespace resourceenum;
using namespace spv;

// DEFINES

/////////////////////////////////////////////////////////////////////////////////////////////

class ShaderReflection
{
public:
	/** Takes a shader in spir-v binary format (shaderBinary) and through relfection through spirv-cross library,
	* extracts all the needed information to know all resources in the shader that can be managed from the C++ side 
	* @param shaderBinary         [in]    Shader in spir-v binary form
	* @param vecUniformBase       [inout] Vector with the uniforms present in each struct of each uniform buffer in the shader
	* @param vecSampler           [inout] Vector with the sampler information of the shader
	* @param vecImage             [inout] Vector with the image information of the shader
	* @param vecAtomicCounterUnit [inout] Vector with the atomic counter unit information of the shader
	* @param vecUniformBuffer     [inout] Vector with the uniform buffer information of the shader
	* @param vectorStorageBuffer  [inout] Vector with the storage buffer information of the shader
	* @param vectorTLAS           [inout] Vector with the top level acceleration structure information of the shader
	* @param pushConstant         [inout] Push constant to put information (if any) from the shader
	* @return nothing */
	static void extractResources(vector<uint32_t>&&                            shaderBinary,
	                             vectorUniformBasePtr&                         vecUniformBase,
	                             vectorSamplerPtr&                             vecSampler,
	                             vectorSamplerPtr&                             vecImage,
	                             vectorAtomicCounterUnitPtr&                   vecAtomicCounterUnit,
	                             vectorShaderStructPtr&                        vecShaderStruct,
								 vectorShaderStorageBufferPtr&                 vectorShaderStorageBuffer,
								 vectorShaderTopLevelAccelerationStructurePtr& vectorTLAS,
								 PushConstant*                                 pushConstant);

	/** Returns the ResourceInternalType equilavent to the string given as value, if present in arrayResourceKeywords
	* @param value [in] value to look the corresponding enum for
	* @return equilavent to the string given as value if any, UIT_SIZE otherwise */
	static ResourceInternalType stringTypeToEnum(string&& value);

	/** Returns the ImageInternalFormat equilavent to the string given as value, if present in arrayImageFormatQualifierKeywords
	* @param value [in] value to look the corresponding enum for
	* @return equilavent to the string given as value if any, IIF_SIZE otherwise */
	static ImageInternalFormat stringImageFormatQualifierToEnum(string&& value);

	/** Returns true if the value of the parameter corresponds to a shader struct variable
	* @param type [in] value to test
	* @return true if the type of the parameter is a uniform and false otherwise */
	static bool isStruct(const ResourceInternalType &type);

	/** Returns true if the value of the parameter corresponds to a shader data type (float, int, vec3, etc) variable
	* @param type [in] value to test
	* @return true if the type of the parameter is a data type and false otherwise */
	static bool isDataType(const ResourceInternalType &type);

	/** Returns true if the value of the parameter corresponds to a shader sampler variable
	* @param type [in] value to test
	* @return true if the type of the parameter is a sampler and false otherwise */
	static bool isSampler(const ResourceInternalType &type);

	/** Returns true if the value of the parameter corresponds to a shader image variable
	* @param type [in] value to test
	* @return true if the type of the parameter is an image and false otherwise */
	static bool isImage(const ResourceInternalType &type);

	/** Returns true if the value of the parameter corresponds to a shader atomic counter variable
	* @param type [in] value to test
	* @return true if the type of the parameter is an atomic counter and false otherwise */
	static bool isAtomicCounter(const ResourceInternalType &type);

	/** Compares the values ExposedStructField::m_structFieldResource and ExposedStructField::m_data
	* properly converted to its original type according to ExposedStructField::m_internalType and,
	* if different, the value in exposedStructField is updated
	* @param exposedStructField [in] data to see if there're changes
	* @return true of the resource changed, false otherwise */
	static bool setResourceCPUValue(ExposedStructField& exposedStructField);

	/** Appends the exposed data present in exposedStructField to the uniform buffer given by uniformBuffer
	* at cell given by cellIndex
	* @param cpuBuffer          [in] cpu buffer to append data from exposedStructField at cell cellIndex
	* @param cellIndex          [in] index in the uniform buffer given by uniformBuffer to append data
	* @param exposedStructField [in] exposed field with the data to append at cell index cellIndex at uniformBuffer
	* @return nothing */
	static void appendExposedStructFieldDataToUniformBufferCell(CPUBuffer* cpuBuffer, int cellIndex, const ExposedStructField* exposedStructField);

	/** Utility function to know whether a specific texture sample is integer format sampler
	* @param resourceInternalType  [in] enum to test
	* @return true if the enum tested corresponds to an integer format sampler, flase otherwise */
	static bool isIntegerTextureFormat(ResourceInternalType resourceInternalType);

	/** Utility function to know whether a specific image sample is integer format sampler
	* @param resourceInternalType  [in] enum to test
	* @return true if the enum tested corresponds to an integer format sampler, flase otherwise */
	static bool isIntegerImageFormat(ResourceInternalType resourceInternalType);

protected:
	/** Compares the values ExposedStructField::m_structFieldResource and ExposedStructField::m_data
	* properly converted to its original type according to ExposedStructField::m_internalType and,
	* if different, the value in exposedStructField is updated
	* @param exposedStructField [in] data to see if there're changes
	* @return true of the resource changed, false otherwise */
	static bool testUpdateResourceCPUValue(ExposedStructField& exposedStructField);

	/** Tests if the value in uniformBase is different from the one given as parameter, and updates it
	* @param uniformBase [in] uniform base instance to test the value given as parameter
	* @param value		 [in] value to test
	* @return true if the uniform's value was modified and false otherwise */
	static bool setUniformCPUValue(UniformBase *uniformBase, const float &value);

	/** Tests if the value in uniformBase is different from the one given as parameter, and updates it
	* @param uniformBase [in] uniform base instance to test the value given as parameter
	* @param value		 [in] value to test
	* @return true if the uniform's value was modified and false otherwise */
	static bool setUniformCPUValue(UniformBase *uniformBase, const vec2 &value);

	/** Tests if the value in uniformBase is different from the one given as parameter, and updates it
	* @param uniformBase [in] uniform base instance to test the value given as parameter
	* @param value		 [in] value to test
	* @return true if the uniform's value was modified and false otherwise */
	static bool setUniformCPUValue(UniformBase *uniformBase, const vec3 &value);

	/** Tests if the value in uniformBase is different from the one given as parameter, and updates it
	* @param uniformBase [in] uniform base instance to test the value given as parameter
	* @param value		 [in] value to test
	* @return true if the uniform's value was modified and false otherwise */
	static bool setUniformCPUValue(UniformBase *uniformBase, const vec4 &value);

	/** Tests if the value in uniformBase is different from the one given as parameter, and updates it
	* @param uniformBase [in] uniform base instance to test the value given as parameter
	* @param value		 [in] value to test
	* @return true if the uniform's value was modified and false otherwise */
	static bool setUniformCPUValue(UniformBase *uniformBase, const double &value);

	/** Tests if the value in uniformBase is different from the one given as parameter, and updates it
	* @param uniformBase [in] uniform base instance to test the value given as parameter
	* @param value		 [in] value to test
	* @return true if the uniform's value was modified and false otherwise */
	static bool setUniformCPUValue(UniformBase *uniformBase, const dvec2 &value);

	/** Tests if the value in uniformBase is different from the one given as parameter, and updates it
	* @param uniformBase [in] uniform base instance to test the value given as parameter
	* @param value		 [in] value to test
	* @return true if the uniform's value was modified and false otherwise */
	static bool setUniformCPUValue(UniformBase *uniformBase, const dvec3 &value);

	/** Tests if the value in uniformBase is different from the one given as parameter, and updates it
	* @param uniformBase [in] uniform base instance to test the value given as parameter
	* @param value		 [in] value to test
	* @return true if the uniform's value was modified and false otherwise */
	static bool setUniformCPUValue(UniformBase *uniformBase, const dvec4 &value);

	/** Tests if the value in uniformBase is different from the one given as parameter, and updates it
	* @param uniformBase [in] uniform base instance to test the value given as parameter
	* @param value		 [in] value to test
	* @return true if the uniform's value was modified and false otherwise */
	static bool setUniformCPUValue(UniformBase *uniformBase, const int &value);

	/** Tests if the value in uniformBase is different from the one given as parameter, and updates it
	* @param uniformBase [in] uniform base instance to test the value given as parameter
	* @param value		 [in] value to test
	* @return true if the uniform's value was modified and false otherwise */
	static bool setUniformCPUValue(UniformBase *uniformBase, const ivec2 &value);

	/** Tests if the value in uniformBase is different from the one given as parameter, and updates it
	* @param uniformBase [in] uniform base instance to test the value given as parameter
	* @param value		 [in] value to test
	* @return true if the uniform's value was modified and false otherwise */
	static bool setUniformCPUValue(UniformBase *uniformBase, const ivec3 &value);

	/** Tests if the value in uniformBase is different from the one given as parameter, and updates it
	* @param uniformBase [in] uniform base instance to test the value given as parameter
	* @param value		 [in] value to test
	* @return true if the uniform's value was modified and false otherwise */
	static bool setUniformCPUValue(UniformBase *uniformBase, const ivec4 &value);

	/** Tests if the value in uniformBase is different from the one given as parameter, and updates it
	* @param uniformBase [in] uniform base instance to test the value given as parameter
	* @param value		 [in] value to test
	* @return true if the uniform's value was modified and false otherwise */
	static bool setUniformCPUValue(UniformBase *uniformBase, const uint &value);

	/** Tests if the value in uniformBase is different from the one given as parameter, and updates it
	* @param uniformBase [in] uniform base instance to test the value given as parameter
	* @param value		 [in] value to test
	* @return true if the uniform's value was modified and false otherwise */
	static bool setUniformCPUValue(UniformBase *uniformBase, const uvec2 &value);

	/** Tests if the value in uniformBase is different from the one given as parameter, and updates it
	* @param uniformBase [in] uniform base instance to test the value given as parameter
	* @param value		 [in] value to test
	* @return true if the uniform's value was modified and false otherwise */
	static bool setUniformCPUValue(UniformBase *uniformBase, const uvec3 &value);

	/** Tests if the value in uniformBase is different from the one given as parameter, and updates it
	* @param uniformBase [in] uniform base instance to test the value given as parameter
	* @param value		 [in] value to test
	* @return true if the uniform's value was modified and false otherwise */
	static bool setUniformCPUValue(UniformBase *uniformBase, const uvec4 &value);

	/** Tests if the value in uniformBase is different from the one given as parameter, and updates it
	* @param uniformBase [in] uniform base instance to test the value given as parameter
	* @param value		 [in] value to test
	* @return true if the uniform's value was modified and false otherwise */
	static bool setUniformCPUValue(UniformBase *uniformBase, const bool &value);

	/** Tests if the value in uniformBase is different from the one given as parameter, and updates it
	* @param uniformBase [in] uniform base instance to test the value given as parameter
	* @param value		 [in] value to test
	* @return true if the uniform's value was modified and false otherwise */
	static bool setUniformCPUValue(UniformBase *uniformBase, const bvec2 &value);

	/** Tests if the value in uniformBase is different from the one given as parameter, and updates it
	* @param uniformBase [in] uniform base instance to test the value given as parameter
	* @param value		 [in] value to test
	* @return true if the uniform's value was modified and false otherwise */
	static bool setUniformCPUValue(UniformBase *uniformBase, const bvec3 &value);

	/** Tests if the value in uniformBase is different from the one given as parameter, and updates it
	* @param uniformBase [in] uniform base instance to test the value given as parameter
	* @param value		 [in] value to test
	* @return true if the uniform's value was modified and false otherwise */
	static bool setUniformCPUValue(UniformBase *uniformBase, const bvec4 &value);

	/** Tests if the value in uniformBase is different from the one given as parameter, and updates it
	* @param uniformBase [in] uniform base instance to test the value given as parameter
	* @param value		 [in] value to test
	* @return true if the uniform's value was modified and false otherwise */
	static bool setUniformCPUValue(UniformBase *uniformBase, const mat2 &value);

	/** Tests if the value in uniformBase is different from the one given as parameter, and updates it
	* @param uniformBase [in] uniform base instance to test the value given as parameter
	* @param value		 [in] value to test
	* @return true if the uniform's value was modified and false otherwise */
	static bool setUniformCPUValue(UniformBase *uniformBase, const mat3 &value);

	/** Tests if the value in uniformBase is different from the one given as parameter, and updates it
	* @param uniformBase [in] uniform base instance to test the value given as parameter
	* @param value		 [in] value to test
	* @return true if the uniform's value was modified and false otherwise */
	static bool setUniformCPUValue(UniformBase *uniformBase, const mat4 &value);

	/** Tests if the value in uniformBase is different from the one given as parameter, and updates it
	* @param uniformBase [in] uniform base instance to test the value given as parameter
	* @param value		 [in] value to test
	* @return true if the uniform's value was modified and false otherwise */
	static bool setUniformCPUValue(UniformBase *uniformBase, const mat2x3 &value);

	/** Tests if the value in uniformBase is different from the one given as parameter, and updates it
	* @param uniformBase [in] uniform base instance to test the value given as parameter
	* @param value		 [in] value to test
	* @return true if the uniform's value was modified and false otherwise */
	static bool setUniformCPUValue(UniformBase *uniformBase, const mat2x4 &value);

	/** Tests if the value in uniformBase is different from the one given as parameter, and updates it
	* @param uniformBase [in] uniform base instance to test the value given as parameter
	* @param value		 [in] value to test
	* @return true if the uniform's value was modified and false otherwise */
	static bool setUniformCPUValue(UniformBase *uniformBase, const mat3x2 &value);

	/** Tests if the value in uniformBase is different from the one given as parameter, and updates it
	* @param uniformBase [in] uniform base instance to test the value given as parameter
	* @param value		 [in] value to test
	* @return true if the uniform's value was modified and false otherwise */
	static bool setUniformCPUValue(UniformBase *uniformBase, const mat3x4 &value);

	/** Tests if the value in uniformBase is different from the one given as parameter, and updates it
	* @param uniformBase [in] uniform base instance to test the value given as parameter
	* @param value		 [in] value to test
	* @return true if the uniform's value was modified and false otherwise */
	static bool setUniformCPUValue(UniformBase *uniformBase, const mat4x2 &value);

	/** Tests if the value in uniformBase is different from the one given as parameter, and updates it
	* @param uniformBase [in] uniform base instance to test the value given as parameter
	* @param value		 [in] value to test
	* @return true if the uniform's value was modified and false otherwise */
	static bool setUniformCPUValue(UniformBase *uniformBase, const mat4x3 &value);

	/** Tests if the value in uniformBase is different from the one given as parameter, and updates it
	* @param uniformBase [in] uniform base instance to test the value given as parameter
	* @param value		 [in] value to test
	* @return true if the uniform's value was modified and false otherwise */
	static bool setUniformCPUValue(UniformBase *uniformBase, const dmat2 &value);

	/** Tests if the value in uniformBase is different from the one given as parameter, and updates it
	* @param uniformBase [in] uniform base instance to test the value given as parameter
	* @param value		 [in] value to test
	* @return true if the uniform's value was modified and false otherwise */
	static bool setUniformCPUValue(UniformBase *uniformBase, const dmat3 &value);

	/** Tests if the value in uniformBase is different from the one given as parameter, and updates it
	* @param uniformBase [in] uniform base instance to test the value given as parameter
	* @param value		 [in] value to test
	* @return true if the uniform's value was modified and false otherwise */
	static bool setUniformCPUValue(UniformBase *uniformBase, const dmat4 &value);

	/** Tests if the value in uniformBase is different from the one given as parameter, and updates it
	* @param uniformBase [in] uniform base instance to test the value given as parameter
	* @param value		 [in] value to test
	* @return true if the uniform's value was modified and false otherwise */
	static bool setUniformCPUValue(UniformBase *uniformBase, const dmat2x3 &value);

	/** Tests if the value in uniformBase is different from the one given as parameter, and updates it
	* @param uniformBase [in] uniform base instance to test the value given as parameter
	* @param value		 [in] value to test
	* @return true if the uniform's value was modified and false otherwise */
	static bool setUniformCPUValue(UniformBase *uniformBase, const dmat2x4 &value);

	/** Tests if the value in uniformBase is different from the one given as parameter, and updates it
	* @param uniformBase [in] uniform base instance to test the value given as parameter
	* @param value		 [in] value to test
	* @return true if the uniform's value was modified and false otherwise */
	static bool setUniformCPUValue(UniformBase *uniformBase, const dmat3x2 &value);

	/** Tests if the value in uniformBase is different from the one given as parameter, and updates it
	* @param uniformBase [in] uniform base instance to test the value given as parameter
	* @param value		 [in] value to test
	* @return true if the uniform's value was modified and false otherwise */
	static bool setUniformCPUValue(UniformBase *uniformBase, const dmat3x4 &value);

	/** Tests if the value in uniformBase is different from the one given as parameter, and updates it
	* @param uniformBase [in] uniform base instance to test the value given as parameter
	* @param value		 [in] value to test
	* @return true if the uniform's value was modified and false otherwise */
	static bool setUniformCPUValue(UniformBase *uniformBase, const dmat4x2 &value);

	/** Tests if the value in uniformBase is different from the one given as parameter, and updates it
	* @param uniformBase [in] uniform base instance to test the value given as parameter
	* @param value		 [in] value to test
	* @return true if the uniform's value was modified and false otherwise */
	static bool setUniformCPUValue(UniformBase *uniformBase, const dmat4x3 &value);
	
	/** Inits the values of m_mapResourceInternalType, building as key the hashed value of each element in arrayResourceKeywords
	* and as mapped the corresponding element in ResourceInternalType
	* @return the correspnding generated map */
	static map<uint, ResourceInternalType> initResourceInternalTypeMap();

	/** Inits the values of m_mapImageInternalType, building as key the hashed value of each element in arrayImageFormatQualifierKeywords
	* and as mapped the corresponding element in ImageInternalFormat
	* @return the correspnding generated map */
	static map<uint, ImageInternalFormat> initImageInternalType();

	/** Returns a pointer of the uniform with type given by the type parameter and name given by the name parameter,
	* therefore fields in structs in a shader can be modelled
	* @param type         [in] enumerated with value the type of the uniform
	* @param shaderStage  [in] enumerated with value the shader stage the uniform being built is used
	* @param name         [in] name of the uniform
	* @param structName   [in] name of the struct whose field contains this variable
	* @param structType   [in] name of the type of the struct whose field contains this variable
	* @param shaderStruct [in] pointer to the shader struct owner of the variable to make (the struct in the shader this variable is defined inside)
	* @return a pointer to the uniform built and nullptr if there wasn't possible to build the new uniform */
	static UniformBase* makeUniform(ResourceInternalType type, VkShaderStageFlagBits shaderStage, string&& name, string&& structName, string&& structType, ShaderStruct* shaderStruct);

	/** Returns a Vulkan enum describing the shader stage for the compiler given as parameter
	* therefore fields in structs in a shader can be modelled
	* @param compiler [in] spirv-cross compiler to obtain the shader stage from
	* @return a Vulkan enum describing the shader stage for the compiler given as parameter */
	static VkShaderStageFlagBits obtainShaderStage(spirv_cross::CompilerGLSL& compiler);

	static map<uint, ResourceInternalType> m_mapResourceInternalType; //!< Map having as key the hashed value of each element in arrayResourceKeywords and as mapped the corresponding element in ResourceInternalType
	static map<uint, ImageInternalFormat>  m_mapImageInternalType;    //!< Map having as key the hashed value of each element in arrayImageFormatQualifierKeywords and as mapped the corresponding element in ImageInternalFormat
};

/////////////////////////////////////////////////////////////////////////////////////////////

#endif _SHADER_REFLECTION_H_
