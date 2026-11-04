#include "imgui_property_panel.h"

#include <im3e/utils/core/throw_utils.h>
#include <im3e/utils/imgui_utils.h>

#include <fmt/format.h>
#include <imgui.h>

using namespace im3e;

namespace {

auto getPropertyControl(const std::shared_ptr<IProperty>& pProperty,
                        ImguiPropertyPanel::PropertyControlMap& rPropertyControlMap) -> IImguiPropertyControl&
{
    auto itFind = rPropertyControlMap.find(pProperty);
    if (itFind == rPropertyControlMap.end())
    {
        itFind = rPropertyControlMap.emplace(pProperty, createImguiPropertyControl(*pProperty)).first;
    }
    return *itFind->second;
}

void drawProperty(const std::shared_ptr<IPropertyNode>& pNode,
                  ImguiPropertyPanel::PropertyControlMap& rPropertyControlMap)
{
    ImGui::PushID(static_cast<int>(reinterpret_cast<intptr_t>(pNode.get())));
    ImguiScope idScope(&ImGui::PopID);

    ImGui::TableNextRow();
    ImGui::TableSetColumnIndex(0);
    ImGui::AlignTextToFramePadding();

    if (auto pPropertyGroup = std::dynamic_pointer_cast<IPropertyGroup>(pNode))
    {
        const auto treeNodeId = fmt::format("{}##header", pPropertyGroup->getName());
        const auto treeNodeFlags = ImGuiTreeNodeFlags_Framed | ImGuiTreeNodeFlags_DefaultOpen;
        ImguiScope treeNode(
            ImGui::TreeNodeEx(treeNodeId.c_str(), treeNodeFlags, "%s", pPropertyGroup->getName().c_str()),
            &ImGui::TreePop);
        ImGui::SetItemTooltip("%s", pPropertyGroup->getDescription().c_str());

        ImGui::TableSetColumnIndex(1);

        const auto treeNode2Id = fmt::format("{}##header2", pPropertyGroup->getName());
        const auto treeNode2Flags = ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_Framed;
        ImguiScope treeNode2(ImGui::TreeNodeEx(treeNode2Id.c_str(), treeNode2Flags, " "), &ImGui::TreePop);

        if (treeNode.isOpen())
        {
            for (const auto& rpChild : pPropertyGroup->getChildren())
            {
                if (auto pChild = rpChild.lock())
                {
                    drawProperty(pChild, rPropertyControlMap);
                }
            }
        }
    }
    else if (auto pProperty = std::dynamic_pointer_cast<IProperty>(pNode))
    {
        auto& rPropertyControl = getPropertyControl(pProperty, rPropertyControlMap);
        ImGui::Text("%s", rPropertyControl.getName().c_str());
        ImGui::SetItemTooltip("%s", pProperty->getDescription().c_str());

        ImGui::TableSetColumnIndex(1);

        ImGui::PushItemWidth(-FLT_MIN);
        rPropertyControl.draw(*pProperty);
    }
}

auto getGroupName(const std::weak_ptr<IPropertyGroup>& rpPropertyGroup) -> std::string
{
    auto pPropertyGroup = rpPropertyGroup.lock();
    throwIfArgNull(pPropertyGroup, "ImguiPropertyPanel requires a PropertyGroup");
    return pPropertyGroup->getName();
}

}  // namespace

ImguiPropertyPanel::ImguiPropertyPanel(std::weak_ptr<IPropertyGroup> pPropertyGroup)
  : m_pPropertyGroup(std::move(pPropertyGroup))
  , m_name(getGroupName(m_pPropertyGroup))
{
}

void ImguiPropertyPanel::draw(const ICommandBuffer&)
{
    // Controls of destroyed properties are no longer needed:
    std::erase_if(m_pPropertyControlMap, [](const auto& rEntry) { return rEntry.first.expired(); });

    auto pPropertyGroup = m_pPropertyGroup.lock();
    if (!pPropertyGroup)
    {
        return;
    }

    constexpr auto ColumnCount = 2;
    constexpr auto TableFlags = ImGuiTableFlags_Resizable | ImGuiTableFlags_SizingStretchSame;

    const auto tableId = fmt::format("{}##table", reinterpret_cast<intptr_t>(this));
    ImguiScope table(ImGui::BeginTable(tableId.c_str(), ColumnCount, TableFlags), &ImGui::EndTable);
    if (table.isOpen())
    {
        // Assign initial weights to the columns so that the "Name" column takes less space that the "Value" column.
        ImGui::TableSetupColumn("Name", 0, 0.8840F);
        ImGui::TableSetupColumn("Value", 0, 1.1160F);

        for (const auto& rpChild : pPropertyGroup->getChildren())
        {
            if (auto pChild = rpChild.lock())
            {
                drawProperty(pChild, m_pPropertyControlMap);
            }
        }
    }
}

auto im3e::createImguiPropertyPanel(std::weak_ptr<IPropertyGroup> pPropertyGroup) -> std::shared_ptr<IGuiPanel>
{
    return std::make_shared<ImguiPropertyPanel>(std::move(pPropertyGroup));
}
