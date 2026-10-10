#include "imgui_stats_panel.h"

#include "guis.h"

#include <im3e/utils/core/throw_utils.h>
#include <im3e/utils/imgui_utils.h>

#include <fmt/format.h>
#include <imgui.h>

#include <limits>

using namespace im3e;
using namespace std::chrono_literals;
using std::chrono::duration_cast;
using std::chrono::microseconds;
using std::chrono::milliseconds;

namespace {

}

ImguiStatsPanel::ImguiStatsPanel(std::string_view name, std::shared_ptr<IStatsProvider> pStatsProvider)
  : m_name(name)
  , m_pStatsProvider(throwIfArgNull(std::move(pStatsProvider), "ImGui stats panel requires a stats provider"))
  , m_pStatsReceiver(std::make_shared<StatsReceiver>(*this))
{
    m_pStatsProvider->addReceiver(m_pStatsReceiver);
}

ImguiStatsPanel::~ImguiStatsPanel()
{
    m_pStatsProvider->removeReceiver(m_pStatsReceiver);
}

void ImguiStatsPanel::draw(const ICommandBuffer&)
{
    constexpr ImGuiTableFlags TableFlags = ImGuiTableFlags_BordersV | ImGuiTableFlags_BordersOuterH |
                                           ImGuiTableFlags_Resizable | ImGuiTableFlags_RowBg;
    if (auto tableScope = ImguiScope(ImGui::BeginTable("Spans", 6, TableFlags), &ImGui::EndTable))
    {
        ImGui::TableSetupColumn("Span");
        ImGui::TableSetupColumn("Current");
        ImGui::TableSetupColumn("Min");
        ImGui::TableSetupColumn("Average");
        ImGui::TableSetupColumn("Max");
        ImGui::TableSetupColumn("Frequency");
        ImGui::TableHeadersRow();

        std::lock_guard lk(m_mutex);

        for (auto& [rSpanPath, rSpanStats] : m_spanStats)
        {
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::Text("%s", rSpanPath.c_str());

            microseconds curDuration{};
            microseconds minDuration{std::numeric_limits<microseconds::rep>::max()};
            microseconds totalDuration{};
            microseconds maxDuration{};
            float frequency{};

            constexpr auto TimeWindow = 1s;
            auto& rDurations = rSpanStats.durations;

            auto lowerBound = std::chrono::steady_clock::now() - TimeWindow;
            auto itLowerBound = rDurations.lower_bound(lowerBound);
            if (itLowerBound != rDurations.begin())
            {
                itLowerBound--;
                rDurations.erase(rDurations.begin(), itLowerBound);
            }

            if (!rDurations.empty())
            {
                curDuration = rDurations.rbegin()->second;

                for (auto& [rTimestamp, rDuration] : rDurations)
                {
                    minDuration = std::min(minDuration, rDuration);
                    totalDuration += rDuration;
                    maxDuration = std::max(maxDuration, rDuration);
                }

                const auto curTimeWindow = duration_cast<milliseconds>(rDurations.rbegin()->first -
                                                                       rDurations.begin()->first);
                frequency = rDurations.size() / (curTimeWindow.count() / 1.0e3F);
            }

            ImGui::TableNextColumn();
            ImGui::Text("%s", fmt::format("{:.2f}ms", curDuration.count() / 1000.0).c_str());
            ImGui::TableNextColumn();
            ImGui::Text("%s", fmt::format("{:.2f}ms", minDuration.count() / 1000.0).c_str());
            ImGui::TableNextColumn();
            ImGui::Text("%s", fmt::format("{:.2f}ms", totalDuration.count() / (rDurations.size() * 1000.0)).c_str());
            ImGui::TableNextColumn();
            ImGui::Text("%s", fmt::format("{:.2f}ms", maxDuration.count() / 1000.0).c_str());
            ImGui::TableNextColumn();
            ImGui::Text("%s", fmt::format("{:.2f}", frequency).c_str());
        }
    }
}

ImguiStatsPanel::StatsReceiver::StatsReceiver(ImguiStatsPanel& rPanel)
  : m_rPanel(rPanel)
{
}

void ImguiStatsPanel::StatsReceiver::onSpanAdded(Span span)
{
    std::lock_guard lk(m_rPanel.m_mutex);
    auto& rSpanStats = m_rPanel.m_spanStats[span.path];
    rSpanStats.durations.emplace(std::chrono::steady_clock::now(),
                                 duration_cast<microseconds>(span.endTime - span.startTime));
}

auto im3e::createImguiStatsPanel(std::string_view name, std::shared_ptr<IStatsProvider> pStatsProvider)
    -> std::shared_ptr<IGuiPanel>
{
    return std::make_shared<ImguiStatsPanel>(name, std::move(pStatsProvider));
}
