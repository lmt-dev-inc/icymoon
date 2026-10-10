#include "mock_stats.h"

using namespace im3e;

MockStatsReceiver::MockStatsReceiver() = default;
MockStatsReceiver::~MockStatsReceiver() = default;

namespace {

class MockProxyStatsProvider : public IStatsProvider
{
public:
    MockProxyStatsProvider(MockStatsProvider& rMock)
      : m_rMock(rMock)
    {
    }

    void addReceiver(std::shared_ptr<IStatsReceiver> pReceiver) override { m_rMock.addReceiver(std::move(pReceiver)); }
    void removeReceiver(std::shared_ptr<IStatsReceiver> pReceiver) override
    {
        m_rMock.removeReceiver(std::move(pReceiver));
    }

    auto startScopedSpan(std::string_view name) -> std::unique_ptr<IScopedSpan> override
    {
        return m_rMock.startScopedSpan(name);
    }

private:
    MockStatsProvider& m_rMock;
};

}  // namespace

MockStatsProvider::MockStatsProvider() = default;
MockStatsProvider::~MockStatsProvider() = default;

auto MockStatsProvider::createMockProxy() -> std::unique_ptr<IStatsProvider>
{
    return std::make_unique<MockProxyStatsProvider>(*this);
}
