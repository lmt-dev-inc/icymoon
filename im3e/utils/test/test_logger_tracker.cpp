#include "src/stream_logger.h"

#include <im3e/test_utils/test_utils.h>

using namespace im3e;

TEST(LoggerTracker, constructor)
{
    LoggerTracker tracker;
    EXPECT_THAT(tracker.getErrors().empty(), IsTrue());
}

TEST(LoggerTracker, addError)
{
    LoggerTracker tracker;

    const std::vector<std::string> expectedErrors{
        "error message",
        "another error",
        "error2",
    };
    std::ranges::for_each(expectedErrors, [&](const auto& rMsg) { tracker.addError(rMsg); });

    const auto actualErrors = tracker.getErrors();
    EXPECT_THAT(actualErrors, ContainerEq(expectedErrors));
}

TEST(LoggerTracker, clearErrors)
{
    LoggerTracker tracker;
    tracker.addError("error message");
    tracker.addError("another error");
    EXPECT_THAT(tracker.getErrors().size(), Eq(2U));

    tracker.clearErrors();
    EXPECT_THAT(tracker.getErrors().empty(), IsTrue());
}