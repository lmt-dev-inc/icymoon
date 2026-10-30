#pragma once

#include "anari_device.h"

#include <im3e/utils/core/types.h>
#include <im3e/utils/loggers.h>
#include <im3e/property/properties.h>

#include <anari/anari.h>

#include <memory>
#include <string>

namespace im3e {

class AnariRenderer
{
public:
    AnariRenderer(std::shared_ptr<AnariDevice> pAnDevice);

    void commitChanges();
    auto createProperties() -> std::shared_ptr<IPropertyGroup>;

    auto getHandle() const -> ANARIRenderer { return m_pAnRenderer.get(); }

private:
    std::shared_ptr<AnariDevice> m_pAnDevice;
    std::unique_ptr<ILogger> m_pLogger;

    const std::string m_anRendererSubtype;
    std::shared_ptr<anari::api::Renderer> m_pAnRenderer;

    bool m_parametersChanged{};
};

}  // namespace im3e