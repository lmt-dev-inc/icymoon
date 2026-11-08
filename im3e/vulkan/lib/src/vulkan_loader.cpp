#include "vulkan_loader.h"

#include <im3e/utils/core/throw_utils.h>
#include <im3e/utils/core/types.h>

#include <vulkan/vulkan.h>

#if defined(__linux__)
#include <dlfcn.h>
#elif defined(_WIN32)
#include <windows.h>
#else
static_assert(false, "Vulkan Loader does not support this platform at the moment");
#endif

#include <array>
#include <memory>
#include <string_view>

using namespace im3e;

namespace {

/// @brief Loads the Vulkan library of the system and its entry points.
struct VulkanLibraryLoader
{
    VulkanLibraryLoader();

    UniquePtrWithDeleter<void> pLibrary;
    PFN_vkGetInstanceProcAddr vkGetInstanceProcAddr{};
    PFN_vkGetDeviceProcAddr vkGetDeviceProcAddr{};
};

#if defined(__linux__)
VulkanLibraryLoader::VulkanLibraryLoader()
{
    constexpr std::array<std::string_view, 2U> VulkanLibraryNames{
        "libvulkan.so.1",
        "libvulkan.so",
    };

    for (const auto& rVulkanLibraryName : VulkanLibraryNames)
    {
        if (void* pLib = dlopen(rVulkanLibraryName.data(), RTLD_NOW))
        {
            pLibrary = UniquePtrWithDeleter<void>(pLib, [](void* pLib) { dlclose(pLib); });
            break;
        }
    }
    throwIfNull<std::runtime_error>(pLibrary.get(), "Could not load Vulkan shared library file");

    vkGetInstanceProcAddr = throwIfNull<std::runtime_error>(
        reinterpret_cast<PFN_vkGetInstanceProcAddr>(dlsym(pLibrary.get(), "vkGetInstanceProcAddr")),
        "Failed to load vkGetInstanceProcAddr");

    vkGetDeviceProcAddr = throwIfNull<std::runtime_error>(
        reinterpret_cast<PFN_vkGetDeviceProcAddr>(dlsym(pLibrary.get(), "vkGetDeviceProcAddr")),
        "Failed to load vkGetDeviceProcAddr");
}
#elif defined(_WIN32)
auto loadFct(HMODULE hModule, LPCSTR fctName) -> void*
{
    // GetProcAddress returns FARPROC which is already a function pointer. This causes a warning a clang when
    // trying to reinterpret_cast to the actual function pointer type. To avoid this warning, we cast the result
    // to void* first:
    return reinterpret_cast<void*>(GetProcAddress(hModule, fctName));
}

VulkanLibraryLoader::VulkanLibraryLoader()
{
    constexpr auto VulkanLibraryName{L"vulkan-1.dll"};
    if (HMODULE hModule = LoadLibraryExW(VulkanLibraryName, nullptr, LOAD_LIBRARY_SEARCH_SYSTEM32))
    {
        pLibrary = UniquePtrWithDeleter<void>(hModule,
                                              [](void* hModule) { FreeLibrary(static_cast<HMODULE>(hModule)); });
    }

    throwIfNull<std::runtime_error>(pLibrary.get(), "Could not load Vulkan shared library file");
    const auto hModule = static_cast<HMODULE>(pLibrary.get());

    vkGetInstanceProcAddr = throwIfNull<std::runtime_error>(
        reinterpret_cast<PFN_vkGetInstanceProcAddr>(loadFct(hModule, "vkGetInstanceProcAddr")),
        "Failed to load vkGetInstanceProcAddr");

    vkGetDeviceProcAddr = throwIfNull<std::runtime_error>(
        reinterpret_cast<PFN_vkGetDeviceProcAddr>(loadFct(hModule, "vkGetDeviceProcAddr")),
        "Failed to load vkGetDeviceProcAddr");
}
#endif

class VulkanLoader : public IVulkanLoader
{
public:
    VulkanLoader(VulkanLoaderConfig config, UniquePtrWithDeleter<void> pLibrary,
                 PFN_vkGetInstanceProcAddr vkGetInstanceProcAddr, PFN_vkGetDeviceProcAddr vkGetDeviceProcAddr);

    auto loadGlobalFcts() const -> VulkanGlobalFcts override;
    auto loadInstanceFcts(VkInstance vkInstance) const -> VulkanInstanceFcts override;
    auto loadDeviceFcts(VkDevice vkDevice) const -> VulkanDeviceFcts override;
    auto loadVmaFcts(VkInstance VkInstance, VkDevice vkDevice) const -> VmaVulkanFunctions override;

private:
    const VulkanLoaderConfig m_config;
    UniquePtrWithDeleter<void> m_pLibrary;
    PFN_vkGetInstanceProcAddr m_vkGetInstanceProcAddr;
    PFN_vkGetDeviceProcAddr m_vkGetDeviceProcAddr;
};

}  // namespace

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
    VulkanLibraryLoader libraryLoader;
    return createVulkanLoader(std::move(config), std::move(libraryLoader.pLibrary), libraryLoader.vkGetInstanceProcAddr,
                              libraryLoader.vkGetDeviceProcAddr);
}

auto im3e::createVulkanLoader(VulkanLoaderConfig config, UniquePtrWithDeleter<void> pLibrary,
                              PFN_vkGetInstanceProcAddr vkGetInstanceProcAddr,
                              PFN_vkGetDeviceProcAddr vkGetDeviceProcAddr) -> std::unique_ptr<IVulkanLoader>
{
    return std::make_unique<VulkanLoader>(std::move(config), std::move(pLibrary), vkGetInstanceProcAddr,
                                          vkGetDeviceProcAddr);
}