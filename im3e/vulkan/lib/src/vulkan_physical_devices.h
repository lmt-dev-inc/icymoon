#pragma once

#include "vulkan_extensions.h"

#include <im3e/api/vulkan_functions.h>
#include <im3e/utils/loggers.h>
#include <im3e/vulkan/device.h>

#include <functional>
#include <map>
#include <memory>

namespace im3e {

struct VulkanPhysicalDevice
{
    VkPhysicalDevice vkPhysicalDevice{};
    VkPhysicalDeviceProperties vkDeviceProperties{};
    VkPhysicalDeviceMemoryProperties vkDeviceMemoryProperties{};
    VkPhysicalDeviceFeatures vkDeviceFeatures{};

    struct QueueFamilies
    {
        std::vector<VkQueueFamilyProperties> vkQueueFamilyProperties;
        std::vector<uint32_t> computeFamilyIndices;
        std::vector<uint32_t> graphicsFamilyIndices;
        std::vector<uint32_t> transferFamilyIndices;
        std::vector<uint32_t> presentationFamilyIndices;
    } queueFamilies;
};

class VulkanPhysicalDevices
{
public:
    VulkanPhysicalDevices(const ILogger& rLogger, const VulkanInstanceFcts& rFcts, const VulkanExtensions& rExtensions,
                          VkInstance vkInstance, const IsPresentationSupportedFct& rIsPresentationSupported = {});

    auto choosePhysicalDevice() const -> const VulkanPhysicalDevice&;

private:
    std::unique_ptr<ILogger> m_pLogger;
    VkInstance m_vkInstance;

    std::vector<VkPhysicalDevice> m_physicalDevices;
    std::multimap<uint32_t, VulkanPhysicalDevice> m_scoresToDevices;
};

}  // namespace im3e