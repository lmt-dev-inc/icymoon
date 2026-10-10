#include "vulkan_physical_devices.h"

#include <im3e/utils/core/throw_utils.h>
#include <im3e/utils/vk_utils.h>

#include <fmt/format.h>

#include <algorithm>
#include <ranges>
#include <vector>

using namespace im3e;

namespace {

std::string toString(VkPhysicalDeviceType deviceType)
{
    switch (deviceType)
    {
        case VK_PHYSICAL_DEVICE_TYPE_OTHER: return "Other";
        case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU: return "Integrated GPU";
        case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU: return "Discrete GPU";
        case VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU: return "Virtual GPU";
        case VK_PHYSICAL_DEVICE_TYPE_CPU: return "CPU";
        default: break;
    }
    return "Unknow";
}

bool doesDeviceSupportExtensions(const ILogger& rLogger, const VulkanInstanceFcts& rFcts,
                                 const VulkanPhysicalDevice& rDevice, const VulkanExtensions& rExtensions)
{
    rLogger.info(
        fmt::format(R"(Checking support for device extensions of "{}")", rDevice.vkDeviceProperties.deviceName));

    auto supportedExtensions = getVkList<VkExtensionProperties>(
        rFcts.vkEnumerateDeviceExtensionProperties,
        fmt::format("device extensions for {}", rDevice.vkDeviceProperties.deviceName), rDevice.vkPhysicalDevice,
        nullptr);

    bool allSupported = true;
    for (const auto& rExtension : rExtensions.getDeviceExtensions())
    {
        auto itFind = std::ranges::find_if(supportedExtensions, [&](const auto& rSupportedExtension) {
            return strcmp(rExtension, &rSupportedExtension.extensionName[0]) == 0;
        });

        if (itFind != supportedExtensions.end())
        {
            rLogger.info(fmt::format("OK - {} v{}", itFind->extensionName, itFind->specVersion));
        }
        else
        {
            rLogger.info(fmt::format("FAILED - {}", rExtension));
            allSupported = false;
        }
    }
    return allSupported;
}

uint32_t getDeviceScore(const ILogger& logger, const VulkanInstanceFcts& rFcts, const VulkanPhysicalDevice& rDevice,
                        const VulkanExtensions& rExtensions, bool presentationRequired)
{
    if (!doesDeviceSupportExtensions(logger, rFcts, rDevice, rExtensions) ||  // Extensions always required
        rDevice.queueFamilies.graphicsFamilyIndices.empty() ||                // Graphics support always required
        rDevice.queueFamilies.transferFamilyIndices.empty())                  // Transfer support always required
    {
        return 0U;
    }

    // Presentation support may be required:
    if (presentationRequired && rDevice.queueFamilies.presentationFamilyIndices.empty())
    {
        return 0U;
    }

    constexpr uint32_t DiscreteGpuScore = 1'000U;
    constexpr uint32_t IntegratedGpuScore = 500U;
    constexpr uint32_t OtherGpuTypeScore = 100U;
    uint32_t score{};

    // Discrete GPUs have a significant performance advantage:
    if (rDevice.vkDeviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
    {
        score += DiscreteGpuScore;
    }
    else if (rDevice.vkDeviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU)
    {
        score += IntegratedGpuScore;
    }
    else
    {
        score += OtherGpuTypeScore;
    }

    return score;
}

auto getQueueFamilyProperties(const VulkanInstanceFcts& rFcts, VkInstance vkInstance, VkPhysicalDevice vkPhysicalDevice,
                              const IsPresentationSupportedFct& rIsPresentationSupported)
{
    VulkanPhysicalDevice::QueueFamilies queueFamilies{};

    uint32_t queueFamilyCount{};
    rFcts.vkGetPhysicalDeviceQueueFamilyProperties(vkPhysicalDevice, &queueFamilyCount, nullptr);

    queueFamilies.vkQueueFamilyProperties.resize(queueFamilyCount);
    rFcts.vkGetPhysicalDeviceQueueFamilyProperties(vkPhysicalDevice, &queueFamilyCount,
                                                   queueFamilies.vkQueueFamilyProperties.data());

    for (uint32_t i = 0U; i < queueFamilies.vkQueueFamilyProperties.size(); i++)
    {
        const auto& rProperties = queueFamilies.vkQueueFamilyProperties[i];
        if (vkFlagsContain(rProperties.queueFlags, VK_QUEUE_COMPUTE_BIT))
        {
            queueFamilies.computeFamilyIndices.push_back(i);
        }
        if (vkFlagsContain(rProperties.queueFlags, VK_QUEUE_GRAPHICS_BIT))
        {
            queueFamilies.graphicsFamilyIndices.push_back(i);
        }
        if (vkFlagsContain(rProperties.queueFlags, VK_QUEUE_TRANSFER_BIT))
        {
            queueFamilies.transferFamilyIndices.push_back(i);
        }
        if (!!rIsPresentationSupported && rIsPresentationSupported(vkInstance, vkPhysicalDevice, i))
        {
            queueFamilies.presentationFamilyIndices.push_back(i);
        }
    }

    return queueFamilies;
}

auto enumerateDevices(const ILogger& rLogger, const VulkanInstanceFcts& rFcts, VkInstance vkInstance)
{
    const auto devices = getVkList<VkPhysicalDevice>(rFcts.vkEnumeratePhysicalDevices, "physical devices", vkInstance);
    throwIfFalse<std::runtime_error>(!devices.empty(), "Could not detect any physical device");
    rLogger.info(fmt::format("Detected {} device(s)", devices.size()));
    return devices;
}

auto enumerateAndRankDevices(const ILogger& rLogger, const VulkanInstanceFcts& rFcts,
                             const VulkanExtensions& rExtensions, VkInstance vkInstance,
                             const IsPresentationSupportedFct& rIsPresentationSupported)
{
    const auto vkPhysicalDevices = enumerateDevices(rLogger, rFcts, vkInstance);

    std::multimap<uint32_t, VulkanPhysicalDevice> scoresToDevices;
    for (const auto vkPhysicalDevice : vkPhysicalDevices)
    {
        VulkanPhysicalDevice device{
            .vkPhysicalDevice = vkPhysicalDevice,
        };
        rFcts.vkGetPhysicalDeviceProperties(vkPhysicalDevice, &device.vkDeviceProperties);
        rFcts.vkGetPhysicalDeviceMemoryProperties(vkPhysicalDevice, &device.vkDeviceMemoryProperties);
        rFcts.vkGetPhysicalDeviceFeatures(vkPhysicalDevice, &device.vkDeviceFeatures);
        device.queueFamilies = getQueueFamilyProperties(rFcts, vkInstance, vkPhysicalDevice, rIsPresentationSupported);

        if (const auto deviceScore = getDeviceScore(rLogger, rFcts, device, rExtensions, !!rIsPresentationSupported))
        {
            scoresToDevices.insert(std::make_pair(deviceScore, std::move(device)));
        }
    }
    throwIfFalse<std::runtime_error>(!scoresToDevices.empty(), "Could not find a suitable device");
    return scoresToDevices;
}

void logDeviceRanks(const ILogger& rLogger, const std::multimap<uint32_t, VulkanPhysicalDevice>& rScoresToDevices)
{
    for (const auto& scoreToDevice : rScoresToDevices)
    {
        const auto& device = scoreToDevice.second;
        rLogger.info(fmt::format("Score {} - Device #{} - {} - {}", scoreToDevice.first,
                                 device.vkDeviceProperties.deviceID, device.vkDeviceProperties.deviceName,
                                 toString(device.vkDeviceProperties.deviceType)));
    }
}

}  // namespace

VulkanPhysicalDevices::VulkanPhysicalDevices(const ILogger& logger, const VulkanInstanceFcts& rFcts,
                                             const VulkanExtensions& rExtensions, VkInstance vkInstance,
                                             const IsPresentationSupportedFct& rIsPresentationSupported)
  : m_pLogger(logger.createChild("Physical Devices"))
  , m_vkInstance(throwIfArgNull(vkInstance, "Physical Devices require an instance"))
  , m_scoresToDevices(enumerateAndRankDevices(*m_pLogger, rFcts, rExtensions, m_vkInstance, rIsPresentationSupported))
{
    logDeviceRanks(*m_pLogger, m_scoresToDevices);
}

auto VulkanPhysicalDevices::choosePhysicalDevice() const -> const VulkanPhysicalDevice&
{
    const auto& rDevice = m_scoresToDevices.rbegin()->second;
    const auto& deviceName = rDevice.vkDeviceProperties.deviceName;
    m_pLogger->info(fmt::format(R"(Choosing device: "{}")", deviceName));
    return rDevice;
}
