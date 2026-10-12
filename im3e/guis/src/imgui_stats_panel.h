#pragma once

#include <im3e/api/gui.h>
#include <im3e/utils/stats.h>

#include <filesystem>
#include <map>
#include <mutex>

namespace im3e {

class ImguiStatsPanel : public IGuiPanel
{
public:
    ImguiStatsPanel(std::string_view name, std::shared_ptr<IStatsProvider> pStatsProvider);
    ~ImguiStatsPanel() override;

    void draw(const ICommandBuffer&) override;

    auto getName() const -> std::string override { return m_name; }

private:
    const std::string m_name;
    std::shared_ptr<IStatsProvider> m_pStatsProvider;

    std::mutex m_mutex;

    struct SpanStats
    {
        std::map<std::chrono::steady_clock::time_point, std::chrono::microseconds> durations;
    };
    std::map<std::filesystem::path, SpanStats> m_spanStats;

    class StatsReceiver : public IStatsReceiver
    {
    public:
        StatsReceiver(ImguiStatsPanel& rPanel);

        void onSpanAdded(Span span) override;

    private:
        ImguiStatsPanel& m_rPanel;
    };
    std::shared_ptr<IStatsReceiver> m_pStatsReceiver;
};

}  // namespace im3e