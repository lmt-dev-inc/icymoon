#include "vulkan_instance.h"

#include "vulkan_debug_message_handler.h"

#include <im3e/utils/core/throw_utils.h>
#include <im3e/vulkan/utils.h>

using namespace im3e;

namespace {

auto createVulkanAppInfo() -> VkApplicationInfo
{
    return VkApplicationInfo{
        .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
        .pApplicationName = "IcyMoonEngine",
        .applicationVersion = VK_MAKE_VERSION(1, 0, 0),  // NOLINT warning due to Vulkan macro
        .pEngineName = "IcyMoonEngine",
        .engineVersion = VK_MAKE_VERSION(0, 1, 0),             // NOLINT warning due to Vulkan macro
        .apiVersion = IVulkanInstance::getVulkanApiVersion(),  // NOLINT warning due to Vulkan macro
    };
}

auto createVulkanInstanceCreateInfo(const VkApplicationInfo& vkAppInfo, const IVulkanExtensions& extensions)
    -> VkInstanceCreateInfo
{
    const auto& instanceExtensions = extensions.getInstanceExtensions();
    const auto& layers = extensions.getLayers();

    return VkInstanceCreateInfo{
        .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        .pApplicationInfo = &vkAppInfo,
        .enabledLayerCount = static_cast<uint32_t>(layers.size()),
        .ppEnabledLayerNames = layers.data(),
        .enabledExtensionCount = static_cast<uint32_t>(instanceExtensions.size()),
        .ppEnabledExtensionNames = instanceExtensions.data(),
    };
}

auto createVkInstance(ILogger& logger, const VulkanGlobalFcts& globalFcts, const IVulkanExtensions& extensions,
                      bool isDebugEnabled) -> VkInstance
{
    const auto vkAppInfo = createVulkanAppInfo();
    auto vkCreateInfo = createVulkanInstanceCreateInfo(vkAppInfo, extensions);

    const auto vkDebugCreateInfo = VulkanDebugMessageHandler::generateDebugUtilsCreateInfo(logger);
    if (isDebugEnabled)
    {
        vkCreateInfo.pNext = &vkDebugCreateInfo;
    }

    VkInstance vkInstance{};
    throwIfVkFailed(globalFcts.vkCreateInstance(&vkCreateInfo, nullptr, &vkInstance),
                    "Failed to create Vulkan instance");
    return vkInstance;
}

class VulkanInstance : public IVulkanInstance
{
public:
    VulkanInstance(const ILogger& logger, bool isDebugEnabled,
                   const std::vector<const char*>& requiredInstanceExtensions, std::unique_ptr<IVulkanLoader> loader);

    auto loadDeviceFcts(VkDevice vkDevice) const -> VulkanDeviceFcts override;
    auto loadVmaFcts(VkDevice vkDevice) const -> VmaVulkanFunctions override;
    auto choosePhysicalDevice(const IsPresentationSupportedFct& isPresentationSupportedFct) const
        -> VulkanPhysicalDevice override;

    auto getFcts() const -> const VulkanInstanceFcts& override { return m_fcts; }
    auto getExtensions() const -> const IVulkanExtensions& override { return *m_extensions; }
    auto getVkInstance() const -> VkInstance override { return m_vkInstance.get(); }

private:
    std::unique_ptr<ILogger> m_logger;
    std::unique_ptr<IVulkanLoader> m_loader;
    VulkanGlobalFcts m_globalFcts;

    std::unique_ptr<IVulkanExtensions> m_extensions;

    VulkanInstanceFcts m_fcts;
    VkUniquePtr<VkInstance> m_vkInstance;
    std::unique_ptr<VulkanDebugMessageHandler> m_debugMessageHandler;
};

VulkanInstance::VulkanInstance(const ILogger& logger, bool isDebugEnabled,
                               const std::vector<const char*>& requiredInstanceExtensions,
                               std::unique_ptr<IVulkanLoader> loader)
  : m_logger(logger.createChild("VulkanInstance"))
  , m_loader(throwIfArgNull(std::move(loader), "Vulkan instance requires a Vulkan loader"))
  , m_globalFcts(m_loader->loadGlobalFcts())
  , m_extensions(createVulkanExtensions(logger, m_globalFcts, isDebugEnabled, requiredInstanceExtensions))
{
    auto vkInstance = createVkInstance(*m_logger, m_globalFcts, *m_extensions, isDebugEnabled);
    m_fcts = m_loader->loadInstanceFcts(vkInstance);
    m_vkInstance = VkUniquePtr<VkInstance>(vkInstance,
                                           [this](auto vkInst) { m_fcts.vkDestroyInstance(vkInst, nullptr); });

    if (isDebugEnabled)
    {
        m_debugMessageHandler = std::make_unique<VulkanDebugMessageHandler>(*m_logger, m_fcts, m_vkInstance.get());
    }
}

auto VulkanInstance::loadDeviceFcts(VkDevice vkDevice) const -> VulkanDeviceFcts
{
    return m_loader->loadDeviceFcts(vkDevice);
}

auto VulkanInstance::loadVmaFcts(VkDevice vkDevice) const -> VmaVulkanFunctions
{
    return m_loader->loadVmaFcts(m_vkInstance.get(), vkDevice);
}

auto VulkanInstance::choosePhysicalDevice(const IsPresentationSupportedFct& isPresentationSupportedFct) const
    -> VulkanPhysicalDevice
{
    VulkanPhysicalDevices physicalDevices(*m_logger, m_fcts, *m_extensions, m_vkInstance.get(),
                                          isPresentationSupportedFct);
    return physicalDevices.choosePhysicalDevice();
}

}  // namespace

auto im3e::createVulkanInstance(const ILogger& logger, bool isDebugEnabled,
                                const std::vector<const char*>& requiredInstanceExtensions,
                                std::unique_ptr<IVulkanLoader> loader) -> std::unique_ptr<IVulkanInstance>
{
    return std::make_unique<VulkanInstance>(logger, isDebugEnabled, requiredInstanceExtensions, std::move(loader));
}
