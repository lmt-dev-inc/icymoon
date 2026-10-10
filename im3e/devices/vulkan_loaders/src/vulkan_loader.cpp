#include "vulkan_loader.h"

#if defined(__linux__)
#include "vulkan_library_loader_linux.h"
#else
static_assert(false, "Vulkan Loader does not support this platform at the moment");
#endif

using namespace im3e;

#define LOAD_GLOBAL_FCT(fctName) .fctName = reinterpret_cast<PFN_##fctName>(m_vkGetInstanceProcAddr(nullptr, #fctName))
#define LOAD_INST_FCT(fctName) .fctName = reinterpret_cast<PFN_##fctName>(m_vkGetInstanceProcAddr(vkInstance, #fctName))
#define LOAD_DEVICE_FCT(fctName) .fctName = reinterpret_cast<PFN_##fctName>(m_vkGetDeviceProcAddr(vkDevice, #fctName))

/// Alternative functions specific to VMA function loading where VmaVulkanFunctions's function names are suffixed with
/// KHR but we load their core version (i.e. without the KHR suffix)
#define LOAD_INST_FCT_KHR(fctNameKHR, fctName) \
    .fctNameKHR = reinterpret_cast<PFN_##fctName>(m_vkGetInstanceProcAddr(vkInstance, #fctName))
#define LOAD_DEVICE_FCT_KHR(fctNameKHR, fctName) \
    .fctNameKHR = reinterpret_cast<PFN_##fctName>(m_vkGetDeviceProcAddr(vkDevice, #fctName))

VulkanLoader::VulkanLoader(VulkanLoaderConfig config, UniquePtrWithDeleter<void> pLibrary,
                           PFN_vkGetInstanceProcAddr vkGetInstanceProcAddr, PFN_vkGetDeviceProcAddr vkGetDeviceProcAddr)
  : m_config(std::move(config))
  , m_pLibrary(throwIfArgNull(std::move(pLibrary), "Vulkan loader requires a library pointer"))
  , m_vkGetInstanceProcAddr(throwIfArgNull(vkGetInstanceProcAddr, "Vulkan loader requires vkGetInstanceProcAddr"))
  , m_vkGetDeviceProcAddr(throwIfArgNull(vkGetDeviceProcAddr, "Vulkan loader requires vkGetDeviceProcAddr"))
{
}

auto VulkanLoader::loadGlobalFcts() const -> VulkanGlobalFcts
{
    return VulkanGlobalFcts{
        LOAD_GLOBAL_FCT(vkEnumerateInstanceVersion),
        LOAD_GLOBAL_FCT(vkEnumerateInstanceExtensionProperties),
        LOAD_GLOBAL_FCT(vkEnumerateInstanceLayerProperties),
        LOAD_GLOBAL_FCT(vkCreateInstance),
    };
}
auto VulkanLoader::loadInstanceFcts(VkInstance vkInstance) const -> VulkanInstanceFcts
{
    throwIfArgNull(vkInstance, "Cannot load Vulkan instance functions without an instance");

    VulkanInstanceFcts fcts{
        LOAD_INST_FCT(vkGetInstanceProcAddr),
        LOAD_INST_FCT(vkDestroyInstance),
        LOAD_INST_FCT(vkCreateDevice),
        LOAD_INST_FCT(vkDestroySurfaceKHR),

        LOAD_INST_FCT(vkEnumeratePhysicalDevices),
        LOAD_INST_FCT(vkGetPhysicalDeviceProperties),
        LOAD_INST_FCT(vkGetPhysicalDeviceFeatures),
        LOAD_INST_FCT(vkEnumerateDeviceExtensionProperties),
        LOAD_INST_FCT(vkGetPhysicalDeviceQueueFamilyProperties),
        LOAD_INST_FCT(vkGetPhysicalDeviceMemoryProperties),

        LOAD_INST_FCT(vkGetPhysicalDeviceSurfaceCapabilitiesKHR),
        LOAD_INST_FCT(vkGetPhysicalDeviceSurfaceFormatsKHR),
        LOAD_INST_FCT(vkGetPhysicalDeviceSurfacePresentModesKHR),
    };
    if (m_config.isDebugEnabled)
    {
        fcts LOAD_INST_FCT(vkCreateDebugUtilsMessengerEXT);
        fcts LOAD_INST_FCT(vkDestroyDebugUtilsMessengerEXT);
    }
    return fcts;
}
auto VulkanLoader::loadDeviceFcts(VkDevice vkDevice) const -> VulkanDeviceFcts
{
    throwIfArgNull(vkDevice, "Cannot load Vulkan device functions without a device");

    VulkanDeviceFcts fcts{
        LOAD_DEVICE_FCT(vkDestroyDevice),
        LOAD_DEVICE_FCT(vkDeviceWaitIdle),
        LOAD_DEVICE_FCT(vkGetDeviceQueue),
        LOAD_DEVICE_FCT(vkQueueSubmit),
        LOAD_DEVICE_FCT(vkQueueWaitIdle),
        LOAD_DEVICE_FCT(vkGetImageSubresourceLayout),

        LOAD_DEVICE_FCT(vkCreateCommandPool),
        LOAD_DEVICE_FCT(vkDestroyCommandPool),
        LOAD_DEVICE_FCT(vkAllocateCommandBuffers),
        LOAD_DEVICE_FCT(vkFreeCommandBuffers),
        LOAD_DEVICE_FCT(vkResetCommandBuffer),
        LOAD_DEVICE_FCT(vkBeginCommandBuffer),
        LOAD_DEVICE_FCT(vkEndCommandBuffer),
        LOAD_DEVICE_FCT(vkCmdPipelineBarrier2),
        LOAD_DEVICE_FCT(vkCmdClearColorImage),
        LOAD_DEVICE_FCT(vkCmdBlitImage),
        LOAD_DEVICE_FCT(vkCmdCopyImage),
        LOAD_DEVICE_FCT(vkCmdBeginRenderPass),
        LOAD_DEVICE_FCT(vkCmdEndRenderPass),

        LOAD_DEVICE_FCT(vkCreateFence),
        LOAD_DEVICE_FCT(vkDestroyFence),
        LOAD_DEVICE_FCT(vkWaitForFences),
        LOAD_DEVICE_FCT(vkResetFences),

        LOAD_DEVICE_FCT(vkCreateSemaphore),
        LOAD_DEVICE_FCT(vkDestroySemaphore),

        LOAD_DEVICE_FCT(vkCreateSwapchainKHR),
        LOAD_DEVICE_FCT(vkDestroySwapchainKHR),
        LOAD_DEVICE_FCT(vkGetSwapchainImagesKHR),
        LOAD_DEVICE_FCT(vkAcquireNextImageKHR),
        LOAD_DEVICE_FCT(vkQueuePresentKHR),

        LOAD_DEVICE_FCT(vkCreateFramebuffer),
        LOAD_DEVICE_FCT(vkDestroyFramebuffer),
        LOAD_DEVICE_FCT(vkCreateRenderPass),
        LOAD_DEVICE_FCT(vkDestroyRenderPass),
        LOAD_DEVICE_FCT(vkCreateDescriptorPool),
        LOAD_DEVICE_FCT(vkDestroyDescriptorPool),
        LOAD_DEVICE_FCT(vkCreateImageView),
        LOAD_DEVICE_FCT(vkDestroyImageView),
        LOAD_DEVICE_FCT(vkCreateSampler),
        LOAD_DEVICE_FCT(vkDestroySampler),
    };
    if (m_config.isDebugEnabled)
    {
        fcts LOAD_DEVICE_FCT(vkSetDebugUtilsObjectNameEXT);
    }
    return fcts;
}

auto VulkanLoader::loadVmaFcts(VkInstance vkInstance, VkDevice vkDevice) const -> VmaVulkanFunctions
{
    throwIfArgNull(vkInstance, "Cannot load VMA functions without an instance");
    throwIfArgNull(vkDevice, "Cannot load VMA functions without a device");

    return VmaVulkanFunctions{
        LOAD_INST_FCT(vkGetPhysicalDeviceProperties),
        LOAD_INST_FCT(vkGetPhysicalDeviceMemoryProperties),
        LOAD_DEVICE_FCT(vkAllocateMemory),
        LOAD_DEVICE_FCT(vkFreeMemory),
        LOAD_DEVICE_FCT(vkMapMemory),
        LOAD_DEVICE_FCT(vkUnmapMemory),
        LOAD_DEVICE_FCT(vkFlushMappedMemoryRanges),
        LOAD_DEVICE_FCT(vkInvalidateMappedMemoryRanges),
        LOAD_DEVICE_FCT(vkBindBufferMemory),
        LOAD_DEVICE_FCT(vkBindImageMemory),
        LOAD_DEVICE_FCT(vkGetBufferMemoryRequirements),
        LOAD_DEVICE_FCT(vkGetImageMemoryRequirements),
        LOAD_DEVICE_FCT(vkCreateBuffer),
        LOAD_DEVICE_FCT(vkDestroyBuffer),
        LOAD_DEVICE_FCT(vkCreateImage),
        LOAD_DEVICE_FCT(vkDestroyImage),
        LOAD_DEVICE_FCT(vkCmdCopyBuffer),
        LOAD_DEVICE_FCT_KHR(vkGetBufferMemoryRequirements2KHR, vkGetBufferMemoryRequirements2),
        LOAD_DEVICE_FCT_KHR(vkGetImageMemoryRequirements2KHR, vkGetImageMemoryRequirements2),
        LOAD_DEVICE_FCT_KHR(vkBindBufferMemory2KHR, vkBindBufferMemory2),
        LOAD_DEVICE_FCT_KHR(vkBindImageMemory2KHR, vkBindImageMemory2),
        LOAD_INST_FCT_KHR(vkGetPhysicalDeviceMemoryProperties2KHR, vkGetPhysicalDeviceMemoryProperties2),
        LOAD_DEVICE_FCT(vkGetDeviceBufferMemoryRequirements),
        LOAD_DEVICE_FCT(vkGetDeviceImageMemoryRequirements),
    };
}

auto im3e::createVulkanLoader(VulkanLoaderConfig config) -> std::unique_ptr<IVulkanLoader>
{
    vulkan_loader::VulkanLibraryLoader libraryLoader;
    return std::make_unique<VulkanLoader>(std::move(config), std::move(libraryLoader.pLibrary),
                                          libraryLoader.vkGetInstanceProcAddr, libraryLoader.vkGetDeviceProcAddr);
}