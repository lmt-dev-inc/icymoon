#pragma once

#include "guis.h"
#include "imgui_property_control.h"

namespace im3e {

class ImguiPropertyPanel : public IGuiPanel
{
public:
    ImguiPropertyPanel(std::shared_ptr<ILegacyPropertyGroup> pPropertyGroup);

    void draw(const ICommandBuffer& rCommandBuffer) override;

    auto getName() const -> std::string override { return std::string{m_pPropertyGroup->getName()}; }

private:
    std::shared_ptr<ILegacyPropertyGroup> m_pPropertyGroup;
    std::unordered_map<std::shared_ptr<ILegacyProperty>, std::shared_ptr<IImguiPropertyControl>> m_pPropertyControlMap;
};

}  // namespace im3e