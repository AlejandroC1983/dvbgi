/*
Copyright 2020 Alejandro Cosin

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

#ifndef _COREMANAGER_H_
#define _COREMANAGER_H_

// GLOBAL INCLUDES

// PROJECT INCLUDES
#include "../../include/util/singleton.h"
#include "../../include/core/gpupipeline.h"
#include "../../include/core/surface.h"
#include "../../include/texture/texture.h"
#include "../../include/util/getsetmacros.h"
#include "../../include/util/loopmacrodefines.h"
#include "../../include/core/logicaldevice.h"
#include "../../include/core/physicaldevice.h"
#include "../../include/core/instance.h"
#include "../../include/core/swapchain.h"
#include "../../include/core/input.h"
#include "../../include/core/functionpointer.h"

// CLASS FORWARDING

// NAMESPACE

// DEFINES
#define coreM s_pCoreManager->instance()

/////////////////////////////////////////////////////////////////////////////////////////////

class CoreManager: public Singleton<CoreManager>
{
public:
	/** Constructor
	* @return nothing */
	CoreManager();

	/** Default destructor
	* @return nothing */
	~CoreManager();

	/** Initialize and allocate resources
	* @return nothing */
	void initialize();

	/** Prepare resources
	* @return nothing */
	void prepare();

	/** Resize window
	* @return nothing */
	void resize();

	/** Release resources
	* @return nothing */
	void deInitialize();

	/** Create swapchain color image and depth image
	* @return nothing */
	void buildSwapChainAndDepthImage();

	/** Builds the render pass, m_renderPass
	* @return nothing */
	void createRenderPass(bool includeDepth, bool clear = true);	// Render Pass creation

	/** Builds the command pools, m_graphicsCommandPool and m_computeCommandPool
	* @return nothing */
	void createCommandPools();

	/** Sets the extent of the swap chain
	* @return nothing */
	void setSwapChainExtent(uint32_t width, uint32_t height);

	/** Render primitives
	* @return nothing */
	void render();

	/** Called after render method
	* @return nothing */
	void postRender();

	/** Destroys m_renderPass
	* @return nothing */
	void destroyRenderpass();

	/** Getter of Surface::m_surface
	* @return Surface::m_surface of type VkFormat */
	VkFormat getSurfaceFormat();

	/** Getter of PhysicalDevice::m_queueFamilyProps
	* @return PhysicalDevice::m_queueFamilyProps of type vector<VkQueueFamilyProperties> */
	const vector<VkQueueFamilyProperties>& getQueueFamilyProps();

	/** Getter of Swapchain::m_arraySwapchainImages
	* @return Swapchain::m_arraySwapchainImages of type vector<VkImage> */
	const vector<VkImage>& getArraySwapchainImages();

	/** Getter of Swapchain::m_arrayFramebuffers
	* @return Swapchain::m_arrayFramebuffers of type vector<Framebuffer*> */
	const vector<Framebuffer*>& getArrayFramebuffers();

	/** Sets the extent of the swap chain images to be build
	* @return nothing */
	VkExtent2D setSwapChainExtent();

	/** Returns the result of calling Surface::managePresentMode
	* @return the result of calling Surface::managePresentMode */
	void managePresentMode(VkPresentModeKHR& presentMode, uint32_t& numberOfSwapChainImages);

	/** Returns the result of calling PhysicalDevice::memoryTypeFromProperties
	* @return the result of calling PhysicalDevice::memoryTypeFromProperties */
	bool memoryTypeFromProperties(uint32_t typeBits, VkFlags requirementsMask, const VkMemoryType* memoryTypes, uint32_t& typeIndex);

	/** Getter of PhysicalDevice::m_physicalDeviceMemoryProperties
	* @return PhysicalDevice::m_physicalDeviceMemoryProperties of type VkPhysicalDeviceMemoryProperties */
	const VkPhysicalDeviceMemoryProperties& getPhysicalDeviceMemoryProperties();

	/** Getter of Surface::m_width
	* @return Surface::m_width of type uint32_t */
	uint32_t getWidth() const;

	/** Getter of Surface::m_height
	* @return Surface::m_height of type uint32_t */
	uint32_t getHeight() const;

	/** Getter of Surface::m_preTransform
	* @return Surface::m_preTransform of type VkSurfaceTransformFlagBitsKHR */
	VkSurfaceTransformFlagBitsKHR getPreTransform() const;

	/** Getter of Instance::m_instance
	* @return copy of Instance::m_instance of type VkInstance */
	const VkInstance getInstance() const;

	/** Getter of Surface::m_surface
	* @return copy of Surface::m_surface of type VkSurfaceKHR */
	const VkSurfaceKHR getSurface() const;

	/** Getter of PhysicalDevice::m_physicalDevice
	* @return copy of PhysicalDevice::m_physicalDevice of type VkPhysicalDevice */
	const VkPhysicalDevice getPhysicalDevice() const;

	/** Getter of LogicalDevice::m_logicalDevice
	* @return copy of LogicalDevice::m_logicalDevice of type VkDevice */
	const VkDevice& getLogicalDevice() const;

	/** Getter of LogicalDevice::m_logicalDeviceGraphicsQueue
	* @return copy of LogicalDevice::m_logicalDeviceGraphicsQueue of type VkQueue */
	const VkQueue getLogicalDeviceGraphicsQueue() const;

	/** Getter of LogicalDevice::m_logicalDeviceComputeQueue
	* @return copy of LogicalDevice::m_logicalDeviceComputeQueue of type VkQueue */
	const VkQueue getLogicalDeviceComputeQueue() const;

	/** Getter of PhysicalDevice::m_physicalDeviceProperties
	* @return reference to PhysicalDevice::m_physicalDeviceProperties of type VkPhysicalDeviceProperties */
	const VkPhysicalDeviceProperties& getPhysicalDeviceProperties() const;

	/** Getter of PhysicalDevice::m_physicalDeviceRayTracingPipelineProperties
	* @return reference to PhysicalDevice::m_physicalDeviceRayTracingPipelineProperties
	* VkPhysicalDeviceRayTracingPipelinePropertiesKHR of type VkPhysicalDeviceProperties */
	const VkPhysicalDeviceRayTracingPipelinePropertiesKHR& getkPhysicalDeviceRayTracingPipelineProperties() const;

	/** Getter of PhysicalDevice::m_physicalDeviceSubgroupProperties
	* @return reference to PhysicalDevice::m_physicalDeviceSubgroupProperties */
	const VkPhysicalDeviceSubgroupProperties& getVkPhysicalDeviceSubgroupProperties() const;

	/** Getter of PhysicalDevice::m_physicalDeviceConservativeRasterizationProperties
	* @return reference to PhysicalDevice::m_physicalDeviceConservativeRasterizationProperties */
	const VkPhysicalDeviceConservativeRasterizationPropertiesEXT& getVkPhysicalDeviceConservativeRasterizationProperties() const;

	/** Destroys m_graphicsCommandPool and m_computeCommandPool
	* @return nothing */
	void destroyCommandPools();

	/** Getter of Surface::m_window
	* @return windows platform handle */
	const HWND getWindowPlatformHandle() const;

	/** Returns the next available index for unique identifying command buffers
	* @return next available index for unique identifying command buffers */
	uint getNextCommandBufferIndex();

	/** Initialize some hardware limits information that might be of common use
	* @return nothing */
	void initHardwareLimitValues();

	/** Utility method to show some hardware limits information
	* @return nothing */
	void showHardwareLimits();

	/** Returns the values of SwapChain::m_width and SwapChain::m_height as an ivec2
	* @return values of SwapChain::m_width and SwapChain::m_height */
	ivec2 getSwapChainDimensions();

	/** Returns the value of PhysicalDevice::m_graphicsQueueIndex
	* @return value of PhysicalDevice::m_graphicsQueueIndex */
	uint32_t getGraphicsQueueIndex();

	/** Returns the value of PhysicalDevice::m_computeQueueIndex
	* @return value of PhysicalDevice::m_computeQueueIndex */
	uint32_t getComputeQueueIndex();

	/** Initialize the function pointers of the Vulkan functions present in the vkfp namespace
	* @return nothing */
	void initializeFunctionPointerFunctions();

	/** Set the Vulkan name of the Vulkan object given
	* @param object     [in] Vulkan object to name
	* @param objectType [in] Vulkan object type
	* @param name       [in] object name
	* @return nothing */
	void setObjectName(uint64_t object, VkDebugReportObjectTypeEXT objectType, const string& name);

	static void allocCommandBuffer(const VkDevice* device, const VkCommandPool cmdPool, VkCommandBuffer* cmdBuf, const VkCommandBufferAllocateInfo* commandBufferInfo = NULL);
	static void beginCommandBuffer(VkCommandBuffer cmdBuf, VkCommandBufferBeginInfo* inCmdBufInfo = NULL);
	static void endCommandBuffer(VkCommandBuffer cmdBuf);
	static void submitCommandBuffer(const VkQueue& queue, const VkCommandBuffer* cmdBufList, const VkSubmitInfo* submitInfo = NULL, const VkFence& fence = VK_NULL_HANDLE);

	GETCOPY(bool, m_isPrepared, IsPrepared)
	GETCOPY(bool, m_isResizing, IsResizing)
	REF(VkRenderPass, m_renderPass, RenderPass)
	GETCOPY(VkCommandPool, m_graphicsCommandPool, GraphicsCommandPool)
	GETCOPY(VkCommandPool, m_computeCommandPool, ComputeCommandPool)
	GETCOPY_SET(bool, m_reachedFirstRaster, ReachedFirstRaster)
	GET(VkQueryPool, m_graphicsQueueQueryPool, GraphicsQueueQueryPool)
	GET(VkQueryPool, m_computeQueueQueryPool, ComputeQueueQueryPool)
	GETCOPY(uint, m_maxComputeWorkGroupInvocations, MaxComputeWorkGroupInvocations)
	GETCOPY(uint, m_maxComputeSharedMemorySize, MaxComputeSharedMemorySize)
	GETCOPY(uvec3, m_maxComputeWorkGroupCount, MaxComputeWorkGroupCount)
	GETCOPY(uvec3, m_maxComputeWorkGroupSize, MaxComputeWorkGroupSize)
	GETCOPY(uvec3, m_minComputeWorkGroupSize, MinComputeWorkGroupSize)
	GETCOPY(uint, m_maxPushConstantsSize, MaxPushConstantsSize)
	GETCOPY(uint, m_maxImageArrayLayers, MaxImageArrayLayers)
	GETCOPY(uint, m_maxImageDimension1D, MaxImageDimension1D)
	GETCOPY(uint, m_maxImageDimension2D, MaxImageDimension2D)
	GETCOPY(uint, m_maxImageDimension3D, MaxImageDimension3D)
	GETCOPY(uint, m_maxImageDimensionCube, MaxImageDimensionCube)
	GETCOPY_SET(bool, m_endApplicationMessage, EndApplicationMessage)
	GETCOPY(bool, m_firstFrameFinished, FirstFrameFinished)

protected:
	/** Build m_graphicsQueueQueryPool and m_computeQueueQueryPool query pools
	* @return nothing */
	void initializeQueryPools();

	/** Destroys m_graphicsQueueQueryPool and m_computeQueueQueryPool query pools
	* @return nothing */
	void destroyQueryPool();

	/** Adds a new device extension feature struct to the m_mapDeviceExtensionFeature map
	* @param structureType [in] structure type to add
	* @return nothing */
	template<class T> void addDeviceExtensionFeatureStruct(VkStructureType structureType)
	{
		T* extensionFeatureStruct = new T;
		extensionFeatureStruct->sType = structureType;
		extensionFeatureStruct->pNext = nullptr;
		m_mapDeviceExtensionFeature.insert(make_pair(structureType, (void*)extensionFeatureStruct));

		VkPhysicalDeviceFeatures2KHR physicalDeviceFeatures{ static_cast<VkStructureType>(VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2_KHR) };
		physicalDeviceFeatures.pNext = extensionFeatureStruct;
		vkfpM->vkGetPhysicalDeviceFeatures2KHR(getPhysicalDevice(), &physicalDeviceFeatures);

		if (m_deviceExtensionFeatureLinkedList != nullptr)
		{
			extensionFeatureStruct->pNext = m_deviceExtensionFeatureLinkedList;
		}

		m_deviceExtensionFeatureLinkedList = (void*)extensionFeatureStruct;
	}

	/** Adds a new device extension feature struct to the m_mapDeviceExtensionFeature map
	* @param structData [in] struct to add
	* @return nothing */
	template<class T> void addDeviceExtensionFeatureStruct(const T& structData)
	{
		T* extensionFeatureStruct = new T(structData);
		m_mapDeviceExtensionFeature.insert(make_pair(extensionFeatureStruct->sType, (void*)extensionFeatureStruct));

		VkPhysicalDeviceFeatures2KHR physicalDeviceFeatures{ static_cast<VkStructureType>(VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2_KHR) };
		physicalDeviceFeatures.pNext = extensionFeatureStruct;
		vkfpM->vkGetPhysicalDeviceFeatures2KHR(getPhysicalDevice(), &physicalDeviceFeatures);

		if (m_deviceExtensionFeatureLinkedList != nullptr)
		{
			extensionFeatureStruct->pNext = m_deviceExtensionFeatureLinkedList;
		}

		m_deviceExtensionFeatureLinkedList = (void*)extensionFeatureStruct;
	}

	/** Adds a new device extension feature struct to the m_mapDeviceExtensionFeature map, with pNext pointing to another provided struct
	* which is instantiated as well
	* @param structData      [in] struct to add
	* @param pNextStructData [in] struct to instantiate and add to pNextStructData
	* @return nothing */
	template<class T, class U> void addDeviceExtensionFeatureStruct(const T& structData, const U& pNextStructData)
	{
		T* extensionFeatureStruct = new T(structData);
		U* pNextStruct            = new U(pNextStructData);
		extensionFeatureStruct->pNext = pNextStruct;
		m_mapDeviceExtensionFeature.insert(make_pair(extensionFeatureStruct->sType, (void*)extensionFeatureStruct));

		VkPhysicalDeviceFeatures2KHR physicalDeviceFeatures{ static_cast<VkStructureType>(VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2_KHR) };
		physicalDeviceFeatures.pNext = extensionFeatureStruct;
		vkfpM->vkGetPhysicalDeviceFeatures2KHR(getPhysicalDevice(), &physicalDeviceFeatures);

		if (m_deviceExtensionFeatureLinkedList != nullptr)
		{
			extensionFeatureStruct->pNext = m_deviceExtensionFeatureLinkedList;
		}

		m_deviceExtensionFeatureLinkedList = (void*)extensionFeatureStruct;
	}

	/** Removes and deletes from m_mapDeviceExtensionFeature the device extension feature with VkStructureType given by the parameter
	* @param structureType [in] structure type to remove
	* @return true if a struct with the type given was found, false othherwise */
	template<class T> bool removeDeviceExtensionFeatureStruct(VkStructureType structureType)
	{
		map<VkStructureType, void*>::iterator it = m_mapDeviceExtensionFeature.find(structureType);

		if (it == m_mapDeviceExtensionFeature.end())
		{
			return false;
		}

		T* extensionFeatureStruct = static_cast<T*>(it->second);
		delete extensionFeatureStruct;
		m_mapDeviceExtensionFeature.erase(it);
		return true;
	}

	/** Removes and deletes from m_mapDeviceExtensionFeature the device extension feature with VkStructureType given 
	* by the first template parameter and with pNext struct given by second template parameter
	* @param structureType [in] structure type to remove
	* @return true if a struct with the type given was found, false othherwise */
	template<class T, class U> bool removeDeviceExtensionFeatureStruct(VkStructureType structureType)
	{
		map<VkStructureType, void*>::iterator it = m_mapDeviceExtensionFeature.find(structureType);

		if (it == m_mapDeviceExtensionFeature.end())
		{
			return false;
		}

		T* extensionFeatureStruct = static_cast<T*>(it->second);

		if (extensionFeatureStruct->pNext == nullptr)
		{
			cout << "ERROR: pNext field is null in template<class T, class U> bool removeDeviceExtensionFeatureStruct" << endl;
		}

		U* pNextStruct = static_cast<U*>(extensionFeatureStruct->pNext);

		extensionFeatureStruct->pNext = nullptr;

		delete pNextStruct;
		delete extensionFeatureStruct;
		m_mapDeviceExtensionFeature.erase(it);
		return true;
	}

	VkCommandPool               m_graphicsCommandPool;              //!< Graphics command pool
	VkCommandPool               m_computeCommandPool;               //!< Compute command pool
	VkRenderPass                m_renderPass;                       //!< Render pass created object
	Instance                    m_instance;                         //!< Instance wrapper class
	Surface                     m_surface;                          //!< Surface wrapper class
	LogicalDevice               m_logicalDevice;                    //!< Logical device wrapper class
	PhysicalDevice              m_physicalDevice;                   //!< Physical device wrapper class
	SwapChain                   m_swapChain;                        //!< Swap chain wrapper class
	uint32_t                    m_currentColorBuffer;               //!< Current drawing surface index in use
	VkSemaphore                 m_presentCompleteSemaphore;         //!< Semaphore for sync purposes
	VkSemaphore                 m_drawingCompleteSemaphore;         //!< Semaphore for sync purposes
	bool                        m_debugFlag;                        //!< If true, debug callback is added
	bool                        m_isPrepared;                       //!< If false, a resize is being performed, or the renderer object is preparing the next frame
	bool                        m_isResizing;                       //!< If true, a resize is being performed
	bool                        m_reachedFirstRaster;               //!< True when the first commands for recording rasterization for the first time are reached
	static int                  m_vectorRecordIndex;                //!< Index to record commands to m_arrayCommandDraw
	int                         m_lastSubmittedCommand;             //!< Index of the last submitted command
	static uint                 m_commandBufferIndex;               //!< Unique identifier to notify recording techniques
	VkQueryPool                 m_graphicsQueueQueryPool;           //!< Query pool for performance measurements for graphics queue
	VkQueryPool                 m_computeQueueQueryPool;            //!< Query pool for performance measurements for compute queue
	bool                        m_queryPoolsInitialized;            //!< True if query pools have been initialized
	uint                        m_maxComputeWorkGroupInvocations;   //!< Vulkan hardware limit that might be needed by more than one technique
	uint                        m_maxComputeSharedMemorySize;       //!< Vulkan hardware limit that might be needed by more than one technique
	uvec3                       m_maxComputeWorkGroupCount;         //!< Vulkan hardware limit that might be needed by more than one technique
	uvec3                       m_maxComputeWorkGroupSize;          //!< Vulkan hardware limit that might be needed by more than one technique
	uvec3                       m_minComputeWorkGroupSize;          //!< Vulkan hardware limit that might be needed by more than one technique
	uint                        m_maxPushConstantsSize;             //!< Vulkan hardware limit that might be needed by more than one technique
	uint                        m_maxImageArrayLayers;              //!< Vulkan hardware limit that might be needed by more than one technique
	uint                        m_maxImageDimension1D;              //!< Vulkan hardware limit that might be needed by more than one technique
	uint                        m_maxImageDimension2D;              //!< Vulkan hardware limit that might be needed by more than one technique
	uint                        m_maxImageDimension3D;              //!< Vulkan hardware limit that might be needed by more than one technique
	uint                        m_maxImageDimensionCube;            //!< Vulkan hardware limit that might be needed by more than one technique
	uint                        m_numAcquiredImages;                //!< Number of acquired images through PFN_vkAcquireNextImageKHR
	uint                        m_numPresentedUmages;               //!< Number of presented images through PFN_vkQueuePresentKHR
	uint                        m_maxImageNumberAdquired;           //!< Maximum number of images that can be aquired and have not been presented yet
	bool                        m_endApplicationMessage;            //!< Flag to know when a message to end application is received
	VkFence                     m_fence;                            //!< Fence used for command buffer submitting
	bool                        m_firstFrameFinished;               //!< To know when the first frame has been finished, updated in postRender method
	map<VkStructureType, void*> m_mapDeviceExtensionFeature;        //!< Map where to store the device extension feature struct pointers for those extensions that need it to be loaded
	void*                       m_deviceExtensionFeatureLinkedList; //!< Pointer to the start of the linked list of requested device extension features that require a struct to be loaded
};

static CoreManager* s_pCoreManager;

/////////////////////////////////////////////////////////////////////////////////////////////

// INLINE METHODS

/////////////////////////////////////////////////////////////////////////////////////////////

inline VkFormat CoreManager::getSurfaceFormat()
{
	return m_surface.getFormat();
}

/////////////////////////////////////////////////////////////////////////////////////////////

inline const vector<VkQueueFamilyProperties>& CoreManager::getQueueFamilyProps()
{
	return m_physicalDevice.getQueueFamilyProps();
}

/////////////////////////////////////////////////////////////////////////////////////////////

inline const vector<VkImage>& CoreManager::getArraySwapchainImages()
{
	return m_swapChain.getArraySwapchainImages();
}

/////////////////////////////////////////////////////////////////////////////////////////////

inline const vector<Framebuffer*>& CoreManager::getArrayFramebuffers()
{
	return m_swapChain.getArrayFramebuffers();
}

/////////////////////////////////////////////////////////////////////////////////////////////

inline VkExtent2D CoreManager::setSwapChainExtent()
{
	VkExtent2D extent = m_surface.setSwapChainExtent();

	return extent;
}

/////////////////////////////////////////////////////////////////////////////////////////////

inline void CoreManager::managePresentMode(VkPresentModeKHR& presentMode, uint32_t& numberOfSwapChainImages)
{
	m_surface.managePresentMode(presentMode, numberOfSwapChainImages);
}

/////////////////////////////////////////////////////////////////////////////////////////////

inline bool CoreManager::memoryTypeFromProperties(uint32_t typeBits, VkFlags requirementsMask, const VkMemoryType* memoryTypes, uint32_t& typeIndex)
{
	return m_physicalDevice.memoryTypeFromProperties(typeBits, requirementsMask, memoryTypes, typeIndex);
}

/////////////////////////////////////////////////////////////////////////////////////////////

inline const VkPhysicalDeviceMemoryProperties& CoreManager::getPhysicalDeviceMemoryProperties()
{
	return m_physicalDevice.getPhysicalDeviceMemoryProperties();
}

/////////////////////////////////////////////////////////////////////////////////////////////

inline uint32_t CoreManager::getWidth() const
{
	return m_surface.getWidth();
}

/////////////////////////////////////////////////////////////////////////////////////////////

inline uint32_t CoreManager::getHeight() const
{
	return m_surface.getHeight();
}

/////////////////////////////////////////////////////////////////////////////////////////////

inline VkSurfaceTransformFlagBitsKHR CoreManager::getPreTransform() const
{
	return m_surface.getPreTransform();
}

/////////////////////////////////////////////////////////////////////////////////////////////

inline const VkInstance CoreManager::getInstance() const
{
	return m_instance.getInstance();
}

/////////////////////////////////////////////////////////////////////////////////////////////

inline const VkSurfaceKHR CoreManager::getSurface() const
{
	return m_surface.getSurface();
}

/////////////////////////////////////////////////////////////////////////////////////////////

inline const VkPhysicalDevice CoreManager::getPhysicalDevice() const
{
	return m_physicalDevice.getPhysicalDevice();
}

/////////////////////////////////////////////////////////////////////////////////////////////

inline const VkDevice& CoreManager::getLogicalDevice() const
{
	return m_logicalDevice.getLogicalDevice();
}

/////////////////////////////////////////////////////////////////////////////////////////////

inline const VkQueue CoreManager::getLogicalDeviceGraphicsQueue() const
{
	return m_logicalDevice.getLogicalDeviceGraphicsQueue();
}

/////////////////////////////////////////////////////////////////////////////////////////////

inline const VkQueue CoreManager::getLogicalDeviceComputeQueue() const
{
	return m_logicalDevice.getLogicalDeviceComputeQueue();
}

/////////////////////////////////////////////////////////////////////////////////////////////

inline const VkPhysicalDeviceProperties& CoreManager::getPhysicalDeviceProperties() const
{
	return m_physicalDevice.getPhysicalDeviceProperties();
}

/////////////////////////////////////////////////////////////////////////////////////////////

inline const VkPhysicalDeviceRayTracingPipelinePropertiesKHR& CoreManager::getkPhysicalDeviceRayTracingPipelineProperties() const
{
	return m_physicalDevice.getPhysicalDeviceRayTracingPipelineProperties();
}

/////////////////////////////////////////////////////////////////////////////////////////////

inline const VkPhysicalDeviceSubgroupProperties& CoreManager::getVkPhysicalDeviceSubgroupProperties() const
{
	return m_physicalDevice.getPhysicalDeviceSubgroupProperties();
}

/////////////////////////////////////////////////////////////////////////////////////////////

inline const VkPhysicalDeviceConservativeRasterizationPropertiesEXT& CoreManager::getVkPhysicalDeviceConservativeRasterizationProperties() const
{
	return m_physicalDevice.getPhysicalDeviceConservativeRasterizationProperties();
}

/////////////////////////////////////////////////////////////////////////////////////////////

inline const HWND CoreManager::getWindowPlatformHandle() const
{
	return m_surface.getWindow();
}

/////////////////////////////////////////////////////////////////////////////////////////////

#endif _COREMANAGER_H_
