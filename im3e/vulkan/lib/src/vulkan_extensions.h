#pragma once

#include <im3e/utils/loggers.h>
#include <im3e/vulkan/vulkan_functions.h>

#include <memory>
#include <vector>

namespace im3e {

class IVulkanExtensions
{
public:
    virtual ~IVulkanExtensions() = default;

    virtual auto getInstanceExtensions() const -> const std::vector<const char*>& = 0;
    virtual auto getDeviceExtensions() const -> const std::vector<const char*>& = 0;
    virtual auto getLayers() const -> const std::vector<const char*>& = 0;

    virtual auto areDebugUtilsEnabled() const -> bool = 0;
};

auto createVulkanExtensions(const ILogger& logger, const VulkanGlobalFcts& fcts, bool isDebugEnabled,
                            const std::vector<const char*>& requiredInstanceExtensions = {})
    -> std::unique_ptr<IVulkanExtensions>;

}  // namespace im3e