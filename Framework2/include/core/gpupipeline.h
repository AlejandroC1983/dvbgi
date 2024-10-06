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

#ifndef _GPUPIPELINE_H_
#define _GPUPIPELINE_H_


// GLOBAL INCLUDES

// PROJECT INCLUDES
#include "../headers.h"
#include "../../include/util/getsetmacros.h"
#include "../../include/commonnamespace.h"
#include "../../include/core/coreenum.h"
#include "../../include/util/singleton.h"

// CLASS FORWARDING
class UniformBuffer;
class RasterTechnique;

// NAMESPACE
using namespace commonnamespace;
using namespace coreenum;

// DEFINES
#define gpuPipelineM s_pGPUPipeline->instance()
#define NUM_SAMPLES VK_SAMPLE_COUNT_1_BIT // Number of samples needs to be the same at image creation, used at renderpass creation (in attachment) and pipeline creation

/////////////////////////////////////////////////////////////////////////////////////////////

class GPUPipeline : public Singleton<GPUPipeline>
{
public:
	/** Default constructor
	* @return nothing */
	GPUPipeline();

	/** Default destructor
	* @return nothing */
	~GPUPipeline();

	/** Destroys all elements in the manager
	* @return nothing */
	void destroyResources();

	/** Initializes all the rasterization techniques, scene index and vertex buffers, and the material
	* and uniform buffer objetcs
	* @return nothing */
	void init();

	/** Calls destructor
	* @return nothing */
	void shutdown();

	/** Returns a pointer to the element in m_vectorRasterTechnique with name equal to the one given as parameter, if not
	* found, it returns nullptr
	* @param name [in] name of the technique to look for in m_vectorRasterTechnique
	* @return pointer to the element in m_vectorRasterTechnique with name equal to the one given as parameter, nullptr otherwise */
	RasterTechnique* getRasterTechniqueByName(string &&name);

	/** Build the scene geoemtry index and vertex buffer
	* @param arrayIndices     [in] array with index data
	* @param indexBufferName  [in] name of the index buffer
	* @param arrayVertexData  [in] array with vertex data (expanded per-vertex attributes)
	* @param vertexBufferName [in] name of the vertex buffer
	* @return nothing */
	void createVertexBuffer(vectorUint& arrayIndices, const string& indexBufferName, vectorUint8& arrayVertexData, const string& vertexBufferName);

	/** Build the scene vertex specification with viIpBind and m_viIpAttrb
	* @param dataStride [in] per vertex total stride
	* @return nothing */
	void setVertexInput(uint32_t dataStride);

	/** Update scene element transform buffer and camera buffer and upload data to GPU
	* @return nothing */
	void update();

	/** Sets the viewport extent and origin coordinates in screen space, as well as the minimum and maximum depth
	* @param width    [in] viewport width
	* @param height   [in] viewport height
	* @param offsetX  [in] viewport offset
	* @param offsetY  [in] viewport offset
	* @param minDepth [in] viewport minimum depth
	* @param maxDepth [in] viewport maximum depth
	* @param cmd      [in] command to record to
	* @return nothing */
	void initViewports(float width, float height, float offsetX, float offsetY, float minDepth, float maxDepth, VkCommandBuffer* cmd);

	/** Sets the scissor extent and origin coordinates in screen space
	* @param width    [in] scissor width
	* @param height   [in] scissor height
	* @param offsetX  [in] scissor offset
	* @param offsetY  [in] scissor offset
	* @param minDepth [in] scissor minimum depth
	* @param maxDepth [in] scissor maximum depth
	* @param cmd      [in] command to record to
	* @return nothing */
	void initScissors(uint width, uint height, int offsetX, int offsetY, VkCommandBuffer* cmd);
	
	/** Build all scene geometry buffer to upload to GPU
	* @param vectorNode      [in]    vector with the nodes to generate a single vector with all indices and another vctor with all vertices in the inout parameters
	* @param arrayIndices    [inout] array with indices
	* @param arrayVertexData [inout] array with vertex data (expanded per-vertex attribute)
	* @return nothing */
	void buildSceneBufferData(const vectorNodePtr& vectorNode, vectorUint& arrayIndices, vectorUint8& arrayVertexData);

	/** Skeletal meshes have the same vertex format as regular static meshes. The vertex buffer is updated through compute and used for rasterizaton and ray tracing.
	* This method builds the date for an extra buffer where to store the per-vertex bone transform weights and matrix indices, with a current implementation 
	* of up to four weights per vertex
	* @param vectorNode [in]    vector with the nodes which have a skinned mesh render component to generate the buffer data from 
	* @param vectorData [inout] vector with the bone index and bone weight information from the vectorNode provided
	* @return nothing */
	void buildSkeletalMeshBufferData(const vectorNodePtr& vectorNode, vectorFloat& vectorData);

	/** Build pipeline caches for building compute and graphics pipelines
	* @return nothing */
	void createPipelineCache();

	/** Build the uniform buffer with world space transform information for all the scene elements
	* @return nothing */
	void createSceneDataUniformBuffer();

	/** Build the uniform buffer with camera information
	* @return nothing */
	void createSceneCameraUniformBuffer();

	/** Builds a descriptor set of type given by descriptorType. The generated resources a VkDescriptorSetLayout,
	* a VkDescriptorPool, and a VkDescriptorSet, are returned as parameters.
	* @param vectorDescriptorType  [in] Vector with the types of the descriptors to build (must match in size vectorDescriptorType)
	* @param vectorBindingIndex    [in] Vector with the binding indices for each one of the descriptor set elements (must match in size vectorDescriptorType)
	* @param vectorStageFlags      [in] Vector with the state flags for the descriptor sets to build
	* @param vectorDescriptorCount [in] Vector with the value of VkWriteDescriptorSet::descriptorCount of each descriptor 
	* @param descriptorSetLayout   [in] Descriptor set layout generated for the descriptor set to build
	* @param descriptorPool        [in] Descriptor pool generated for the descriptor set to build
	* @return descriptor set generated */
	VkDescriptorSet buildDescriptorSet(vector<VkDescriptorType>   vectorDescriptorType,
									   vector<uint32_t>           vectorBindingIndex,
									   vector<VkShaderStageFlags> vectorStageFlags,
									   vectorInt&                 vectorDescriptorCount,
									   VkDescriptorSetLayout&     descriptorSetLayout,
									   VkDescriptorPool&          descriptorPool);

	/** Updates the descriptor sets given by vectorDescriptorSet, of type buffer / image / texel buffer
	* (given as void pointers in vectorDescriptorInfo).
	* @param descriptorSet            [in] Descriptor set to update
	* @param vectorDescriptorType     [in] Vector with the descriptor types to update
	* @param vectorDescriptorInfo     [in] Vector with the descriptor ifo (buffer / image / texel) codified as void pointers
	* @param vectorDescriptorInfoHint [in] Vector to know what reeosurce is represented in the same index in vectorDescriptorInfo
	* @param vectorDescriptorCount    [in] Vector with the value of VkWriteDescriptorSet::descriptorCount of each descriptor
	* @param vectorBinding            [in] Vector with the binding indices
	* @return nothing */
	void updateDescriptorSet(
		const VkDescriptorSet&          descriptorSet,
		const vector<VkDescriptorType>& vectorDescriptorType,
		const vector<void*>&            vectorDescriptorInfo,
		const vector<int>&              vectorDescriptorInfoHint,
		const vectorInt&                vectorDescriptorCount,
		const vector<uint32_t>&         vectorBinding);

	/** Destroys m_pipelineCache
	* @return nothing */
	void destroyPipelineCache();

	/** Returns the index of the technique given as parameter in
	* m_vectorRasterTechnique, or -1 if not present
	* @param technique [in] technique to look for the index in m_vectorRasterTechnique
	* @return index of the technique given as parameter in m_vectorRasterTechnique , or -1 if not present */
	int getRasterTechniqueIndex(RasterTechnique* technique);

	/** Add to m_mapRasterFlag the flag with name flagName and value given as parameter
	* @param flagName [in] name of the flag to add
	* @param value    [in] value fo the flag to add
	* @return true if the flag didn't exist previously, false if already exists (value is set in any case) */
	bool addRasterFlag(string&& flagName, int value);

	/** Remove from m_mapRasterFlag the flag with name flagName
	* @param flagName [in] name of the flag to remove
	* @return true if the flag was removed successfully, false otherwise (the flag wasn't found) */
	bool removeRasterFlag(string&& flagName);

	/** Set valueto the raster flag in m_mapRasterFlag
	* @param flagName [in] name of the flag whose value to set
	* @param value    [in] vlaue set to the flag
	* @return true if the value was set successfully, false otherwise (the flag didn't exist) */
	bool setRasterFlag(string&& flagName, int value);

	/** Return the value fo the raster flag with name flagName
	* @param flagName [in] name of the flag whose value to return
	* @return value of the raster flag given by flagName if exists, -1 otherwise */
	int getRasterFlagValue(string&& flagName);

	/** Getter of m_viIpAttrb
	* @return pointer to m_viIpAttrb */
	VkVertexInputAttributeDescription* refVertexInputAttributeDescription();

	/** Builds basic resources needed for scene loading like a default material
	* @return nothing */
	void preSceneLoadResources();

	REF(VkVertexInputBindingDescription, m_viIpBind, ViIpBind)
	GET(VkPipelineCache, m_pipelineCache, PipelineCache)
	GET_PTR(UniformBuffer, m_sceneUniformData, SceneUniformData)
	REF_PTR(UniformBuffer, m_sceneUniformData, SceneUniformData)
	GET_PTR(UniformBuffer, m_sceneCameraUniformData, SceneCameraUniformData)
	REF_PTR(UniformBuffer, m_sceneCameraUniformData, SceneCameraUniformData)
	GET(vectorRasterTechniquePtr, m_vectorRasterTechnique, VectorRasterTechnique)
	REF(vectorRasterTechniquePtr, m_vectorRasterTechnique, VectorRasterTechnique)
	GET(bool, m_pipelineInitialized, PipelineInitialized)
	GETCOPY(uint, m_vertexStrideBytes, VertexStrideBytes)

protected:
	VkVertexInputBindingDescription   m_viIpBind;               //!< Stores the vertex input rate
	VkVertexInputAttributeDescription m_viIpAttrb[2];           //!< Store metadata helpful in data interpretation
	VkPipelineCache                   m_pipelineCache;          //!< Pipeline cache
	UniformBuffer*                    m_sceneUniformData;       //!< Uniform buffer with the per scene elements data
	UniformBuffer*                    m_sceneCameraUniformData; //!< Uniform buffer with the scene camera data
	vectorRasterTechniquePtr          m_vectorRasterTechnique;  //!< Vector with the raster techniques currently in use
	vector<bool>                      m_vectorReRecordFlags;    //!< Vector to cache the results returned by post queue submit and record method, to know if any technique is asking for re-recording
	bool                              m_pipelineInitialized;    //!< True if the call to GPUPipeline::init() has been done
	map<string, int>                  m_mapRasterFlag;          //!< Map to set rasterizatin flags used by the different raster techniques
	uint                              m_vertexStrideBytes;      //!< Vertex data stride in bytes
};

static GPUPipeline* s_pGPUPipeline;

/////////////////////////////////////////////////////////////////////////////////////////////

#endif _GPUPIPELINE_H_
