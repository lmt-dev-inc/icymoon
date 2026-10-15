#include "loggers.h"

#include <im3e/test_utils/test_utils.h>

using namespace im3e;

TEST(TerminalLoggerTest, canCreateTerminalLogger)
{
    auto pLogger = createTerminalLogger();
    ASSERT_THAT(pLogger, NotNull());
}
