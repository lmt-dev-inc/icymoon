#include "integration_test.h"

#include <im3e/devices/devices.h>
#include <im3e/utils/core/platform_utils.h>
#include <im3e/utils/loggers.h>

#include <fmt/format.h>

using namespace im3e;

namespace {

constexpr auto TestsLogLevel = LogLevel::Debug;

auto getTestName()
{
    auto* testInfo = testing::UnitTest::GetInstance()->current_test_info();
    const std::string name(testInfo->name());
    return name.substr(name.find_last_of("/") + 1U);
}

}  // namespace

IntegrationTest::IntegrationTest()
  : m_name([] {
      const auto name = getTestName();
      s_pLogger->info(fmt::format("{:*^100}", name));
      return name;
  }())
{
}

void IntegrationTest::SetUp()
{
    EXPECT_THAT(s_pLoggerTracker->getErrors(), IsEmpty()) << "Start of error log tracking";
    s_pLoggerTracker->clearErrors();

    m_pErrorTrackerScope = UniquePtrWithDeleter<void>(reinterpret_cast<void*>(0x1234), [](auto*) {
        EXPECT_THAT(s_pLoggerTracker->getErrors(), IsEmpty()) << "End of error log tracking";
        s_pLoggerTracker->clearErrors();
    });
}

void IntegrationTest::TearDown()
{
    m_pErrorTrackerScope.reset();
    s_pLogger->info(fmt::format("{:*>100}", ""));
}

void IntegrationTest::SetUpTestSuite()
{
    if (!s_pLogger)
    {
        auto* pTestSuite = testing::UnitTest::GetInstance()->current_test_suite();
        s_suiteName = std::string(pTestSuite->name());
        if (auto itFind = s_suiteName.find_first_of("/"); itFind != std::string::npos)
        {
            s_suiteName.erase(s_suiteName.find_first_of("/"));
        }

        const auto logFilePath = generateTestOutputFilePath(s_suiteName, s_suiteName, "log");
        s_pLogger = createFileLogger(logFilePath);
        s_pLogger->setLevelFilter(TestsLogLevel);
        s_pLoggerTracker = s_pLogger->createGlobalTracker();
    }
}

void IntegrationTest::TearDownTestSuite()
{
    s_pLogger.reset();
}
