#include "vulkan_device.h"

#include "vulkan_images.h"

#include <im3e/utils/core/throw_utils.h>
#include <im3e/utils/stats.h>
#include <im3e/vulkan/utils.h>

#include <algorithm>
#include <cstdint>
#include <iterator>
#include <limits>
#include <memory>
#include <set>
#include <vector>

using namespace im3e;

namespace {

constexpr float MaxQueuePriority = 1.0F;

auto makeQueueCreateInfos(const VulkanPhysicalDevice& physicalDevice) -> std::vector<VkDeviceQueueCreateInfo>
{
    std::set<uint32_t> queueFamilyIndices;
    auto insertToQueueFamilyIndices = [&](const std::vector<uint32_t>& indices) {
        std::ranges::transform(indices, std::inserter(queueFamilyIndices, queueFamilyIndices.end()),
                               [](auto i) { return i; });
    };
    insertToQueueFamilyIndices(physicalDevice.queueFamilies.computeFamilyIndices);
    insertToQueueFamilyIndices(physicalDevice.queueFamilies.graphicsFamilyIndices);
    insertToQueueFamilyIndices(physicalDevice.queueFamilies.transferFamilyIndices);
    insertToQueueFamilyIndices(physicalDevice.queueFamilies.presentationFamilyIndices);

    std::vector<VkDeviceQueueCreateInfo> vkQueueCreateInfos;
    vkQueueCreateInfos.reserve(queueFamilyIndices.size());
    for (const auto& queueFamilyIndex : queueFamilyIndices)
    {
        vkQueueCreateInfos.emplace_back(VkDeviceQueueCreateInfo{
            .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
            .queueFamilyIndex = queueFamilyIndex,
            .queueCount = 1U,
            .pQueuePriorities = &MaxQueuePriority,
        });
    }
    return vkQueueCreateInfos;
}

auto makeVk11Features() -> VkPhysicalDeviceVulkan11Features
{
    return VkPhysicalDeviceVulkan11Features{
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES,
        .storageBuffer16BitAccess = VK_TRUE,
    };
}

auto makeVk12Features() -> VkPhysicalDeviceVulkan12Features
{
    return VkPhysicalDeviceVulkan12Features{
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES,
        .shaderFloat16 = VK_TRUE,
        .shaderInt8 = VK_TRUE,
        .descriptorBindingPartiallyBound = VK_TRUE,
        .descriptorBindingVariableDescriptorCount = VK_TRUE,
        .runtimeDescriptorArray = VK_TRUE,
        .scalarBlockLayout = VK_TRUE,
        .bufferDeviceAddress = VK_TRUE,
    };
}

auto makeVk13Features() -> VkPhysicalDeviceVulkan13Features
{
    return VkPhysicalDeviceVulkan13Features{
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES,
        .synchronization2 = VK_TRUE,
        .dynamicRendering = VK_TRUE,
        .maintenance4 = VK_TRUE,
    };
}

auto createVkDevice(const VulkanInstanceFcts& instanceFcts, const IVulkanExtensions& extensions,
                    const VulkanPhysicalDevice& physicalDevice) -> VkDevice
{
    const auto vkQueueCreateInfos = makeQueueCreateInfos(physicalDevice);
    const auto& deviceExtensions = extensions.getDeviceExtensions();

    VkPhysicalDeviceFeatures vkFeatures{
        .samplerAnisotropy = VK_TRUE,
        .textureCompressionETC2 = physicalDevice.vkDeviceFeatures.textureCompressionETC2,
        .textureCompressionASTC_LDR = physicalDevice.vkDeviceFeatures.textureCompressionASTC_LDR,
        .textureCompressionBC = physicalDevice.vkDeviceFeatures.textureCompressionBC,
        .shaderSampledImageArrayDynamicIndexing = VK_TRUE,
        .shaderInt64 = VK_TRUE,
        .shaderInt16 = VK_TRUE,
    };

    VkDeviceCreateInfo vkCreateInfo{
        .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
        .queueCreateInfoCount = static_cast<uint32_t>(vkQueueCreateInfos.size()),
        .pQueueCreateInfos = vkQueueCreateInfos.data(),
        .enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size()),
        .ppEnabledExtensionNames = deviceExtensions.data(),
        .pEnabledFeatures = &vkFeatures,
    };

    auto vk11Features = makeVk11Features();
    vkCreateInfo.pNext = &vk11Features;

    auto vk12Features = makeVk12Features();
    vk11Features.pNext = &vk12Features;

    auto vk13Features = makeVk13Features();
    vk12Features.pNext = &vk13Features;

    VkDevice vkDevice{};
    throwIfVkFailed(instanceFcts.vkCreateDevice(physicalDevice.vkPhysicalDevice, &vkCreateInfo, nullptr, &vkDevice),
                    "Could not create Vulkan logical device");
    return vkDevice;
}

auto createDeviceAndLoadFcts(const IVulkanInstance& instance, const VulkanPhysicalDevice& physicalDevice,
                             VulkanDeviceFcts& fcts) -> VkUniquePtr<VkDevice>
{
    const auto vkDevice = createVkDevice(instance.getFcts(), instance.getExtensions(), physicalDevice);
    fcts = instance.loadDeviceFcts(vkDevice);
    return VkUniquePtr<VkDevice>(vkDevice, [&fcts](VkDevice vkDev) { fcts.vkDestroyDevice(vkDev, nullptr); });
}

auto findQueue(const VulkanDeviceFcts& fcts, VkDevice vkDevice, const std::vector<uint32_t>& familyIndices)
    -> VulkanCommandQueueInfo
{
    for (auto familyIndex : familyIndices)
    {
        VkQueue vkQueue{};
        fcts.vkGetDeviceQueue(vkDevice, familyIndex, 0U, &vkQueue);
        return VulkanCommandQueueInfo{
            .vkQueue = vkQueue,
            .queueFamilyIndex = familyIndex,
        };
    }
    return VulkanCommandQueueInfo{};
}

auto findCommandQueueInfo(const VulkanDeviceFcts& fcts, VkDevice vkDevice,
                          const VulkanPhysicalDevice::QueueFamilies& queueFamilies) -> VulkanCommandQueueInfo
{
    if (auto queueInfo = findQueue(fcts, vkDevice, queueFamilies.presentationFamilyIndices); queueInfo.vkQueue)
    {
        return queueInfo;
    }
    if (auto queueInfo = findQueue(fcts, vkDevice, queueFamilies.graphicsFamilyIndices); queueInfo.vkQueue)
    {
        return queueInfo;
    }
    throw std::runtime_error("Could not find Vulkan command queue with either presentation or graphics capabilities");
}

}  // namespace

VulkanDevice::VulkanDevice(const ILogger& logger, DeviceConfig config, std::shared_ptr<IStatsProvider> stats,
                           std::unique_ptr<IVulkanInstance> instance, MemoryAllocatorFactory memoryAllocatorFactory)
  : m_logger(logger.createChild("VulkanDevice"))
  , m_statsProvider(throwIfArgNull(std::move(stats), "Vulkan device requires a stats provider"))
  , m_config(std::move(config))

  , m_instance(throwIfArgNull(std::move(instance), "Vulkan device requires an instance"))
  , m_physicalDevice(m_instance->choosePhysicalDevice(m_config.isPresentationSupported))
  , m_vkDevice(createDeviceAndLoadFcts(*m_instance, m_physicalDevice, m_fcts))
  , m_commandQueueInfo(findCommandQueueInfo(m_fcts, m_vkDevice.get(), m_physicalDevice.queueFamilies))
  , m_memoryAllocator([&] {
      throwIfArgNull(memoryAllocatorFactory, "Vulkan device requires a memory allocator factory");
      return memoryAllocatorFactory(*this, m_instance->loadVmaFcts(m_vkDevice.get()));
  }())
  , m_commandQueue(createVulkanCommandQueue(*this, m_commandQueueInfo, "MainQueue"))
{
    m_logger->info("Successfully initialized");
}

VulkanDevice::~VulkanDevice()
{
    m_logger->debug("Waiting for device to be idle");
    m_fcts.vkDeviceWaitIdle(m_vkDevice.get());
    m_logger->debug("Device idle, destroying");
}

auto VulkanDevice::createVkSemaphore() const -> VkUniquePtr<VkSemaphore>
{
    VkSemaphoreCreateInfo vkCreateInfo{.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO};

    VkSemaphore vkSemaphore{};
    throwIfVkFailed(m_fcts.vkCreateSemaphore(m_vkDevice.get(), &vkCreateInfo, nullptr, &vkSemaphore),
                    "Failed to create semaphore for Vulkan Device");

    return makeVkUniquePtr<VkSemaphore>(m_vkDevice.get(), vkSemaphore, m_fcts.vkDestroySemaphore);
}

auto VulkanDevice::createVkFence(VkFenceCreateFlags vkFlags) const -> VkUniquePtr<VkFence>
{
    VkFenceCreateInfo vkCreateInfo{.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO, .flags = vkFlags};

    VkFence vkFence{};
    throwIfVkFailed(m_fcts.vkCreateFence(m_vkDevice.get(), &vkCreateInfo, nullptr, &vkFence),
                    "Could not create fence for Vulkan device");

    return makeVkUniquePtr<VkFence>(m_vkDevice.get(), vkFence, m_fcts.vkDestroyFence);
}

void VulkanDevice::waitForVkFence(VkFence vkFence) const
{
    if (!vkFence)
    {
        return;
    }
    throwIfVkFailed(
        m_fcts.vkWaitForFences(m_vkDevice.get(), 1U, &vkFence, VK_TRUE, std::numeric_limits<uint64_t>::max()),
        "Failed to wait for fence from Vulkan Device");
}

auto VulkanDevice::createLogger(std::string_view name) const -> std::unique_ptr<ILogger>
{
    return m_logger->createChild(name);
}

auto VulkanDevice::getImageFactory() const -> std::shared_ptr<const IImageFactory>
{
    if (!m_imageFactory)
    {
        auto self = shared_from_this();
        m_imageFactory = createVulkanImageFactory(self, m_memoryAllocator);
    }
    return m_imageFactory;
}

auto im3e::createDevice(const ILogger& logger, DeviceConfig config) -> std::shared_ptr<IDevice>
{
    auto loader = createVulkanLoader(VulkanLoaderConfig{
        .isDebugEnabled = config.isDebugEnabled,
    });
    auto instance = createVulkanInstance(logger, config.isDebugEnabled, config.requiredInstanceExtensions,
                                         std::move(loader));
    return std::make_shared<VulkanDevice>(logger, std::move(config), createStatsProvider(), std::move(instance),
                                          &createVulkanMemoryAllocator);
}
