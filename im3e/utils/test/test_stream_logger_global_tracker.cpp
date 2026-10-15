#include "loggers.h"
#include "src/stream_logger.h"

#include <im3e/test_utils/test_utils.h>

#include <fmt/format.h>

#include <sstream>

using namespace im3e;

namespace {

auto createStreamLogger()
{
    return std::make_unique<StreamLogger>("Test", std::make_shared<std::stringstream>());
}

}  // namespace

class StreamLoggerGlobalTrackerTest : public Test
{
public:
    StreamLoggerGlobalTrackerTest()
      : m_pLogger(createStreamLogger())
      , m_pTracker(m_pLogger->createGlobalTracker())
    {
    }

protected:
    std::unique_ptr<ILogger> m_pLogger;
    UniquePtrWithDeleter<ILoggerTracker> m_pTracker;
};

TEST_F(StreamLoggerGlobalTrackerTest, canCreateTrackers)
{
    ASSERT_THAT(m_pLogger->createGlobalTracker(), NotNull());
    ASSERT_THAT(m_pLogger->createGlobalTracker(), NotNull());
    ASSERT_THAT(m_pLogger->createGlobalTracker(), NotNull());
}

TEST_F(StreamLoggerGlobalTrackerTest, trackerStartsWithNoError)
{
    auto pLogger = createStreamLogger();
    auto pTracker = pLogger->createGlobalTracker();
    EXPECT_THAT(pTracker->getErrors(), IsEmpty());
}

TEST_F(StreamLoggerGlobalTrackerTest, canCollectLocalErrors)
{
    const auto message1 = "my error message";
    m_pLogger->error(message1);

    auto collectedErrors = m_pTracker->getErrors();
    ASSERT_THAT(collectedErrors.size(), Eq(1U));
    EXPECT_THAT(collectedErrors.front(), StrEq(message1));

    const auto message2 = "my second error";
    m_pLogger->error(message2);

    collectedErrors = m_pTracker->getErrors();
    ASSERT_THAT(collectedErrors.size(), Eq(2U));
    EXPECT_THAT(collectedErrors[0], StrEq(message1));
    EXPECT_THAT(collectedErrors[1], StrEq(message2));
}

TEST_F(StreamLoggerGlobalTrackerTest, canCollectChildErrors)
{
    auto pChildLogger = m_pLogger->createChild("Child");

    const auto message = "hello from child";
    pChildLogger->error(message);

    const auto collectedErrors = m_pTracker->getErrors();
    ASSERT_THAT(collectedErrors.size(), Eq(1U));
    EXPECT_THAT(collectedErrors.front(), StrEq(message));
}

TEST_F(StreamLoggerGlobalTrackerTest, canCollectParentErrors)
{
    auto pChildLogger = m_pLogger->createChild("Child");
    auto pChildTracker = pChildLogger->createGlobalTracker();

    const auto message = "message from parent";
    m_pLogger->error(message);

    const auto collectedErrors = m_pTracker->getErrors();
    ASSERT_THAT(collectedErrors.size(), Eq(1U));
    EXPECT_THAT(collectedErrors.front(), StrEq(message));
}

TEST_F(StreamLoggerGlobalTrackerTest, clearErrors)
{
    m_pLogger->error("error 1");
    m_pLogger->error("error 2");

    EXPECT_THAT(m_pTracker->getErrors().size(), Eq(2U));

    m_pTracker->clearErrors();
    EXPECT_THAT(m_pTracker->getErrors().size(), Eq(0U));
}

TEST_F(StreamLoggerGlobalTrackerTest, canBeDestroyed)
{
    m_pLogger->error("message");
    EXPECT_THAT(m_pTracker->getErrors().size(), Eq(1U));

    m_pTracker.reset();

    m_pLogger->error("message2");
}

TEST_F(StreamLoggerGlobalTrackerTest, canBeDestroyedAfterLoggerDestroyed)
{
    m_pLogger->error("message");

    m_pLogger.reset();

    const auto errors = m_pTracker->getErrors();
    ASSERT_THAT(errors.size(), Eq(1U));
    EXPECT_THAT(errors.front(), StrEq("message"));

    m_pTracker.reset();
}
