#include "vulkan_debug_message_handler.h"

#include <im3e/utils/core/throw_utils.h>
#include <im3e/utils/vk_utils.h>

#include <fmt/format.h>

#include <string>

using namespace im3e;

namespace {

std::string toString(VkDebugUtilsMessageTypeFlagsEXT type)
{
    std::string result;
    if (vkFlagsContain(type, VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT))
    {
        result += "[General] ";
    }
    if (vkFlagsContain(type, VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT))
    {
        result += "[Validation] ";
    }
    if (vkFlagsContain(type, VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT))
    {
        result += "[Performance] ";
    }
    return result;
}

VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT severity,
                                             VkDebugUtilsMessageTypeFlagsEXT type,
                                             const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData)

{
    const auto& logger = *reinterpret_cast<const ILogger*>(pUserData);
    auto message = fmt::format(R"({}{})", toString(type), pCallbackData->pMessage);

    switch (severity)
    {
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT: logger.verbose(message); break;
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT: logger.info(message); break;
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT: logger.warning(message); break;
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT: logger.error(message); break;
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_FLAG_BITS_MAX_ENUM_EXT:
        default: logger.error(fmt::format("Unknown severity: {}", message)); break;
    }

    return VK_FALSE;
}

auto createVkMessenger(ILogger& rLogger, const VulkanInstanceFcts& rFcts, VkInstance vkInstance)
{
    throwIfArgNull(vkInstance, "Cannot create debug message handler without a Vulkan instance");

    auto vkCreateInfo = VulkanDebugMessageHandler::generateDebugUtilsCreateInfo(rLogger);

    VkDebugUtilsMessengerEXT vkMessenger{};
    throwIfVkFailed(rFcts.vkCreateDebugUtilsMessengerEXT(vkInstance, &vkCreateInfo, nullptr, &vkMessenger),
                    "Failed to create Vk Debug Messenger");

    return VkUniquePtr<VkDebugUtilsMessengerEXT>(vkMessenger, [vkInstance, pFcts = &rFcts](auto vkMessenger) {
        pFcts->vkDestroyDebugUtilsMessengerEXT(vkInstance, vkMessenger, nullptr);
    });
}

}  // namespace

VulkanDebugMessageHandler::VulkanDebugMessageHandler(const ILogger& logger, const VulkanInstanceFcts& rFcts,
                                                     VkInstance vkInstance)
  : m_pLogger(logger.createChild("VulkanDebugMessageHandler"))
  , m_rFcts(rFcts)
  , m_pVkMessenger(createVkMessenger(*m_pLogger, m_rFcts, vkInstance))
{
    m_pLogger->info("Successfully initialized");
}

VkDebugUtilsMessengerCreateInfoEXT VulkanDebugMessageHandler::generateDebugUtilsCreateInfo(ILogger& logger)
{
    VkDebugUtilsMessengerCreateInfoEXT createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
                                 VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
                                 VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                             VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                             VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    createInfo.pfnUserCallback = debugCallback;
    createInfo.pUserData = &logger;
    return createInfo;
}