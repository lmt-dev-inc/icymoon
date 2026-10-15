#include "src/vulkan_debug_message_handler.h"

#include <im3e/devices/vulkan_loaders/mock/mock_vulkan_loader.h>
#include <im3e/test_utils/test_utils.h>
#include <im3e/utils/mock/mock_logger.h>

using namespace im3e;

struct VulkanDebugMessageHandlerTest : public Test
{
    auto createHandler()
    {
        EXPECT_CALL(m_mockLogger, createChild(StrEq("VulkanDebugMessageHandler")));
        EXPECT_CALL(m_rMockFcts, vkCreateDebugUtilsMessengerEXT(m_vkInstance, NotNull(), _, NotNull()))
            .WillOnce(Invoke([&](Unused, auto* pCreateInfo, Unused, auto* pVkMessenger) {
                EXPECT_THAT(pCreateInfo->sType, Eq(VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT));
                EXPECT_THAT(pCreateInfo->messageSeverity, Eq(VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
                                                             VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
                                                             VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT));
                EXPECT_THAT(pCreateInfo->messageType, Eq(VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                                                         VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                                                         VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT));
                m_debugCallback = pCreateInfo->pfnUserCallback;
                EXPECT_THAT(pCreateInfo->pUserData, NotNull());
                *pVkMessenger = m_vkMessenger;
                return VK_SUCCESS;
            }));

        return VulkanDebugMessageHandler(m_mockLogger, m_rFcts, m_vkInstance);
    }

    NiceMock<MockLogger> m_mockLogger;
    NiceMock<MockVulkanLoader> m_mockVulkanLoader;
    MockVulkanInstanceFcts& m_rMockFcts = m_mockVulkanLoader.getMockInstanceFcts();

    const VulkanInstanceFcts& m_rFcts = m_mockVulkanLoader.getInstanceFcts();

    const VkInstance m_vkInstance = reinterpret_cast<VkInstance>(0xb324a7ef43);
    const VkDebugUtilsMessengerEXT m_vkMessenger = reinterpret_cast<VkDebugUtilsMessengerEXT>(0xabef45f);

    std::function<VkBool32(VkDebugUtilsMessageSeverityFlagBitsEXT, VkDebugUtilsMessageTypeFlagsEXT,
                           const VkDebugUtilsMessengerCallbackDataEXT*, void*)>
        m_debugCallback;
};

TEST_F(VulkanDebugMessageHandlerTest, constructorThrowsWithoutInstance)
{
    EXPECT_THROW(VulkanDebugMessageHandler handler(m_mockLogger, m_rFcts, VK_NULL_HANDLE), std::invalid_argument);
}

TEST_F(VulkanDebugMessageHandlerTest, constructor)
{
    auto handler = createHandler();
    ASSERT_THAT(m_debugCallback, NotNull());

    EXPECT_CALL(m_rMockFcts, vkDestroyDebugUtilsMessengerEXT(m_vkInstance, m_vkMessenger, IsNull()));
}

TEST_F(VulkanDebugMessageHandlerTest, debugCallbackVerbose)
{
    auto handler = createHandler();

    const VkDebugUtilsMessengerCallbackDataEXT callbackData{
        .pMessage = "verbose message",
    };
    EXPECT_CALL(m_mockLogger, verbose("[Performance] verbose message"));
    m_debugCallback(VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT, VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT,
                    &callbackData, &m_mockLogger);
}

TEST_F(VulkanDebugMessageHandlerTest, debugCallbackInfo)
{
    auto handler = createHandler();

    const VkDebugUtilsMessengerCallbackDataEXT callbackData{
        .pMessage = "Hello World!",
    };
    EXPECT_CALL(m_mockLogger, info("[General] Hello World!"));
    m_debugCallback(VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT, VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT,
                    &callbackData, &m_mockLogger);
}

TEST_F(VulkanDebugMessageHandlerTest, debugCallbackWarning)
{
    auto handler = createHandler();

    const VkDebugUtilsMessengerCallbackDataEXT callbackData{
        .pMessage = "this is a warning",
    };
    EXPECT_CALL(m_mockLogger, warning("[Validation] this is a warning"));
    m_debugCallback(VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT, VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT,
                    &callbackData, &m_mockLogger);
}

TEST_F(VulkanDebugMessageHandlerTest, debugCallbackError)
{
    auto handler = createHandler();

    const VkDebugUtilsMessengerCallbackDataEXT callbackData{
        .pMessage = "an error occured",
    };
    EXPECT_CALL(m_mockLogger, error("[General] an error occured"));
    m_debugCallback(VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT, VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT,
                    &callbackData, &m_mockLogger);
}

TEST_F(VulkanDebugMessageHandlerTest, debugCallbackUnknow)
{
    auto handler = createHandler();

    const VkDebugUtilsMessengerCallbackDataEXT callbackData{
        .pMessage = "a message",
    };
    EXPECT_CALL(m_mockLogger, error("Unknown severity: a message"));
    m_debugCallback(VK_DEBUG_UTILS_MESSAGE_SEVERITY_FLAG_BITS_MAX_ENUM_EXT,
                    VK_DEBUG_UTILS_MESSAGE_TYPE_DEVICE_ADDRESS_BINDING_BIT_EXT, &callbackData, &m_mockLogger);
}