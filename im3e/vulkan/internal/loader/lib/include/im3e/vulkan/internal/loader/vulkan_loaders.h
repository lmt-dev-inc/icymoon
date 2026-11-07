#pragma once

#include <im3e/api/vulkan_functions.h>

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
auto createVulkanLoader(VulkanLoaderConfig config) -> std::unique_ptr<IVulkanLoader>;

}  // namespace im3e