#include "vulkan_instance.h"

#include <im3e/utils/core/throw_utils.h>

using namespace im3e;

namespace {

auto createVulkanAppInfo()
{
    return VkApplicationInfo{
        .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
        .pApplicationName = "IcyMoonEngine",
        .applicationVersion = VK_MAKE_VERSION(1, 0, 0),  // NOLINT warning due to Vulkan macro
        .pEngineName = "IcyMoonEngine",
        .engineVersion = VK_MAKE_VERSION(0, 1, 0),            // NOLINT warning due to Vulkan macro
        .apiVersion = VulkanInstance::getVulkanApiVersion(),  // NOLINT warning due to Vulkan macro
    };
}

auto createVulkanInstanceCreateInfo(const VkApplicationInfo& rVkAppInfo, const VulkanExtensions& rExtensions)
{
    const auto& rInstanceExtensions = rExtensions.getInstanceExtensions();
    const auto& rLayers = rExtensions.getLayers();

    return VkInstanceCreateInfo{
        .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        .pApplicationInfo = &rVkAppInfo,
        .enabledLayerCount = static_cast<uint32_t>(rLayers.size()),
        .ppEnabledLayerNames = rLayers.data(),
        .enabledExtensionCount = static_cast<uint32_t>(rInstanceExtensions.size()),
        .ppEnabledExtensionNames = rInstanceExtensions.data(),
    };
}

auto createVkInstance(ILogger& rLogger, const VulkanGlobalFcts& rFcts, const VulkanExtensions& rExtensions,
                      bool isDebugEnabled)
{
    const auto vkAppInfo = createVulkanAppInfo();
    auto vkCreateInfo = createVulkanInstanceCreateInfo(vkAppInfo, rExtensions);

    const auto vkDebugCreateInfo = VulkanDebugMessageHandler::generateDebugUtilsCreateInfo(rLogger);
    if (isDebugEnabled)
    {
        vkCreateInfo.pNext = &vkDebugCreateInfo;
    }

    VkInstance vkInstance{};
    throwIfVkFailed(rFcts.vkCreateInstance(&vkCreateInfo, nullptr, &vkInstance), "Failed to create Vulkan instance");
    return vkInstance;
}

}  // namespace

VulkanInstance::VulkanInstance(const ILogger& rLogger, bool isDebugEnabled,
                               const std::vector<const char*>& rRequiredInstanceExtensions,
                               std::unique_ptr<IVulkanLoader> pLoader)
  : m_pLogger(rLogger.createChild("VulkanInstance"))
  , m_pLoader(throwIfArgNull(std::move(pLoader), "Vulkan instance requires a Vulkan loader"))
  , m_globalFcts(m_pLoader->loadGlobalFcts())
  , m_extensions(rLogger, m_globalFcts, isDebugEnabled, rRequiredInstanceExtensions)
{
    auto vkInstance = createVkInstance(*m_pLogger, m_globalFcts, m_extensions, isDebugEnabled);
    m_fcts = m_pLoader->loadInstanceFcts(vkInstance);
    m_pVkInstance = VkUniquePtr<VkInstance>(vkInstance,
                                            [this](auto vkInst) { m_fcts.vkDestroyInstance(vkInst, nullptr); });

    if (isDebugEnabled)
    {
        m_pDebugMessageHandler = std::make_unique<VulkanDebugMessageHandler>(*m_pLogger, m_fcts, m_pVkInstance.get());
    }
}

auto VulkanInstance::loadDeviceFcts(VkDevice vkDevice) const -> VulkanDeviceFcts
{
    return m_pLoader->loadDeviceFcts(vkDevice);
}

auto VulkanInstance::loadVmaFcts(VkDevice vkDevice) const -> VmaVulkanFunctions
{
    return m_pLoader->loadVmaFcts(m_pVkInstance.get(), vkDevice);
}

auto VulkanInstance::choosePhysicalDevice(const IsPresentationSupportedFct& rIsPresentationSupported) const
    -> VulkanPhysicalDevice
{
    VulkanPhysicalDevices physicalDevices(*m_pLogger, m_fcts, m_extensions, m_pVkInstance.get(),
                                          rIsPresentationSupported);
    return physicalDevices.choosePhysicalDevice();
}
