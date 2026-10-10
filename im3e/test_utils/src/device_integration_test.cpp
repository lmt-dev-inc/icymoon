#include "device_integration_test.h"

#include <im3e/devices/devices.h>
#include <im3e/utils/loggers.h>

#include <fmt/format.h>

using namespace im3e;

namespace {

constexpr auto TestsLogLevel = LogLevel::Debug;
constexpr bool VulkanValidationEnabled = true;

auto getTestName()
{
    auto* testInfo = testing::UnitTest::GetInstance()->current_test_info();
    const std::string name(testInfo->name());
    return name.substr(name.find_last_of("/") + 1U);
}

}  // namespace

DeviceIntegrationTest::DeviceIntegrationTest()
  : m_name([this] {
      const auto name = getTestName();

      s_pLogger->info(fmt::format("{:*^100}", name));

      // Only create a new device when needed (e.g. when a test fails and the device has to be destroyed)
      if (!s_pDevice)
      {
          s_pDevice = createDevice(*s_pLogger, DeviceConfig{.isDebugEnabled = true});
      }

      return name;
  }())
{
}

void DeviceIntegrationTest::SetUp()
{
    EXPECT_THAT(s_pLoggerTracker->getErrors(), IsEmpty()) << "Start of error log tracking";
    s_pLoggerTracker->clearErrors();

    m_pErrorTrackerScope = UniquePtrWithDeleter<void>(reinterpret_cast<void*>(0x1234), [this](auto*) {
        EXPECT_THAT(s_pLoggerTracker->getErrors(), IsEmpty()) << "End of error log tracking";
        s_pLoggerTracker->clearErrors();
    });
}

void DeviceIntegrationTest::TearDown()
{
    m_pErrorTrackerScope.reset();

    if (Test::HasFailure())
    {
        s_pDevice.reset();
    }
    s_pLogger->info(fmt::format("{:*>100}", ""));
}

void DeviceIntegrationTest::SetUpTestSuite()
{
    if (!s_pLogger)
    {
        auto* pTestSuite = testing::UnitTest::GetInstance()->current_test_suite();
        s_suiteName = std::string(pTestSuite->name());
        if (auto itFind = s_suiteName.find_first_of("/"); itFind != std::string::npos)
        {
            s_suiteName.erase(s_suiteName.find_first_of("/"));
        }

        s_pLogger = createFileLogger(path{fmt::format("{}.log", s_suiteName)});
        s_pLogger->setLevelFilter(TestsLogLevel);
        s_pLoggerTracker = s_pLogger->createGlobalTracker();
    }
}

void DeviceIntegrationTest::TearDownTestSuite()
{
    if (!s_pLogger)
        return;

    // If s_pDevice has more than one use_count at this point, it means a resource was not properly released and the
    // device will not be properly destroyed.
    throwIfFalse<std::logic_error>(
        s_pDevice.use_count() <= 1U,
        fmt::format("The integration test device's use_count is higher than 1 ({}) on test suite tear down",
                    s_pDevice.use_count()));

    s_pDevice.reset();
    s_pLogger.reset();
}

auto DeviceIntegrationTest::generateFilePath(std::string_view name, std::string_view extension) const
    -> std::filesystem::path
{
    return std::filesystem::current_path() /
           std::filesystem::path{fmt::format("{}_{}.{}", getSuiteName(), name, extension)};
}