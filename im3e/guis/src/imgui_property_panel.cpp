#include "imgui_property_panel.h"

#include <im3e/utils/core/throw_utils.h>
#include <im3e/utils/imgui_utils.h>

#include <fmt/format.h>
#include <imgui.h>

using namespace im3e;

namespace {

auto getPropertyControl(
    const std::shared_ptr<IProperty>& pProperty,
    std::unordered_map<std::shared_ptr<IProperty>, std::shared_ptr<IImguiPropertyControl>>& rPropertyControlMap)
{
    std::shared_ptr<IImguiPropertyControl> pPropertyControl;
    if (auto itFind = rPropertyControlMap.find(pProperty); itFind != rPropertyControlMap.end())
    {
        pPropertyControl = itFind->second;
    }
    else
    {
        pPropertyControl = createImguiPropertyControl(pProperty);
        rPropertyControlMap.emplace(pProperty, pPropertyControl);
    }
    return pPropertyControl;
}

void drawProperty(
    const std::shared_ptr<IProperty>& pProperty,
    std::unordered_map<std::shared_ptr<IProperty>, std::shared_ptr<IImguiPropertyControl>>& pPropertyControlMap)
{
    ImGui::PushID(static_cast<int>(reinterpret_cast<intptr_t>(pProperty.get())));
    ImguiScope idScope(&ImGui::PopID);

    ImGui::TableNextRow();
    ImGui::TableSetColumnIndex(0);
    ImGui::AlignTextToFramePadding();

    if (auto pPropertyGroup = std::dynamic_pointer_cast<IPropertyGroup>(pProperty))
    {
        const auto treeNodeId = fmt::format("{}##header", pPropertyGroup->getName());
        const auto treeNodeFlags = ImGuiTreeNodeFlags_Framed | ImGuiTreeNodeFlags_DefaultOpen;
        ImguiScope treeNode(
            ImGui::TreeNodeEx(treeNodeId.c_str(), treeNodeFlags, "%s", pPropertyGroup->getName().c_str()),
            &ImGui::TreePop);

        ImGui::TableSetColumnIndex(1);

        const auto treeNode2Id = fmt::format("{}##header2", pPropertyGroup->getName());
        const auto treeNode2Flags = ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_Framed;
        ImguiScope treeNode2(ImGui::TreeNodeEx(treeNode2Id.c_str(), treeNode2Flags, " "), &ImGui::TreePop);

        if (treeNode.isOpen())
        {
            for (auto& rChildProperty : pPropertyGroup->getChildren())
            {
                drawProperty(rChildProperty, pPropertyControlMap);
            }
        }
    }
    else
    {
        auto pPropertyControl = getPropertyControl(pProperty, pPropertyControlMap);
        ImGui::Text("%s", pPropertyControl->getName().c_str());

        if (auto pValueProperty = std::dynamic_pointer_cast<IPropertyValue>(pProperty))
        {
            ImGui::SetItemTooltip("%s", pValueProperty->getDescription().c_str());
        }

        ImGui::TableSetColumnIndex(1);

        ImGui::PushItemWidth(-FLT_MIN);
        pPropertyControl->draw();
    }
}

}  // namespace

ImguiPropertyPanel::ImguiPropertyPanel(std::shared_ptr<IPropertyGroup> pPropertyGroup)
  : m_pPropertyGroup(throwIfArgNull(std::move(pPropertyGroup), "ImguiPropertyPanel requires a PropertyGroup"))
{
}

void ImguiPropertyPanel::draw(const ICommandBuffer&)
{
    constexpr auto ColumnCount = 2;
    constexpr auto TableFlags = ImGuiTableFlags_Resizable | ImGuiTableFlags_SizingStretchSame;

    const auto tableId = fmt::format("{}##table", reinterpret_cast<intptr_t>(this));
    ImguiScope table(ImGui::BeginTable(tableId.c_str(), ColumnCount, TableFlags), &ImGui::EndTable);
    if (table.isOpen())
    {
        // Assign initial weights to the columns so that the "Name" column takes less space that the "Value" column.
        ImGui::TableSetupColumn("Name", 0, 0.8840F);
        ImGui::TableSetupColumn("Value", 0, 1.1160F);

        for (auto& pProperty : m_pPropertyGroup->getChildren())
        {
            drawProperty(pProperty, m_pPropertyControlMap);
        }
    }
}

auto im3e::createImguiPropertyPanel(std::shared_ptr<IPropertyGroup> pPropertyGroup) -> std::shared_ptr<IGuiPanel>
{
    return std::make_shared<ImguiPropertyPanel>(std::move(pPropertyGroup));
}