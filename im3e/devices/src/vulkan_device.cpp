#include "vulkan_device.h"

#include "vulkan_images.h"

#include <im3e/utils/core/throw_utils.h>
#include <im3e/utils/core/types.h>
#include <im3e/utils/vk_utils.h>

#include <algorithm>
#include <memory>
#include <set>
#include <vector>

using namespace im3e;

namespace {

constexpr float MaxQueuePriority = 1.0F;

auto makeQueueCreateInfos(const VulkanPhysicalDevice& rPhysicalDevice)
{
    std::set<uint32_t> queueFamilyIndices;
    auto insertToQueueFamilyIndices = [&](const std::vector<uint32_t>& rIndices) {
        std::ranges::transform(rIndices, std::inserter(queueFamilyIndices, queueFamilyIndices.end()),
                               [](auto i) { return i; });
    };
    insertToQueueFamilyIndices(rPhysicalDevice.queueFamilies.computeFamilyIndices);
    insertToQueueFamilyIndices(rPhysicalDevice.queueFamilies.graphicsFamilyIndices);
    insertToQueueFamilyIndices(rPhysicalDevice.queueFamilies.transferFamilyIndices);
    insertToQueueFamilyIndices(rPhysicalDevice.queueFamilies.presentationFamilyIndices);

    std::vector<VkDeviceQueueCreateInfo> vkQueueCreateInfos;
    vkQueueCreateInfos.reserve(queueFamilyIndices.size());
    for (const auto& queueFamilyIndex : queueFamilyIndices)
    {
        VkDeviceQueueCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        createInfo.queueFamilyIndex = queueFamilyIndex;
        createInfo.queueCount = 1U;
        createInfo.pQueuePriorities = &MaxQueuePriority;
        vkQueueCreateInfos.emplace_back(createInfo);
    }
    return vkQueueCreateInfos;
}

auto makeVk11Features()
{
    return VkPhysicalDeviceVulkan11Features{
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES,
        .storageBuffer16BitAccess = VK_TRUE,
    };
}

auto makeVk12Features()
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

auto makeVk13Features()
{
    return VkPhysicalDeviceVulkan13Features{
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES,
        .synchronization2 = VK_TRUE,
        .maintenance4 = VK_TRUE,
    };
}

auto makeRayTracingPipelineFeatures()
{
    return VkPhysicalDeviceRayTracingPipelineFeaturesKHR{
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_FEATURES_KHR,  //
        .rayTracingPipeline = VK_TRUE,
    };
}

auto makeAccelerationStructureFeatures()
{
    return VkPhysicalDeviceAccelerationStructureFeaturesKHR{
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_FEATURES_KHR,
        .accelerationStructure = VK_TRUE,
    };
}

auto makeRayQueryFeatures()
{
    return VkPhysicalDeviceRayQueryFeaturesKHR{
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_QUERY_FEATURES_KHR,
        .rayQuery = VK_TRUE,
    };
}

auto createVkDevice(const VulkanInstanceFcts& rInstFcts, const VulkanExtensions& rExtensions,
                    const VulkanPhysicalDevice& rPhysicalDevice)
{
    const auto vkQueueCreateInfos = makeQueueCreateInfos(rPhysicalDevice);
    const auto& rLayers = rExtensions.getLayers();
    const auto& rDeviceExtensions = rExtensions.getDeviceExtensions();

    VkPhysicalDeviceFeatures features{
        .samplerAnisotropy = VK_TRUE,
        .textureCompressionETC2 = rPhysicalDevice.vkDeviceFeatures.textureCompressionETC2,
        .textureCompressionASTC_LDR = rPhysicalDevice.vkDeviceFeatures.textureCompressionASTC_LDR,
        .textureCompressionBC = rPhysicalDevice.vkDeviceFeatures.textureCompressionBC,
        .shaderSampledImageArrayDynamicIndexing = VK_TRUE,
        .shaderInt64 = VK_TRUE,
        .shaderInt16 = VK_TRUE,
    };

    VkDeviceCreateInfo vkCreateInfo{
        .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
        .queueCreateInfoCount = static_cast<uint32_t>(vkQueueCreateInfos.size()),
        .pQueueCreateInfos = vkQueueCreateInfos.data(),
        .enabledLayerCount = static_cast<uint32_t>(rLayers.size()),
        .ppEnabledLayerNames = rLayers.data(),
        .enabledExtensionCount = static_cast<uint32_t>(rDeviceExtensions.size()),
        .ppEnabledExtensionNames = rDeviceExtensions.data(),
        .pEnabledFeatures = &features,
    };

    auto vk11Features = makeVk11Features();
    vkCreateInfo.pNext = &vk11Features;

    auto vk12Features = makeVk12Features();
    vk11Features.pNext = &vk12Features;

    auto vk13Features = makeVk13Features();
    vk12Features.pNext = &vk13Features;

    auto vkRayTracingPipelineFeatures = makeRayTracingPipelineFeatures();
    vk13Features.pNext = &vkRayTracingPipelineFeatures;

    auto vkAccelerationStructureFeatures = makeAccelerationStructureFeatures();
    vkRayTracingPipelineFeatures.pNext = &vkAccelerationStructureFeatures;

    auto vkRayQueryFeatures = makeRayQueryFeatures();
    vkAccelerationStructureFeatures.pNext = &vkRayQueryFeatures;

    VkDevice vkDevice{};
    throwIfVkFailed(rInstFcts.vkCreateDevice(rPhysicalDevice.vkPhysicalDevice, &vkCreateInfo, nullptr, &vkDevice),
                    "Could not create Vulkan logical device");
    return vkDevice;
}

auto createDeviceAndLoadFcts(const VulkanInstance& rInstance, const VulkanPhysicalDevice& rPhysicalDevice,
                             VulkanDeviceFcts& rFcts)
{
    const auto vkDevice = createVkDevice(rInstance.getFcts(), rInstance.getExtensions(), rPhysicalDevice);
    rFcts = rInstance.loadDeviceFcts(vkDevice);
    return VkUniquePtr<VkDevice>(vkDevice, [&rFcts](VkDevice vkDevice) { rFcts.vkDestroyDevice(vkDevice, nullptr); });
}

auto findQueue(const VulkanDeviceFcts& rFcts, VkDevice vkDevice, const std::vector<uint32_t>& rFamilyIndices)
    -> VulkanCommandQueueInfo
{
    for (auto familyIndex : rFamilyIndices)
    {
        VkQueue vkQueue{};
        rFcts.vkGetDeviceQueue(vkDevice, familyIndex, 0U, &vkQueue);
        return VulkanCommandQueueInfo{
            .vkQueue = vkQueue,
            .queueFamilyIndex = familyIndex,
        };
    }
    return VulkanCommandQueueInfo{};
}

auto findCommandQueueInfo(const VulkanDeviceFcts& rFcts, VkDevice vkDevice,
                          const VulkanPhysicalDevice::QueueFamilies& rQueueFamilies)
{
    if (auto queueInfo = findQueue(rFcts, vkDevice, rQueueFamilies.presentationFamilyIndices); queueInfo.vkQueue)
    {
        return queueInfo;
    }
    if (auto queueInfo = findQueue(rFcts, vkDevice, rQueueFamilies.graphicsFamilyIndices); queueInfo.vkQueue)
    {
        return queueInfo;
    }
    throw std::runtime_error("Could not find Vulkan command queue with either presentation or graphics capabilities");
}

}  // namespace

VulkanDevice::VulkanDevice(const ILogger& rLogger, DeviceConfig config)
  : m_pLogger(rLogger.createChild("VulkanDevice"))
  , m_pStatsProvider(createStatsProvider())
  , m_config(std::move(config))

  , m_instance(*m_pLogger, m_config.isDebugEnabled, m_config.requiredInstanceExtensions,
               createVulkanLoader(VulkanLoaderConfig{
                   .isDebugEnabled = config.isDebugEnabled,
               }))
  , m_physicalDevice(m_instance.choosePhysicalDevice(config.isPresentationSupported))
  , m_pVkDevice(createDeviceAndLoadFcts(m_instance, m_physicalDevice, m_fcts))
  , m_commandQueueInfo(findCommandQueueInfo(m_fcts, m_pVkDevice.get(), m_physicalDevice.queueFamilies))
  , m_pMemoryAllocator(createVulkanMemoryAllocator(*this, m_instance.loadVmaFcts(m_pVkDevice.get())))
  , m_pCommandQueue(createVulkanCommandQueue(*this, m_commandQueueInfo, "MainQueue"))
{
    m_pLogger->info("Successfully initialized");
}

VulkanDevice::~VulkanDevice()
{
    m_pLogger->debug("Waiting for device to be idle");
    m_fcts.vkDeviceWaitIdle(m_pVkDevice.get());
    m_pLogger->debug("Device idle, destroying");
}

auto VulkanDevice::createVkSemaphore() const -> VkUniquePtr<VkSemaphore>
{
    VkSemaphoreCreateInfo vkCreateInfo{.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO};

    VkSemaphore vkSemaphore{};
    throwIfVkFailed(m_fcts.vkCreateSemaphore(m_pVkDevice.get(), &vkCreateInfo, nullptr, &vkSemaphore),
                    "Failed to create semaphore for Vulkan Device");

    return makeVkUniquePtr<VkSemaphore>(m_pVkDevice.get(), vkSemaphore, m_fcts.vkDestroySemaphore);
}

auto VulkanDevice::createVkFence(VkFenceCreateFlags vkFlags) const -> VkUniquePtr<VkFence>
{
    VkFenceCreateInfo vkCreateInfo{.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO, .flags = vkFlags};

    VkFence vkFence{};
    throwIfVkFailed(m_fcts.vkCreateFence(m_pVkDevice.get(), &vkCreateInfo, nullptr, &vkFence),
                    "Could not create fence for Vulkan device");

    return makeVkUniquePtr<VkFence>(m_pVkDevice.get(), vkFence, m_fcts.vkDestroyFence);
}

void VulkanDevice::waitForVkFence(VkFence vkFence) const
{
    if (!vkFence)
    {
        return;
    }
    throwIfVkFailed(
        m_fcts.vkWaitForFences(m_pVkDevice.get(), 1U, &vkFence, VK_TRUE, std::numeric_limits<uint64_t>::max()),
        "Failed to wait for fence from Vulkan Device");
}

auto VulkanDevice::createLogger(std::string_view name) const -> std::unique_ptr<ILogger>
{
    return m_pLogger->createChild(name);
}

auto VulkanDevice::getImageFactory() const -> std::shared_ptr<const IImageFactory>
{
    if (!m_pImageFactory)
    {
        auto pThis = this->shared_from_this();
        m_pImageFactory = createVulkanImageFactory(pThis, m_pMemoryAllocator);
    }
    return m_pImageFactory;
}

auto im3e::createDevice(const ILogger& rLogger, DeviceConfig config) -> std::shared_ptr<IDevice>
{
    return std::make_shared<VulkanDevice>(rLogger, std::move(config));
}