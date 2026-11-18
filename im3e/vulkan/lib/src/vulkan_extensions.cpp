#include "vulkan_extensions.h"

#include <im3e/vulkan/utils.h>

#include <fmt/format.h>

#include <algorithm>
#include <ranges>

using namespace im3e;

namespace {

void insertIfUnique(const char* item, std::vector<const char*>& items)
{
    const auto toStringView = [](const char* item) { return std::string_view{item}; };
    if (std::ranges::find(items, std::string_view{item}, toStringView) == items.end())
    {
        items.push_back(item);
    }
}

auto generateDeviceExtensions()
{
    std::vector<const char*> deviceExtensions;

    // For presentation:
    insertIfUnique(VK_KHR_SWAPCHAIN_EXTENSION_NAME, deviceExtensions);

    return deviceExtensions;
}

void addLayers(bool isVkValidationEnabled, std::vector<const char*>& layers)
{
    if (isVkValidationEnabled)
    {
        insertIfUnique("VK_LAYER_KHRONOS_validation", layers);
    }
}

void addInstanceExtensions(bool isVkValidationEnabled, std::vector<const char*>& extensions)
{
    // For presentation:
    insertIfUnique(VK_KHR_SURFACE_EXTENSION_NAME, extensions);

    if (isVkValidationEnabled)
    {
        insertIfUnique(VK_EXT_DEBUG_UTILS_EXTENSION_NAME, extensions);
    }
}

auto generateInstanceExtensions(const ILogger& logger, const VulkanGlobalFcts& fcts, bool isVkValidationEnabled,
                                const std::vector<const char*>& requiredExtensions)
{
    std::vector<const char*> extensions;
    std::ranges::for_each(requiredExtensions, [&](auto& extension) { insertIfUnique(extension, extensions); });
    addInstanceExtensions(isVkValidationEnabled, extensions);

    logger.info("Checking support for required instance extensions");
    const auto supportedExtensions = getVkList<VkExtensionProperties>(fcts.vkEnumerateInstanceExtensionProperties,
                                                                      "supported instance extensions", nullptr);

    bool allSupported = true;
    for (auto& extension : extensions)
    {
        auto itFind = std::ranges::find_if(
            supportedExtensions, [&](const auto& props) { return strcmp(extension, &props.extensionName[0]) == 0; });

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
    throwIfFalse<std::runtime_error>(allSupported, "Some required instance extensions are not supported");

    return extensions;
}

auto generateLayers(const ILogger& logger, const VulkanGlobalFcts& fcts, bool isVkValidationEnabled)
{
    std::vector<const char*> layers;
    addLayers(isVkValidationEnabled, layers);

    logger.info("Checking support for required layers");
    const auto supportedLayers = getVkList<VkLayerProperties>(fcts.vkEnumerateInstanceLayerProperties,
                                                              "supported layers");

    bool allSupported = true;
    auto itLayer = layers.begin();
    while (itLayer != layers.end())
    {
        auto itFind = std::ranges::find_if(
            supportedLayers, [&](const auto& props) { return strcmp(*itLayer, &props.layerName[0]) == 0; });

        if (itFind != supportedLayers.end())
        {
            logger.info(fmt::format("OK - {} v{}", itFind->layerName, itFind->specVersion));
            itLayer++;
        }
        else
        {
            logger.info(fmt::format("FAILED - {} - Removing from layers", *itLayer));
            itLayer = layers.erase(itLayer);
            allSupported = false;
        }
    }
    if (!allSupported)
    {
        logger.warning("Some layers were removed because they are not supported by the current instance");
    }
    return layers;
}

class VulkanExtensions : public IVulkanExtensions
{
public:
    VulkanExtensions(const ILogger& logger, const VulkanGlobalFcts& fcts, bool isDebugEnabled,
                     const std::vector<const char*>& requiredInstanceExtensions)
      : m_debugUtilsEnabled(isDebugEnabled)
      , m_instanceExtensions(generateInstanceExtensions(logger, fcts, m_debugUtilsEnabled, requiredInstanceExtensions))
      , m_deviceExtensions(generateDeviceExtensions())
      , m_layers(generateLayers(logger, fcts, m_debugUtilsEnabled))
    {
    }

    auto getInstanceExtensions() const -> const std::vector<const char*>& override { return m_instanceExtensions; }
    auto getDeviceExtensions() const -> const std::vector<const char*>& override { return m_deviceExtensions; }
    auto getLayers() const -> const std::vector<const char*>& override { return m_layers; }

    auto areDebugUtilsEnabled() const -> bool override { return m_debugUtilsEnabled; }

private:
    const bool m_debugUtilsEnabled = false;
    std::vector<const char*> m_instanceExtensions;
    std::vector<const char*> m_deviceExtensions;
    std::vector<const char*> m_layers;
};

}  // namespace

auto im3e::createVulkanExtensions(const ILogger& logger, const VulkanGlobalFcts& fcts, bool isDebugEnabled,
                                  const std::vector<const char*>& requiredInstanceExtensions)
    -> std::unique_ptr<IVulkanExtensions>
{
    return std::make_unique<VulkanExtensions>(logger, fcts, isDebugEnabled, requiredInstanceExtensions);
}
