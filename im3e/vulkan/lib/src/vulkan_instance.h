#pragma once

#include "vulkan_debug_message_handler.h"
#include "vulkan_extensions.h"
#include "vulkan_physical_devices.h"

#include <im3e/vulkan/internal/loader/vulkan_loaders.h>
#include <im3e/utils/core/types.h>
#include <im3e/utils/loggers.h>

namespace im3e {

class VulkanInstance
{
public:
    VulkanInstance(const ILogger& rLogger, bool isDebugEnabled,
                   const std::vector<const char*>& rRequiredInstanceExtensions, std::unique_ptr<IVulkanLoader> pLoader);

    auto loadDeviceFcts(VkDevice vkDevice) const -> VulkanDeviceFcts;
    auto loadVmaFcts(VkDevice vkDevice) const -> VmaVulkanFunctions;
    auto choosePhysicalDevice(const IsPresentationSupportedFct& rIsPresentationSupported) const -> VulkanPhysicalDevice;

    static auto getVulkanApiVersion() -> uint32_t { return VK_API_VERSION_1_3; }
    auto getFcts() const -> const VulkanInstanceFcts& { return m_fcts; }
    auto getExtensions() const -> const VulkanExtensions& { return m_extensions; }
    auto getVkInstance() const -> VkInstance { return m_pVkInstance.get(); }

private:
    std::unique_ptr<ILogger> m_pLogger;
    std::unique_ptr<IVulkanLoader> m_pLoader;
    VulkanGlobalFcts m_globalFcts;

    VulkanExtensions m_extensions;

    VulkanInstanceFcts m_fcts;
    VkUniquePtr<VkInstance> m_pVkInstance;
    std::unique_ptr<VulkanDebugMessageHandler> m_pDebugMessageHandler;
};

}  // namespace im3e