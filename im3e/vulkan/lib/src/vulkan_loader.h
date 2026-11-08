#pragma once

#include <im3e/api/vulkan_functions.h>
#include <im3e/utils/core/types.h>

#include <memory>

namespace im3e {

class IVulkanLoader
{
public:
    virtual ~IVulkanLoader() = default;

    virtual auto loadGlobalFcts() const -> VulkanGlobalFcts = 0;
    virtual auto loadInstanceFcts(VkInstance vkInstance) const -> VulkanInstanceFcts = 0;
    virtual auto loadDeviceFcts(VkDevice vkDevice) const -> VulkanDeviceFcts = 0;
    virtual auto loadVmaFcts(VkInstance vkInstance, VkDevice vkDevice) const -> VmaVulkanFunctions = 0;
};

struct VulkanLoaderConfig
{
    bool isDebugEnabled = false;
};

/// @brief Creates a loader that loads the Vulkan library of the system (vulkan-1.dll, libvulkan.so.1).
auto createVulkanLoader(VulkanLoaderConfig config) -> std::unique_ptr<IVulkanLoader>;

/// @brief Creates a loader that loads Vulkan functions through the given entry points, e.g. fakes in unit tests.
/// @param pLibrary Library that the entry points come from, kept loaded as long as the loader exists.
auto createVulkanLoader(VulkanLoaderConfig config, UniquePtrWithDeleter<void> pLibrary,
                        PFN_vkGetInstanceProcAddr vkGetInstanceProcAddr, PFN_vkGetDeviceProcAddr vkGetDeviceProcAddr)
    -> std::unique_ptr<IVulkanLoader>;

}  // namespace im3e
