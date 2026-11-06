#pragma once

#include "guis.h"
#include "imgui_property_control.h"

#include <map>
#include <memory>
#include <string>

namespace im3e {

class ImguiPropertyPanel : public IGuiPanel
{
public:
    using PropertyControlMap =
        std::map<std::weak_ptr<IPropertyNode>, std::unique_ptr<IImguiPropertyControl>, std::owner_less<>>;

    ImguiPropertyPanel(std::weak_ptr<IPropertyGroupAccessor> pPropertyGroup);

    void draw(const ICommandBuffer& rCommandBuffer) override;

    auto getName() const -> std::string override { return m_name; }

private:
    std::weak_ptr<IPropertyGroupAccessor> m_pPropertyGroup;
    const std::string m_name;
    PropertyControlMap m_pPropertyControlMap;
};

}  // namespace im3e
