#include "imgui_workspace.h"

#include "guis.h"

#include <im3e/utils/imgui_utils.h>

#include <fmt/format.h>
#include <imgui_internal.h>

#include <algorithm>

using namespace im3e;

namespace {

auto convertLocationToImGuiDir(IGuiWorkspace::Location location) -> ImGuiDir
{
    switch (location)
    {
        case IGuiWorkspace::Location::Top: return ImGuiDir_Up;
        case IGuiWorkspace::Location::Bottom: return ImGuiDir_Down;
        case IGuiWorkspace::Location::Center: return ImGuiDir_None;
        case IGuiWorkspace::Location::Left: return ImGuiDir_Left;
        case IGuiWorkspace::Location::Right: return ImGuiDir_Right;
    }
    return ImGuiDir_None;
}

void resetWorkspace(ImGuiID dockSpaceId, ImGuiDockNodeFlags dockspaceFlags, ImguiWorkspacePanelInfo& rCenterPanel,
                    std::vector<ImguiWorkspacePanelInfo>& rPanelInfos)
{
    ImGui::DockBuilderRemoveNode(dockSpaceId);  // clear any previous layout
    ImGui::DockBuilderAddNode(dockSpaceId, dockspaceFlags | ImGuiDockNodeFlags_DockSpace);
    ImGui::DockBuilderSetNodeSize(dockSpaceId, ImGui::GetMainViewport()->Size);

    for (auto& rPanelInfo : rPanelInfos)
    {
        rPanelInfo.dockSpaceId = ImGui::DockBuilderSplitNode(
            dockSpaceId, convertLocationToImGuiDir(rPanelInfo.location), rPanelInfo.fraction, nullptr, &dockSpaceId);
    }

    rCenterPanel.dockSpaceId = dockSpaceId;

    // We now dock our windows into the docking node we made above:
    if (rCenterPanel.pPanel)
    {
        ImGui::DockBuilderDockWindow(rCenterPanel.pPanel->getName().c_str(), rCenterPanel.dockSpaceId);
    }
    for (auto& rPanelInfo : rPanelInfos)
    {
        ImGui::DockBuilderDockWindow(rPanelInfo.pPanel->getName().c_str(), rPanelInfo.dockSpaceId);
    }

    ImGui::DockBuilderFinish(dockSpaceId);
}

}  // namespace

ImguiWorkspace::ImguiWorkspace(std::string_view name)
  : m_name(name)
{
}

void ImguiWorkspace::draw(const ICommandBuffer& rCommandBuffer)
{
    if (ImguiScope menuBarScope(ImGui::BeginMainMenuBar(), &ImGui::EndMainMenuBar); menuBarScope.isOpen())
    {
        if (ImguiScope viewScope(ImGui::BeginMenu("View"), &ImGui::EndMenu); viewScope.isOpen())
        {
            ImGui::SeparatorText("Demos");
            ImGui::MenuItem("ImGui Demo", "Alt+F", &m_imguiDemoVisible);
        }
    }
    {
        auto* pViewport = ImGui::GetMainViewport();
        ImGui::SetNextWindowPos(pViewport->Pos);
        ImGui::SetNextWindowSize(pViewport->Size);
        ImGui::SetNextWindowViewport(pViewport->ID);

        ImGuiWindowFlags windowFlags = ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoTitleBar |
                                       ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoResize |
                                       ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoMove |
                                       ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus |
                                       ImGuiWindowFlags_NoBackground;

        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0F);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0F);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0F, 0.0F));

        ImguiScope windowScope(ImGui::Begin(m_name.c_str(), nullptr, windowFlags), &ImGui::End, true);

        ImGui::PopStyleVar(3);

        const auto dockSpaceId = ImGui::GetID(fmt::format("dockspace_{}", reinterpret_cast<intptr_t>(this)).c_str());
        const auto dockSpaceFlags = ImGuiDockNodeFlags_PassthruCentralNode;
        ImGui::DockSpace(dockSpaceId, ImVec2(0.0F, 0.0F), dockSpaceFlags);

        if (!m_workspaceInitialized)
        {
            resetWorkspace(dockSpaceId, dockSpaceFlags, m_centerPanel, m_panelInfos);
            m_workspaceInitialized = true;
        }

        if (ImGui::Shortcut(ImGuiMod_Alt | ImGuiKey_F))
        {
            m_imguiDemoVisible = !m_imguiDemoVisible;
        }
    }

    if (m_imguiDemoVisible)
    {
        ImGui::ShowDemoWindow();
    }

    if (auto pCenterPanel = m_centerPanel.pPanel)
    {
        if (ImguiScope panelScope(ImGui::Begin(pCenterPanel->getName().c_str()), &ImGui::End, true);
            panelScope.isOpen())
        {
            pCenterPanel->draw(rCommandBuffer);
        }
    }

    for (auto& rPanelInfo : m_panelInfos)
    {
        if (ImguiScope panelScope(ImGui::Begin(rPanelInfo.pPanel->getName().c_str()), &ImGui::End, true);
            panelScope.isOpen())
        {
            rPanelInfo.pPanel->draw(rCommandBuffer);
        }
    }
}

void ImguiWorkspace::addPanel(Location location, std::shared_ptr<IGuiPanel> pPanel, float fraction)
{
    ImguiWorkspacePanelInfo panelInfo{
        .location = location,
        .pPanel = throwIfArgNull(std::move(pPanel), "Null panel provided to ImGui workspace"),
        .fraction = fraction,
    };

    if (m_windowSize.width != 0U || m_windowSize.height != 0U)
    {
        panelInfo.pPanel->onWindowResized(m_windowSize, m_windowFormat, m_frameInFlightCount);
    }

    if (location == Location::Center)
    {
        m_centerPanel = std::move(panelInfo);
    }
    else
    {
        m_panelInfos.emplace_back(std::move(panelInfo));
    }

    m_workspaceInitialized = false;  // need to reset the workspace to add the new panel
}

void ImguiWorkspace::onWindowResized(const VkExtent2D& rVkWindowSize, VkFormat vkFormat, uint32_t frameInFlightCount)
{
    m_windowSize = rVkWindowSize;
    m_windowFormat = vkFormat;
    m_frameInFlightCount = frameInFlightCount;
    if (m_centerPanel.pPanel)
    {
        m_centerPanel.pPanel->onWindowResized(m_windowSize, m_windowFormat, m_frameInFlightCount);
    }
    std::ranges::for_each(m_panelInfos, [this](auto& rPanelInfo) {
        rPanelInfo.pPanel->onWindowResized(m_windowSize, m_windowFormat, m_frameInFlightCount);
    });
}

auto im3e::createImguiWorkspace(std::string_view name) -> std::shared_ptr<IGuiWorkspace>
{
    return std::make_shared<ImguiWorkspace>(name);
}
