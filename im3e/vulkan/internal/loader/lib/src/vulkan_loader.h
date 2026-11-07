#pragma once

#include <im3e/utils/core/types.h>
#include <im3e/vulkan/internal/loader/vulkan_loaders.h>

namespace im3e {

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

}  // namespace im3e