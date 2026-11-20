#pragma once

#include "vulkan_debug_message_handler.h"
#include "vulkan_extensions.h"
#include "vulkan_loader.h"
#include "vulkan_physical_devices.h"

#include <im3e/utils/core/types.h>
#include <im3e/utils/loggers.h>

namespace im3e {

class IVulkanInstance
{
public:
    virtual ~IVulkanInstance() = default;

    virtual auto loadDeviceFcts(VkDevice vkDevice) const -> VulkanDeviceFcts = 0;
    virtual auto loadVmaFcts(VkDevice vkDevice) const -> VmaVulkanFunctions = 0;
    virtual auto choosePhysicalDevice(const IsPresentationSupportedFct& isPresentationSupportedFct) const
        -> VulkanPhysicalDevice = 0;

    static auto getVulkanApiVersion() -> uint32_t { return VK_API_VERSION_1_3; }

    virtual auto getFcts() const -> const VulkanInstanceFcts& = 0;
    virtual auto getExtensions() const -> const IVulkanExtensions& = 0;
    virtual auto getVkInstance() const -> VkInstance = 0;
};

[[nodiscard]] auto createVulkanInstance(const ILogger& logger, bool isDebugEnabled,
                                        const std::vector<const char*>& requiredInstanceExtensions,
                                        std::unique_ptr<IVulkanLoader> loader) -> std::unique_ptr<IVulkanInstance>;

}  // namespace im3e