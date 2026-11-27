#include <im3e/test_utils/test_utils.h>
#include <im3e/utils/core/platform_utils.h>
#include <im3e/utils/loggers.h>
#include <im3e/vulkan/device.h>

#include <filesystem>

using namespace im3e;

struct VulkanDeviceIntegration : public Test
{
    void TearDown() override { EXPECT_THAT(m_pLoggerTracker->getErrors(), IsEmpty()); }

    static void SetUpTestSuite()
    {
        if (!s_pLogger)
        {
            auto* pTestSuite = testing::UnitTest::GetInstance()->current_test_suite();
            auto suiteName = std::string(pTestSuite->name());
            if (auto itFind = suiteName.find_first_of("/"); itFind != std::string::npos)
            {
                suiteName.erase(suiteName.find_first_of("/"));
            }

            const auto logFilePath = generateTestOutputFilePath(suiteName, suiteName, "log");
            s_pLogger = createFileLogger(logFilePath);
            s_pLogger->setLevelFilter(LogLevel::Debug);
        }
    }

    static inline std::unique_ptr<ILogger> s_pLogger;
    UniquePtrWithDeleter<ILoggerTracker> m_pLoggerTracker = s_pLogger->createGlobalTracker();
};

TEST_F(VulkanDeviceIntegration, constructor)
{
    auto pDevice = createDevice(*s_pLogger);
    ASSERT_THAT(pDevice, NotNull());
    EXPECT_THAT(pDevice->isValidationEnabled(), IsFalse());
}

TEST_F(VulkanDeviceIntegration, constructorWithDebugEnabled)
{
    auto pDevice = createDevice(*s_pLogger, DeviceConfig{.isDebugEnabled = true});
    ASSERT_THAT(pDevice, NotNull());
    EXPECT_THAT(pDevice->isValidationEnabled(), IsTrue())
        << "Vulkan validation is not enabled: integration tests require the Khronos validation layer";
}

TEST_F(VulkanDeviceIntegration, constructorWithDebugEnabled_logsValidationErrors)
{
    auto pDevice = createDevice(*s_pLogger, DeviceConfig{.isDebugEnabled = true});
    ASSERT_THAT(pDevice, NotNull());
    ASSERT_THAT(pDevice->isValidationEnabled(), IsTrue())
        << "Vulkan validation is not enabled: integration tests require the Khronos validation layer";

    // A wrong sType is always invalid, and is caught by the stateless checks of the validation layer. The debug
    // callback doesn't abort the call, so it still reaches the driver, which ignores sType: apart from it, the create
    // info is the one of a valid unsignaled fence.
    const VkFenceCreateInfo vkInvalidCreateInfo{
        .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
    };
    VkFence vkFence{};
    pDevice->getFcts().vkCreateFence(pDevice->getVkDevice(), &vkInvalidCreateInfo, nullptr, &vkFence);
    if (vkFence != VK_NULL_HANDLE)
    {
        pDevice->getFcts().vkDestroyFence(pDevice->getVkDevice(), vkFence, nullptr);
    }

    EXPECT_THAT(m_pLoggerTracker->getErrors(), Contains(HasSubstr("VUID-VkFenceCreateInfo-sType-sType")))
        << "The validation error was not logged as an error. If validation is narrowed with vkconfig, make sure "
           "parameter validation is enabled";
    m_pLoggerTracker->clearErrors();
}

TEST_F(VulkanDeviceIntegration, destroyDeviceBeforeDestroyingImage)
{
    auto pDevice = createDevice(*s_pLogger, DeviceConfig{.isDebugEnabled = true});
    auto pImage = pDevice->getImageFactory()->createImage(ImageConfig{
        .vkExtent{24U, 24U},
        .vkFormat = VK_FORMAT_R8G8B8A8_UNORM,
        .vkUsage = VK_IMAGE_USAGE_SAMPLED_BIT,
    });

    pDevice.reset();
}