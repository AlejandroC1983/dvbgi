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

#pragma once
// GLOBAL INCLUDES
#define NOMINMAX
#include <Windows.h>

#include "vulkan.h"

// PROJECT INCLUDES
#include "../include/commonnamespace.h"

// CLASS FORWARDING
class Shader;
class Node;
class RasterTechnique;
class Texture;
class Sampler;
class Image;
class AtomicCounterUnit;
class UniformBase;
class ShaderStruct;
class RasterTechnique;
class ShaderStorageBuffer;
class ShaderTopLevelAccelerationStructure;
class TopLevelAccelerationStructure;
class Material;
class GenericResource;
class Framebuffer;
class Buffer;
class Camera;
class BaseComponent;
class SkinnedMeshRenderComponent;

// NAMESPACE
using namespace commonnamespace;

// DEFINES
typedef unsigned int			                     uint;
typedef vector<char>		                         vectorChar;
typedef vector<const char*>		                     vectorCharPtr;
typedef vector<int>		                             vectorInt;
typedef vector<void*>		                         vectorVoidPtr;
typedef vector<uint>                                 vectorUint;
typedef vector<uint16_t>                             vectorUint16;
typedef vector<uint32_t>                             vectorUint32;
typedef vector<uint8_t>                              vectorUint8;
typedef vector<uint32_t*>                            vectorUint32Ptr;
typedef vector<size_t>                               vectorSize_t;
typedef vector<float>                                vectorFloat;
typedef vector<vector<float>>                        vectorVectorFloat;
typedef vector<bool>                                 vectorBool;
typedef vector<vec4>                                 vectorVec4;
typedef vector<vector<vec4>>                         vectorVectorVec4;
typedef vector<vec3>                                 vectorVec3;
typedef vector<vector<vec3>>                         vectorVectorVec3;
typedef vector<vec2>                                 vectorVec2;
typedef vector<mat4>                                 vectorMat4;
typedef vector<quat>                                 vectorQuat;
typedef vector<string>		                         vectorString;
typedef vector<Node*>                                vectorNodePtr;
typedef vector<Shader*>                              vectorShaderPtr;
typedef vector<RasterTechnique*>                     vectorRasterTechniquePtr;
typedef vector<vector<RasterTechnique*>>             vectorVectorRasterTechniquePtr;
typedef map<string, uint>                            mapStringUint;
typedef vector<Texture*>                             vectorTexturePtr;
typedef vector<const Texture*>                       vectorTextureConstPtr;
typedef map<string, Texture*>                        mapStringTexturePtr;
typedef map<string, int>                             mapStringInt;
typedef map<Node*, int>                              mapNodePrtInt;
typedef map<Node*, uint>                             mapNodePrtUint;
typedef vector<UniformBase*>                         vectorUniformBasePtr;
typedef vector<Sampler*>		                     vectorSamplerPtr;
typedef vector<Image*>			                     vectorImagePtr;
typedef vector<AtomicCounterUnit*>                   vectorAtomicCounterUnitPtr;
typedef vector<ShaderStruct*>	                     vectorShaderStructPtr;
typedef vector<ShaderStruct>	                     vectorShaderStruct;
typedef vector<ShaderStorageBuffer*>                 vectorShaderStorageBufferPtr;
typedef vector<ShaderTopLevelAccelerationStructure*> vectorShaderTopLevelAccelerationStructurePtr;
typedef vector<TopLevelAccelerationStructure*>       vectorTopLevelAccelerationStructurePtr;
typedef vector<Material*>                            vectorMaterialPtr;
typedef map<string, Node*>                           mapStringNode;
typedef vector<GenericResource*>                     vectorGenericResourcePtr;
typedef map<uint, VkCommandBuffer>                   mapUintCommandBuffer;
typedef map<uint, uvec4>                             mapUintUvec4;
typedef vector<VkCommandBuffer*>                     vectorCommandBufferPtr;
typedef vector<Framebuffer*>                         vectorFramebufferPtr;
typedef vector<Buffer*>                              vectorBufferPtr;
typedef vector<Camera*>                              vectorCameraPtr;
typedef vector<VkDescriptorBufferInfo>               vectorDescriptorBufferInfo;
typedef vector<VkDescriptorImageInfo>                vectorDescriptorImageInfo;
typedef vector<BaseComponent*>                       vectorBaseComponentPtr;
typedef vector<VkImageLayout>                        vectorImageLayout;
typedef vector<uvec4>                                vectorUVec4;
typedef vector<uvec2>                                vectorUVec2;
typedef vector<SkinnedMeshRenderComponent*>          vectorSkinnedMeshRenderComponentPtr;
typedef vector<VkClearValue>                         vectorVkClearValue;

#ifdef _WIN32
#pragma comment(linker, "/subsystem:console")
#define WIN32_LEAN_AND_MEAN
//#define NOMINMAX
#define APP_NAME_STR_LEN 80
#else  // _WIN32
#define VK_USE_PLATFORM_XCB_KHR
#include <unistd.h>
#endif // _WIN32
#define GLM_FORCE_RADIANS

#define DEG_TO_RAD     pi<float>()/180.0f
#define RAD_TO_DEG   180.0f/pi<float>()
#define SMALL_FLOAT    0.001

#define INSTANCE_FUNC_PTR(instance, entrypoint){											\
    m_fp##entrypoint = (PFN_vk##entrypoint) vkGetInstanceProcAddr(instance, "vk"#entrypoint); \
    if (m_fp##entrypoint == NULL) {															\
        cout << "Unable to locate the vkGetDeviceProcAddr: vk"#entrypoint;				\
        exit(-1);																			\
    }																						\
}

#define DEVICE_FUNC_PTR(dev, entrypoint){													\
    m_fp##entrypoint = (PFN_vk##entrypoint) vkGetDeviceProcAddr(dev, "vk"#entrypoint);		\
    if (m_fp##entrypoint == NULL) {															\
        cout << "Unable to locate the vkGetDeviceProcAddr: vk"#entrypoint;				\
        exit(-1);																			\
    }																						\
}

#define ZNEAR    0.1f // TODO: adapt per-scene
#define ZFAR  10000.0f // TODO: adapt per-scene

/////////////////////////////////////////////////////////////////////////////////////////////

// TODO: MOVE FROM HERE
struct LayerProperties
{
	VkLayerProperties properties;
	vector<VkExtensionProperties> extensions;
};

/////////////////////////////////////////////////////////////////////////////////////////////

/** Used for the diferent types of nodes in the scene (static and dynamic for now) */
enum eNodeType
{
	E_NT_STATIC_ELEMENT      = 0x00000001, //!< The node is a static scene element
	E_NT_DYNAMIC_ELEMENT     = 0x00000002, //!< The node is a dynamic scene element
	E_NT_SKINNEDMESH_ELEMENT = 0x00000004, //!< The node is a skinned mesh scene element
	E_NT_SIZE
};

/////////////////////////////////////////////////////////////////////////////////////////////

/** Used for the diferent types of geometry to represent in OpenGL (triangles, triangle strip, triangle fan for now) */
enum eGeometryType
{
	E_GT_TRIANGLES = 0, // A list of indexed triangles whci form a mesh
	E_GT_TRIANGLESTRIP, // A triangle strip mesh
	E_GT_TRIANGLEFAN,   // A triangle fan mesh
	E_GT_SIZE
};

/////////////////////////////////////////////////////////////////////////////////////////////

/** Used to classify a mesh, and know if it is a ligth volume or a 3D model, and if it is a light vol, which type is it */
enum class eMeshType
{
	E_MT_RENDER_MODEL           = 0x00000001, //!< The mesh is not a light volume
	E_MT_INSTANCED_RENDER_MODEL = 0x00000002, //!< The mesh is for instance rendering
	E_MT_DEBUG_RENDER_MODEL     = 0x00000004, //!< The mesh is not a light volume and has debug purposes
	E_MT_EMITTER_MODEL          = 0x00000008, //!< The mesh is an emitter
	E_MT_LIGHTVOL_SPHERE        = 0x00000010, //!< The mesh is a light volume of type sphere
	E_MT_LIGHTVOL_CONE          = 0x00000020, //!< The mesh is a light volume of type cone
	E_MT_LIGHTVOL_TORUS         = 0x00000040, //!< The mesh is a light volume of type torus
	E_MT_DIRLIGHT_SQUARE        = 0x00000080, //!< The mesh is a square used for the directional light pass
	E_MT_RENDER_SHADOW          = 0x00000100, //!< The mesh is for shadow map rasterization
	E_MT_SIZE                   = 0x00000200, //!< Number of elements of type eMeshType
	E_MT_ALL                    = 0x7FFFFFFF  //!< All flags set (to get all models)
};

/////////////////////////////////////////////////////////////////////////////////////////////

inline eMeshType operator|(eMeshType a, eMeshType b)
{
	return static_cast<eMeshType>(static_cast<uint>(a) | static_cast<uint>(b));
}

/////////////////////////////////////////////////////////////////////////////////////////////

inline bool operator&(eMeshType a, eMeshType b)
{
	return static_cast<uint>(a) & static_cast<uint>(b);
}

/////////////////////////////////////////////////////////////////////////////////////////////

inline eNodeType operator|(eNodeType a, eNodeType b)
{
	return static_cast<eNodeType>(static_cast<uint>(a) | static_cast<uint>(b));
}

/////////////////////////////////////////////////////////////////////////////////////////////
