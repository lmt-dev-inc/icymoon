#pragma once

#include <im3e/api/vulkan_functions.h>
#include <im3e/utils/loggers.h>

#include <vector>

namespace im3e {

class VulkanExtensions
{
public:
    VulkanExtensions(const ILogger& logger, const VulkanGlobalFcts& rFcts, bool isDebugEnabled,
                     const std::vector<const char*>& rRequiredInstanceExtensions = {});

    const auto& getInstanceExtensions() const { return m_instanceExtensions; }
    const auto& getDeviceExtensions() const { return m_deviceExtensions; }
    const auto& getLayers() const { return m_layers; }

    auto areDebugUtilsEnabled() const { return m_debugUtilsEnabled; }

private:
    const bool m_debugUtilsEnabled = false;
    std::vector<const char*> m_instanceExtensions;
    std::vector<const char*> m_deviceExtensions;
    std::vector<const char*> m_layers;
};

}  // namespace im3e