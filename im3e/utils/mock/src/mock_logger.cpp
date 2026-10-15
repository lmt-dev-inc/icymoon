#include "mock_logger.h"

using namespace im3e;

namespace {

class MockProxyLoggerTracker : public ILoggerTracker
{
public:
    MockProxyLoggerTracker(MockLoggerTracker& rMock)
      : m_rMock(rMock)
    {
    }

    void clearErrors() override { m_rMock.clearErrors(); }

    auto getErrors() const -> std::vector<std::string> override { return m_rMock.getErrors(); }

private:
    MockLoggerTracker& m_rMock;
};

}  // namespace

MockLoggerTracker::MockLoggerTracker() = default;
MockLoggerTracker::~MockLoggerTracker() = default;

auto MockLoggerTracker::createMockProxy() -> std::unique_ptr<ILoggerTracker>
{
    return std::make_unique<MockProxyLoggerTracker>(*this);
}

namespace {

class MockProxyLogger : public ILogger
{
public:
    MockProxyLogger(MockLogger& rMock)
      : m_rMock(rMock)
    {
    }

    void setLevelFilter(LogLevel level) override { m_rMock.setLevelFilter(level); }

    void error(std::string_view message) const override { m_rMock.error(message); }
    void warning(std::string_view message) const override { m_rMock.warning(message); }
    void info(std::string_view message) const override { m_rMock.info(message); }
    void debug(std::string_view message) const override { m_rMock.debug(message); }
    void verbose(std::string_view message) const override { m_rMock.verbose(message); }

    auto createChild(std::string_view category) const -> std::unique_ptr<ILogger> override
    {
        return m_rMock.createChild(category);
    }

    auto createGlobalTracker() -> UniquePtrWithDeleter<ILoggerTracker> override
    {
        return m_rMock.createGlobalTracker();
    }

private:
    MockLogger& m_rMock;
};

}  // namespace

MockLogger::MockLogger()
{
    ON_CALL(*this, createChild(_)).WillByDefault(InvokeWithoutArgs([&] {
        return std::make_unique<MockProxyLogger>(*this);
    }));
    ON_CALL(*this, createGlobalTracker()).WillByDefault(Invoke([&] { return m_mockTracker.createMockProxy(); }));
}

MockLogger::~MockLogger() = default;

auto MockLogger::createMockProxy() -> std::unique_ptr<ILogger>
{
    return std::make_unique<MockProxyLogger>(*this);
}
