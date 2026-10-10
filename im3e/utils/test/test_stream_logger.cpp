#include "src/stream_logger.h"

#include <im3e/test_utils/test_utils.h>

#include <sstream>

using namespace im3e;

struct StreamLoggerTest : public Test
{
    void logAllMessageLevels()
    {
        m_logger.error("error message");
        m_logger.warning("warning message");
        m_logger.info("info message");
        m_logger.debug("debug message");
        m_logger.verbose("verbose message");
    }

    std::shared_ptr<std::stringstream> m_pStrStream = std::make_shared<std::stringstream>();
    StreamLogger m_logger{"TestLogger", m_pStrStream};
};

TEST_F(StreamLoggerTest, constructorThrowsWithoutStream)
{
    std::shared_ptr<std::ostream> pStream{};
    EXPECT_THROW(StreamLogger logger("logger", pStream), std::invalid_argument);
}

TEST_F(StreamLoggerTest, defaultLevelIsDebug)
{
    logAllMessageLevels();
    EXPECT_THAT(m_pStrStream->str(), StrEq("[E][TestLogger] error message\n"
                                           "[W][TestLogger] warning message\n"
                                           "[I][TestLogger] info message\n"
                                           "[D][TestLogger] debug message\n"));
}

TEST_F(StreamLoggerTest, errorLevel)
{
    m_logger.setLevelFilter(LogLevel::Error);
    logAllMessageLevels();
    EXPECT_THAT(m_pStrStream->str(), StrEq("[E][TestLogger] error message\n"));
}

TEST_F(StreamLoggerTest, warningLevel)
{
    m_logger.setLevelFilter(LogLevel::Warning);
    logAllMessageLevels();
    EXPECT_THAT(m_pStrStream->str(), StrEq("[E][TestLogger] error message\n"
                                           "[W][TestLogger] warning message\n"));
}

TEST_F(StreamLoggerTest, infoLevel)
{
    m_logger.setLevelFilter(LogLevel::Info);
    logAllMessageLevels();
    EXPECT_THAT(m_pStrStream->str(), StrEq("[E][TestLogger] error message\n"
                                           "[W][TestLogger] warning message\n"
                                           "[I][TestLogger] info message\n"));
}

TEST_F(StreamLoggerTest, debugLevel)
{
    m_logger.setLevelFilter(LogLevel::Debug);
    logAllMessageLevels();
    EXPECT_THAT(m_pStrStream->str(), StrEq("[E][TestLogger] error message\n"
                                           "[W][TestLogger] warning message\n"
                                           "[I][TestLogger] info message\n"
                                           "[D][TestLogger] debug message\n"));
}

TEST_F(StreamLoggerTest, verboseLevel)
{
    m_logger.setLevelFilter(LogLevel::Verbose);
    logAllMessageLevels();
    EXPECT_THAT(m_pStrStream->str(), StrEq("[E][TestLogger] error message\n"
                                           "[W][TestLogger] warning message\n"
                                           "[I][TestLogger] info message\n"
                                           "[D][TestLogger] debug message\n"
                                           "[V][TestLogger] verbose message\n"));
}

TEST_F(StreamLoggerTest, createChild)
{
    auto pChildLogger = m_logger.createChild("Child");
    m_logger.info("Created child");
    pChildLogger->debug("Hello from child");
    m_logger.warning("Bye from parent");

    EXPECT_THAT(m_pStrStream->str(), StrEq("[I][TestLogger] Created child\n"
                                           "[D][Child] Hello from child\n"
                                           "[W][TestLogger] Bye from parent\n"));
}