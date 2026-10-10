#include "stats.h"

#include <im3e/utils/core/throw_utils.h>

#include <mutex>
#include <optional>
#include <set>
#include <stack>
#include <thread>
#include <unordered_map>
#include <vector>

using namespace im3e;

namespace {

class StatsProvider : public IStatsProvider, public std::enable_shared_from_this<StatsProvider>
{
public:
    void addReceiver(std::shared_ptr<IStatsReceiver> pReceiver) override
    {
        throwIfArgNull(pReceiver, "Cannot add null receiver to stats provider");

        std::lock_guard lk(m_mutex);
        m_pReceivers.emplace(std::move(pReceiver));
    }

    void removeReceiver(std::shared_ptr<IStatsReceiver> pReceiver) override
    {
        throwIfArgNull(pReceiver, "Cannot remove null receiver from stats provider");

        std::lock_guard lk(m_mutex);
        m_pReceivers.erase(pReceiver);
    }

    class ScopedSpan : public IScopedSpan
    {
    public:
        ScopedSpan(std::shared_ptr<StatsProvider> pProvider, std::filesystem::path spanPath)
          : m_pProvider(throwIfArgNull(std::move(pProvider), "ScopedSpan requires a StatsProvider"))
          , m_span(Span{
                .path = std::move(spanPath),
                .startTime = std::chrono::steady_clock::now(),
            })
        {
        }

        ~ScopedSpan() override
        {
            m_span.endTime = std::chrono::steady_clock::now();
            {
                std::lock_guard lk(m_pProvider->m_mutex);
                for (auto& pReceiver : m_pProvider->m_pReceivers)
                {
                    pReceiver->onSpanAdded(m_span);
                }

                auto& rActiveSpans = m_pProvider->m_threadToActiveSpans[std::this_thread::get_id()];
                throwIfFalse<std::logic_error>(!rActiveSpans.empty() && m_span.path == rActiveSpans.top(),
                                               "Incorrect active span being removed");
                rActiveSpans.pop();
            }
        }

    private:
        std::shared_ptr<StatsProvider> m_pProvider;
        Span m_span;
    };
    auto startScopedSpan(std::string_view name) -> std::unique_ptr<IScopedSpan> override
    {
        std::lock_guard lk(m_mutex);
        auto& rActiveSpans = m_threadToActiveSpans[std::this_thread::get_id()];
        auto spanPath = (rActiveSpans.empty() ? m_rootPath : rActiveSpans.top()) / name;
        rActiveSpans.emplace(spanPath);
        return std::make_unique<ScopedSpan>(this->shared_from_this(), std::move(spanPath));
    }

private:
    const std::filesystem::path m_rootPath{"/", std::filesystem::path::generic_format};
    std::mutex m_mutex;
    std::set<std::shared_ptr<IStatsReceiver>> m_pReceivers;
    std::unordered_map<std::thread::id, std::stack<std::filesystem::path>> m_threadToActiveSpans;
};

}  // namespace

auto im3e::createStatsProvider() -> std::shared_ptr<IStatsProvider>
{
    return std::make_unique<StatsProvider>();
}