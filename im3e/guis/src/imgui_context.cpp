#include "imgui_context.h"

using namespace im3e;

namespace {

// TODO: bring back ImPlot

class CurrentContextGuard : public ImguiContext::IGuard
{
public:
    CurrentContextGuard(std::shared_ptr<ImGuiContext> pContext /*, shared_ptr<ImPlotContext> pPlotContext*/)
      : m_pPrevContext(ImGui::GetCurrentContext())
      //, m_pPrevPlotContext(ImPlot::GetCurrentContext())
      , m_pContext(std::move(pContext))
    //, m_pPlotContext(move(pPlotContext))
    {
        ImGui::SetCurrentContext(m_pContext.get());
        // ImPlot::SetCurrentContext(m_pPlotContext.get());
    }

    ~CurrentContextGuard() override
    {
        ImGui::SetCurrentContext(m_pPrevContext);
        // ImPlot::SetCurrentContext(m_pPrevPlotContext);
    }

private:
    ImGuiContext* m_pPrevContext{};
    // ImPlotContext* m_pPrevPlotContext{};
    std::shared_ptr<ImGuiContext> m_pContext;
    // shared_ptr<ImPlotContext> m_pPlotContext;
};

}  // namespace

ImguiContext::ImguiContext()
  : m_pContext([&] {
      IMGUI_CHECKVERSION();
      auto pContext = ImGui::CreateContext();
      return std::shared_ptr<ImGuiContext>(pContext, [](auto* pC) { ImGui::DestroyContext(pC); });
  }())
//, m_pPlotContext(ImPlot::CreateContext(), [](auto* pC) { ImPlot::DestroyContext(pC); })
{
}

auto ImguiContext::makeCurrent() -> std::unique_ptr<IGuard>
{
    return std::make_unique<CurrentContextGuard>(m_pContext /*, m_pPlotContext*/);
}
