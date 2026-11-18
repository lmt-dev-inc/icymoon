#include "vulkan_physical_devices.h"

#include "vulkan_instance.h"

#include <im3e/utils/core/throw_utils.h>
#include <im3e/vulkan/utils.h>

#include <fmt/format.h>

#include <algorithm>
#include <ranges>
#include <vector>

using namespace im3e;

namespace {

auto toString(VkPhysicalDeviceType deviceType) -> std::string
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

auto doesDeviceSupportExtensions(const ILogger& logger, const VulkanInstanceFcts& fcts,
                                 const VulkanPhysicalDevice& device, const IVulkanExtensions& extensions) -> bool
{
    logger.info(fmt::format(R"(Checking support for device extensions of "{}")", device.vkDeviceProperties.deviceName));

    auto supportedExtensions = getVkList<VkExtensionProperties>(
        fcts.vkEnumerateDeviceExtensionProperties,
        fmt::format("device extensions for {}", device.vkDeviceProperties.deviceName), device.vkPhysicalDevice,
        nullptr);

    bool allSupported = true;
    for (const auto& extension : extensions.getDeviceExtensions())
    {
        auto itFind = std::ranges::find_if(supportedExtensions, [&](const auto& supportedExtension) {
            return strcmp(extension, &supportedExtension.extensionName[0]) == 0;
        });

        if (itFind != supportedExtensions.end())
        {
            logger.info(fmt::format("OK - {} v{}", itFind->extensionName, itFind->specVersion));
        }
        else
        {
            logger.info(fmt::format("FAILED - {}", extension));
            allSupported = false;
        }
    }
    return allSupported;
}

auto getDeviceScore(const ILogger& logger, const VulkanInstanceFcts& fcts, const VulkanPhysicalDevice& device,
                    const IVulkanExtensions& extensions, bool presentationRequired) -> uint32_t
{
    const auto supportedApiVersion = device.vkDeviceProperties.apiVersion;
    const auto requiredApiVersion = VulkanInstance::getVulkanApiVersion();
    if (supportedApiVersion < requiredApiVersion)
    {
        logger.info(fmt::format(R"(Device "{}" only supports Vulkan {}.{} ({}.{} is required))",
                                device.vkDeviceProperties.deviceName, VK_API_VERSION_MAJOR(supportedApiVersion),
                                VK_API_VERSION_MINOR(supportedApiVersion), VK_API_VERSION_MAJOR(requiredApiVersion),
                                VK_API_VERSION_MINOR(requiredApiVersion)));
        return 0U;
    }

    if (!doesDeviceSupportExtensions(logger, fcts, device, extensions) ||  // Extensions always required
        device.queueFamilies.graphicsFamilyIndices.empty() ||              // Graphics support always required
        device.queueFamilies.transferFamilyIndices.empty())                // Transfer support always required
    {
        return 0U;
    }

    // Presentation support may be required:
    if (presentationRequired && device.queueFamilies.presentationFamilyIndices.empty())
    {
        logger.info(fmt::format(R"(Device "{}" does not support presentation)", device.vkDeviceProperties.deviceName));
        return 0U;
    }

    constexpr uint32_t DiscreteGpuScore = 1'000U;
    constexpr uint32_t IntegratedGpuScore = 500U;
    constexpr uint32_t OtherGpuTypeScore = 100U;
    uint32_t score{};

    // Discrete GPUs have a significant performance advantage:
    if (device.vkDeviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
    {
        score += DiscreteGpuScore;
    }
    else if (device.vkDeviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU)
    {
        score += IntegratedGpuScore;
    }
    else
    {
        score += OtherGpuTypeScore;
    }

    return score;
}

auto getQueueFamilyProperties(const VulkanInstanceFcts& fcts, VkInstance vkInstance, VkPhysicalDevice vkPhysicalDevice,
                              const IsPresentationSupportedFct& isPresentationSupported)
{
    VulkanPhysicalDevice::QueueFamilies queueFamilies{};

    uint32_t queueFamilyCount{};
    fcts.vkGetPhysicalDeviceQueueFamilyProperties(vkPhysicalDevice, &queueFamilyCount, nullptr);

    queueFamilies.vkQueueFamilyProperties.resize(queueFamilyCount);
    fcts.vkGetPhysicalDeviceQueueFamilyProperties(vkPhysicalDevice, &queueFamilyCount,
                                                  queueFamilies.vkQueueFamilyProperties.data());

    for (uint32_t i = 0U; i < queueFamilies.vkQueueFamilyProperties.size(); i++)
    {
        const auto& properties = queueFamilies.vkQueueFamilyProperties[i];
        if (vkFlagsContain(properties.queueFlags, VK_QUEUE_COMPUTE_BIT))
        {
            queueFamilies.computeFamilyIndices.push_back(i);
        }
        if (vkFlagsContain(properties.queueFlags, VK_QUEUE_GRAPHICS_BIT))
        {
            queueFamilies.graphicsFamilyIndices.push_back(i);
        }
        if (vkFlagsContain(properties.queueFlags, VK_QUEUE_TRANSFER_BIT))
        {
            queueFamilies.transferFamilyIndices.push_back(i);
        }
        if (!!isPresentationSupported && isPresentationSupported(vkInstance, vkPhysicalDevice, i))
        {
            queueFamilies.presentationFamilyIndices.push_back(i);
        }
    }

    return queueFamilies;
}

auto enumerateDevices(const ILogger& logger, const VulkanInstanceFcts& fcts, VkInstance vkInstance)
{
    const auto devices = getVkList<VkPhysicalDevice>(fcts.vkEnumeratePhysicalDevices, "physical devices", vkInstance);
    throwIfFalse<std::runtime_error>(!devices.empty(), "Could not detect any physical device");
    logger.info(fmt::format("Detected {} device(s)", devices.size()));
    return devices;
}

auto enumerateAndRankDevices(const ILogger& logger, const VulkanInstanceFcts& fcts, const IVulkanExtensions& extensions,
                             VkInstance vkInstance, const IsPresentationSupportedFct& isPresentationSupported)
{
    const auto vkPhysicalDevices = enumerateDevices(logger, fcts, vkInstance);

    std::multimap<uint32_t, VulkanPhysicalDevice> scoresToDevices;
    for (const auto vkPhysicalDevice : vkPhysicalDevices)
    {
        VulkanPhysicalDevice device{
            .vkPhysicalDevice = vkPhysicalDevice,
        };
        fcts.vkGetPhysicalDeviceProperties(vkPhysicalDevice, &device.vkDeviceProperties);
        fcts.vkGetPhysicalDeviceMemoryProperties(vkPhysicalDevice, &device.vkDeviceMemoryProperties);
        fcts.vkGetPhysicalDeviceFeatures(vkPhysicalDevice, &device.vkDeviceFeatures);
        device.queueFamilies = getQueueFamilyProperties(fcts, vkInstance, vkPhysicalDevice, isPresentationSupported);

        if (const auto deviceScore = getDeviceScore(logger, fcts, device, extensions, !!isPresentationSupported))
        {
            scoresToDevices.insert(std::make_pair(deviceScore, std::move(device)));
        }
    }
    throwIfFalse<std::runtime_error>(!scoresToDevices.empty(), "Could not find a suitable device");
    return scoresToDevices;
}

void logDeviceRanks(const ILogger& logger, const std::multimap<uint32_t, VulkanPhysicalDevice>& scoresToDevices)
{
    for (const auto& scoreToDevice : scoresToDevices)
    {
        const auto& device = scoreToDevice.second;
        logger.info(fmt::format("Score {} - Device #{} - {} - {}", scoreToDevice.first,
                                device.vkDeviceProperties.deviceID, device.vkDeviceProperties.deviceName,
                                toString(device.vkDeviceProperties.deviceType)));
    }
}

}  // namespace

VulkanPhysicalDevices::VulkanPhysicalDevices(const ILogger& logger, const VulkanInstanceFcts& fcts,
                                             const IVulkanExtensions& extensions, VkInstance vkInstance,
                                             const IsPresentationSupportedFct& isPresentationSupported)
  : m_logger(logger.createChild("Physical Devices"))
  , m_vkInstance(throwIfArgNull(vkInstance, "Physical Devices require an instance"))
  , m_scoresToDevices(enumerateAndRankDevices(*m_logger, fcts, extensions, m_vkInstance, isPresentationSupported))
{
    logDeviceRanks(*m_logger, m_scoresToDevices);
}

auto VulkanPhysicalDevices::choosePhysicalDevice() const -> const VulkanPhysicalDevice&
{
    const auto& device = m_scoresToDevices.rbegin()->second;
    const auto& deviceName = device.vkDeviceProperties.deviceName;
    m_logger->info(fmt::format(R"(Choosing device: "{}")", deviceName));
    return device;
}
