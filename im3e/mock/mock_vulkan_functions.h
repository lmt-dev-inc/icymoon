#pragma once

#include <im3e/test_utils/test_utils.h>
#include <im3e/vulkan/vulkan_functions.h>

namespace im3e {

class MockVulkanGlobalFcts
{
public:
    MockVulkanGlobalFcts();
    virtual ~MockVulkanGlobalFcts();

    MOCK_METHOD(VkResult, vkEnumerateInstanceVersion, (uint32_t * pApiVersion));
    MOCK_METHOD(VkResult, vkEnumerateInstanceExtensionProperties,
                (const char* pLayerName, uint32_t* pPropertyCount, VkExtensionProperties* pProperties));
    MOCK_METHOD(VkResult, vkEnumerateInstanceLayerProperties,
                (uint32_t * pPropertyCount, VkLayerProperties* pProperties));
    MOCK_METHOD(VkResult, vkCreateInstance,
                (const VkInstanceCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator,
                 VkInstance* pInstance));
};

class MockVulkanInstanceFcts
{
public:
    MockVulkanInstanceFcts();
    virtual ~MockVulkanInstanceFcts();

    MOCK_METHOD(PFN_vkVoidFunction, vkGetInstanceProcAddr, (VkInstance instance, const char* pName));
    MOCK_METHOD(void, vkDestroyInstance, (VkInstance instance, const VkAllocationCallbacks* pAllocator));
    MOCK_METHOD(VkResult, vkCreateDevice,
                (VkPhysicalDevice physicalDevice, const VkDeviceCreateInfo* pCreateInfo,
                 const VkAllocationCallbacks* pAllocator, VkDevice* pDevice));
    MOCK_METHOD(void, vkDestroySurfaceKHR,
                (VkInstance instance, VkSurfaceKHR surface, const VkAllocationCallbacks* pAllocator));

    MOCK_METHOD(VkResult, vkCreateDebugUtilsMessengerEXT,
                (VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo,
                 const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pMessenger));
    MOCK_METHOD(void, vkDestroyDebugUtilsMessengerEXT,
                (VkInstance instance, VkDebugUtilsMessengerEXT messenger, const VkAllocationCallbacks* pAllocator));

    MOCK_METHOD(VkResult, vkEnumeratePhysicalDevices,
                (VkInstance instance, uint32_t* pPhysicalDeviceCount, VkPhysicalDevice* pPhysicalDevices));
    MOCK_METHOD(void, vkGetPhysicalDeviceProperties,
                (VkPhysicalDevice physicalDevice, VkPhysicalDeviceProperties* pProperties));
    MOCK_METHOD(void, vkGetPhysicalDeviceFeatures,
                (VkPhysicalDevice physicalDevice, VkPhysicalDeviceFeatures* pFeatures));
    MOCK_METHOD(VkResult, vkEnumerateDeviceExtensionProperties,
                (VkPhysicalDevice physicalDevice, const char* pLayerName, uint32_t* pPropertyCount,
                 VkExtensionProperties* pProperties));
    MOCK_METHOD(void, vkGetPhysicalDeviceQueueFamilyProperties,
                (VkPhysicalDevice physicalDevice, uint32_t* pQueueFamilyPropertyCount,
                 VkQueueFamilyProperties* pQueueFamilyProperties));
    MOCK_METHOD(void, vkGetPhysicalDeviceMemoryProperties,
                (VkPhysicalDevice physicalDevice, VkPhysicalDeviceMemoryProperties* pMemoryProperties));

    MOCK_METHOD(VkResult, vkGetPhysicalDeviceSurfaceCapabilitiesKHR,
                (VkPhysicalDevice physicalDevice, VkSurfaceKHR surface,
                 VkSurfaceCapabilitiesKHR* pSurfaceCapabilities));
    MOCK_METHOD(VkResult, vkGetPhysicalDeviceSurfaceFormatsKHR,
                (VkPhysicalDevice physicalDevice, VkSurfaceKHR surface, uint32_t* pSurfaceFormatCount,
                 VkSurfaceFormatKHR* pSurfaceFormats));
    MOCK_METHOD(VkResult, vkGetPhysicalDeviceSurfacePresentModesKHR,
                (VkPhysicalDevice physicalDevice, VkSurfaceKHR surface, uint32_t* pPresentModeCount,
                 VkPresentModeKHR* pPresentModes));
};

class MockVulkanDeviceFcts
{
public:
    MockVulkanDeviceFcts();
    virtual ~MockVulkanDeviceFcts();

    MOCK_METHOD(void, vkDestroyDevice, (VkDevice device, const VkAllocationCallbacks* pAllocator));
    MOCK_METHOD(VkResult, vkDeviceWaitIdle, (VkDevice device));
    MOCK_METHOD(void, vkGetDeviceQueue,
                (VkDevice device, uint32_t queueFamilyIndex, uint32_t queueIndex, VkQueue* pQueue));
    MOCK_METHOD(VkResult, vkQueueSubmit,
                (VkQueue queue, uint32_t submitCount, const VkSubmitInfo* pSubmits, VkFence fence));
    MOCK_METHOD(VkResult, vkQueueWaitIdle, (VkQueue queue));
    MOCK_METHOD(void, vkGetImageSubresourceLayout,
                (VkDevice device, VkImage image, const VkImageSubresource* pSubresource, VkSubresourceLayout* pLayout));

    MOCK_METHOD(VkResult, vkSetDebugUtilsObjectNameEXT,
                (VkDevice device, const VkDebugUtilsObjectNameInfoEXT* pNameInfo));

    MOCK_METHOD(VkResult, vkCreateCommandPool,
                (VkDevice device, const VkCommandPoolCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator,
                 VkCommandPool* pCommandPool));
    MOCK_METHOD(void, vkDestroyCommandPool,
                (VkDevice device, VkCommandPool commandPool, const VkAllocationCallbacks* pAllocator));
    MOCK_METHOD(VkResult, vkAllocateCommandBuffers,
                (VkDevice device, const VkCommandBufferAllocateInfo* pAllocateInfo, VkCommandBuffer* pCommandBuffers));
    MOCK_METHOD(void, vkFreeCommandBuffers,
                (VkDevice device, VkCommandPool commandPool, uint32_t commandBufferCount,
                 const VkCommandBuffer* pCommandBuffers));
    MOCK_METHOD(VkResult, vkResetCommandBuffer, (VkCommandBuffer commandBuffer, VkCommandBufferResetFlags flags));
    MOCK_METHOD(VkResult, vkBeginCommandBuffer,
                (VkCommandBuffer commandBuffer, const VkCommandBufferBeginInfo* pBeginInfo));
    MOCK_METHOD(VkResult, vkEndCommandBuffer, (VkCommandBuffer commandBuffer));
    MOCK_METHOD(void, vkCmdPipelineBarrier2, (VkCommandBuffer commandBuffer, const VkDependencyInfo* pDependencyInfo));
    MOCK_METHOD(void, vkCmdClearColorImage,
                (VkCommandBuffer commandBuffer, VkImage image, VkImageLayout imageLayout,
                 const VkClearColorValue* pColor, uint32_t rangeCount, const VkImageSubresourceRange* pRanges));
    MOCK_METHOD(void, vkCmdBlitImage,
                (VkCommandBuffer commandBuffer, VkImage srcImage, VkImageLayout srcImageLayout, VkImage dstImage,
                 VkImageLayout dstImageLayout, uint32_t regionCount, const VkImageBlit* pRegions, VkFilter filter));
    MOCK_METHOD(void, vkCmdCopyImage,
                (VkCommandBuffer commandBuffer, VkImage srcImage, VkImageLayout srcImageLayout, VkImage dstImage,
                 VkImageLayout dstImageLayout, uint32_t regionCount, const VkImageCopy* pRegions));
    MOCK_METHOD(void, vkCmdBeginRenderPass,
                (VkCommandBuffer commandBuffer, const VkRenderPassBeginInfo* pRenderPassBegin,
                 VkSubpassContents contents));
    MOCK_METHOD(void, vkCmdEndRenderPass, (VkCommandBuffer commandBuffer));

    MOCK_METHOD(VkResult, vkCreateFence,
                (VkDevice device, const VkFenceCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator,
                 VkFence* pFence));
    MOCK_METHOD(void, vkDestroyFence, (VkDevice device, VkFence fence, const VkAllocationCallbacks* pAllocator));
    MOCK_METHOD(VkResult, vkWaitForFences,
                (VkDevice device, uint32_t fenceCount, const VkFence* pFences, VkBool32 waitAll, uint64_t timeout));
    MOCK_METHOD(VkResult, vkResetFences, (VkDevice device, uint32_t fenceCount, const VkFence* pFences));

    MOCK_METHOD(VkResult, vkCreateSemaphore,
                (VkDevice device, const VkSemaphoreCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator,
                 VkSemaphore* pSemaphore));
    MOCK_METHOD(void, vkDestroySemaphore,
                (VkDevice device, VkSemaphore semaphore, const VkAllocationCallbacks* pAllocator));

    MOCK_METHOD(VkResult, vkCreateSwapchainKHR,
                (VkDevice device, const VkSwapchainCreateInfoKHR* pCreateInfo, const VkAllocationCallbacks* pAllocator,
                 VkSwapchainKHR* pSwapchain));
    MOCK_METHOD(void, vkDestroySwapchainKHR,
                (VkDevice device, VkSwapchainKHR swapchain, const VkAllocationCallbacks* pAllocator));
    MOCK_METHOD(VkResult, vkGetSwapchainImagesKHR,
                (VkDevice device, VkSwapchainKHR swapchain, uint32_t* pSwapchainImageCount, VkImage* pSwapchainImages));
    MOCK_METHOD(VkResult, vkAcquireNextImageKHR,
                (VkDevice device, VkSwapchainKHR swapchain, uint64_t timeout, VkSemaphore semaphore, VkFence fence,
                 uint32_t* pImageIndex));
    MOCK_METHOD(VkResult, vkQueuePresentKHR, (VkQueue queue, const VkPresentInfoKHR* pPresentInfo));

    MOCK_METHOD(VkResult, vkCreateFramebuffer,
                (VkDevice device, const VkFramebufferCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator,
                 VkFramebuffer* pFramebuffer));
    MOCK_METHOD(void, vkDestroyFramebuffer,
                (VkDevice device, VkFramebuffer framebuffer, const VkAllocationCallbacks* pAllocator));
    MOCK_METHOD(VkResult, vkCreateRenderPass,
                (VkDevice device, const VkRenderPassCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator,
                 VkRenderPass* pRenderPass));
    MOCK_METHOD(void, vkDestroyRenderPass,
                (VkDevice device, VkRenderPass renderPass, const VkAllocationCallbacks* pAllocator));
    MOCK_METHOD(VkResult, vkCreateDescriptorPool,
                (VkDevice device, const VkDescriptorPoolCreateInfo* pCreateInfo,
                 const VkAllocationCallbacks* pAllocator, VkDescriptorPool* pDescriptorPool));
    MOCK_METHOD(void, vkDestroyDescriptorPool,
                (VkDevice device, VkDescriptorPool descriptorPool, const VkAllocationCallbacks* pAllocator));
    MOCK_METHOD(VkResult, vkCreateImageView,
                (VkDevice device, const VkImageViewCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator,
                 VkImageView* pView));
    MOCK_METHOD(void, vkDestroyImageView,
                (VkDevice device, VkImageView imageView, const VkAllocationCallbacks* pAllocator));
    MOCK_METHOD(VkResult, vkCreateSampler,
                (VkDevice device, const VkSamplerCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator,
                 VkSampler* pSampler));
    MOCK_METHOD(void, vkDestroySampler, (VkDevice device, VkSampler sampler, const VkAllocationCallbacks* pAllocator));
};

class MockVmaVulkanFunctions
{
public:
    MockVmaVulkanFunctions();
    virtual ~MockVmaVulkanFunctions();

    MOCK_METHOD(void, vkGetPhysicalDeviceProperties,
                (VkPhysicalDevice physicalDevice, VkPhysicalDeviceProperties* pProperties));
    MOCK_METHOD(void, vkGetPhysicalDeviceMemoryProperties,
                (VkPhysicalDevice physicalDevice, VkPhysicalDeviceMemoryProperties* pMemoryProperties));
    MOCK_METHOD(VkResult, vkAllocateMemory,
                (VkDevice device, const VkMemoryAllocateInfo* pAllocateInfo, const VkAllocationCallbacks* pAllocator,
                 VkDeviceMemory* pMemory));
    MOCK_METHOD(void, vkFreeMemory, (VkDevice device, VkDeviceMemory memory, const VkAllocationCallbacks* pAllocator));
    MOCK_METHOD(VkResult, vkMapMemory,
                (VkDevice device, VkDeviceMemory memory, VkDeviceSize offset, VkDeviceSize size, VkMemoryMapFlags flags,
                 void** ppData));
    MOCK_METHOD(void, vkUnmapMemory, (VkDevice device, VkDeviceMemory memory));
    MOCK_METHOD(VkResult, vkFlushMappedMemoryRanges,
                (VkDevice device, uint32_t memoryRangeCount, const VkMappedMemoryRange* pMemoryRanges));
    MOCK_METHOD(VkResult, vkInvalidateMappedMemoryRanges,
                (VkDevice device, uint32_t memoryRangeCount, const VkMappedMemoryRange* pMemoryRanges));
    MOCK_METHOD(VkResult, vkBindBufferMemory,
                (VkDevice device, VkBuffer buffer, VkDeviceMemory memory, VkDeviceSize memoryOffset));
    MOCK_METHOD(VkResult, vkBindImageMemory,
                (VkDevice device, VkImage image, VkDeviceMemory memory, VkDeviceSize memoryOffset));
    MOCK_METHOD(void, vkGetBufferMemoryRequirements,
                (VkDevice device, VkBuffer buffer, VkMemoryRequirements* pMemoryRequirements));
    MOCK_METHOD(void, vkGetImageMemoryRequirements,
                (VkDevice device, VkImage image, VkMemoryRequirements* pMemoryRequirements));
    MOCK_METHOD(VkResult, vkCreateBuffer,
                (VkDevice device, const VkBufferCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator,
                 VkBuffer* pBuffer));
    MOCK_METHOD(void, vkDestroyBuffer, (VkDevice device, VkBuffer buffer, const VkAllocationCallbacks* pAllocator));
    MOCK_METHOD(VkResult, vkCreateImage,
                (VkDevice device, const VkImageCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator,
                 VkImage* pImage));
    MOCK_METHOD(void, vkDestroyImage, (VkDevice device, VkImage image, const VkAllocationCallbacks* pAllocator));
    MOCK_METHOD(void, vkCmdCopyBuffer,
                (VkCommandBuffer commandBuffer, VkBuffer srcBuffer, VkBuffer dstBuffer, uint32_t regionCount,
                 const VkBufferCopy* pRegions));
    MOCK_METHOD(void, vkGetBufferMemoryRequirements2KHR,
                (VkDevice device, const VkBufferMemoryRequirementsInfo2* pInfo,
                 VkMemoryRequirements2* pMemoryRequirements));
    MOCK_METHOD(void, vkGetImageMemoryRequirements2KHR,
                (VkDevice device, const VkImageMemoryRequirementsInfo2* pInfo,
                 VkMemoryRequirements2* pMemoryRequirements));
    MOCK_METHOD(VkResult, vkBindBufferMemory2KHR,
                (VkDevice device, uint32_t bindInfoCount, const VkBindBufferMemoryInfo* pBindInfos));
    MOCK_METHOD(VkResult, vkBindImageMemory2KHR,
                (VkDevice device, uint32_t bindInfoCount, const VkBindImageMemoryInfo* pBindInfos));
    MOCK_METHOD(void, vkGetPhysicalDeviceMemoryProperties2KHR,
                (VkPhysicalDevice physicalDevice, VkPhysicalDeviceMemoryProperties2* pMemoryProperties));
    MOCK_METHOD(void, vkGetDeviceBufferMemoryRequirements,
                (VkDevice device, const VkDeviceBufferMemoryRequirements* pInfo,
                 VkMemoryRequirements2* pMemoryRequirements));
    MOCK_METHOD(void, vkGetDeviceImageMemoryRequirements,
                (VkDevice device, const VkDeviceImageMemoryRequirements* pInfo,
                 VkMemoryRequirements2* pMemoryRequirements));
};

class MockVulkanFunctions
{
public:
    MockVulkanFunctions();
    virtual ~MockVulkanFunctions();

    auto getGlobalFcts() const -> const VulkanGlobalFcts& { return m_gFcts; }
    auto getInstanceFcts() const -> const VulkanInstanceFcts& { return m_iFcts; }
    auto getDeviceFcts() const -> const VulkanDeviceFcts& { return m_dFcts; }
    auto getVmaFcts() const -> const VmaVulkanFunctions& { return m_vmaFcts; }

    auto getMockGlobalFcts() -> MockVulkanGlobalFcts& { return m_mockGFcts; }
    auto getMockInstanceFcts() -> MockVulkanInstanceFcts& { return m_mockIFcts; }
    auto getMockDeviceFcts() -> MockVulkanDeviceFcts& { return m_mockDFcts; }
    auto getMockVmaFcts() -> MockVmaVulkanFunctions& { return m_mockVmaFcts; }

private:
    VulkanGlobalFcts m_gFcts;
    VulkanInstanceFcts m_iFcts;
    VulkanDeviceFcts m_dFcts;
    VmaVulkanFunctions m_vmaFcts;

    NiceMock<MockVulkanGlobalFcts> m_mockGFcts;
    NiceMock<MockVulkanInstanceFcts> m_mockIFcts;
    NiceMock<MockVulkanDeviceFcts> m_mockDFcts;
    NiceMock<MockVmaVulkanFunctions> m_mockVmaFcts;
};

}  // namespace im3e