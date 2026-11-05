#pragma once

#include "anari_device.h"

#include <im3e/property/property_group.h>
#include <im3e/property/property_interface.h>
#include <im3e/utils/loggers.h>

#include <anari/anari.h>

#include <memory>
#include <string>
#include <vector>

namespace im3e {

class AnariRenderer
{
public:
    AnariRenderer(std::shared_ptr<AnariDevice> pAnDevice);

    void commitChanges();

    auto getProperties() -> std::weak_ptr<IPropertyGroup> { return m_propertyGroup.getInterface(); }
    auto getHandle() const -> ANARIRenderer { return m_pAnRenderer.get(); }

private:
    std::shared_ptr<AnariDevice> m_pAnDevice;
    std::unique_ptr<ILogger> m_pLogger;

    const std::string m_anRendererSubtype;
    std::shared_ptr<anari::api::Renderer> m_pAnRenderer;

    bool m_parametersChanged{};

    std::vector<std::unique_ptr<IPropertyWrapper>> m_properties;
    PropertyGroup m_propertyGroup;
};

}  // namespace im3e
