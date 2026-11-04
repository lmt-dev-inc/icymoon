#pragma once

#include "anari.h"
#include "anari_device.h"
#include "anari_instance_set.h"

#include <im3e/property/property.h>
#include <im3e/property/property_group.h>
#include <im3e/utils/core/types.h>
#include <im3e/utils/transform.h>

#include <string>

namespace im3e {

class AnariPlane : public IAnariObject
{
public:
    AnariPlane(std::string_view name, std::shared_ptr<AnariDevice> pAnDevice, AnariInstanceSet& rInstanceSet);
    ~AnariPlane();

    void commitChanges();

    auto getProperties() -> std::weak_ptr<IPropertyGroup> override { return m_properties.getInterface(); }
    auto getInstance() const -> ANARIInstance { return m_pAnInstance.get(); }

private:
    const std::string m_name;
    std::shared_ptr<AnariDevice> m_pAnDevice;
    AnariInstanceSet& m_rInstanceSet;

    std::unique_ptr<ILogger> m_pLogger;

    Transform m_transform;
    bool m_transformChanged{};

    UniquePtrWithDeleter<anari::api::Geometry> m_pAnGeometry;
    UniquePtrWithDeleter<anari::api::Material> m_pAnMaterial;
    UniquePtrWithDeleter<anari::api::Surface> m_pAnSurface;
    UniquePtrWithDeleter<anari::api::Group> m_pAnGroup;
    UniquePtrWithDeleter<anari::api::Instance> m_pAnInstance;

    Property<glm::vec3> m_scale{
        {
            .name = "Scale",
            .description = "Applies a scale to the plane",
            .defaultValue = m_transform.getScale(),
        },
        [this] {
            m_transform.setScale(m_scale.getValue());
            m_transformChanged = true;
        },
    };
    PropertyGroup m_properties;
};

}  // namespace im3e