#pragma once

#include <im3e/property/property.h>
#include <im3e/property/property_group.h>

#include <glm/glm.hpp>

#include <functional>
#include <memory>
#include <numbers>

namespace im3e {

class PerspectiveProjection
{
public:
    PerspectiveProjection(std::function<void()> onChanged);

    void setFovY(float fovY) { m_fovY.setValue(fovY); }
    void setAspectRatio(float aspectRatio) { m_aspectRatio.setValue(aspectRatio); }
    void setNear(float near) { m_near.setValue(near); }
    void setFar(float far) { m_far.setValue(far); }

    [[nodiscard]] auto getFovY() const -> float { return m_fovY.getValue(); }
    [[nodiscard]] auto getAspectRatio() const -> float { return m_aspectRatio.getValue(); }
    [[nodiscard]] auto getNear() const -> float { return m_near.getValue(); }
    [[nodiscard]] auto getFar() const -> float { return m_far.getValue(); }
    [[nodiscard]] auto getProperties() -> std::weak_ptr<IPropertyGroupAccessor> { return m_group.getAccessor(); }
    [[nodiscard]] auto getProperties() const -> std::weak_ptr<const IPropertyGroupAccessor>
    {
        return m_group.getAccessor();
    }
    [[nodiscard]] auto getMatrix() const -> const glm::mat4&;

private:
    void _onChanged();

    std::function<void()> m_onChanged;

    Property<float> m_fovY{{
        .name = "FOV Y",
        .description = "Vertical Field of View",
        .defaultValue = std::numbers::pi_v<float> / 3.0F,
        .minValue = std::numbers::pi_v<float> / 180.0F,
        .maxValue = std::numbers::pi_v<float> * 179.0F / 180.0F,
    }};
    Property<float> m_aspectRatio{{
        .name = "Aspect Ratio",
        .description = "Aspect Ratio of the perspective projection",
        .defaultValue = 1.0F,
        .minValue = 0.0F,
        .isReadOnly = true,
    }};
    Property<float> m_near{{
        .name = "Near",
        .description = "Distance from the eye to the near plane of the perspective projection",
        .defaultValue = 0.1F,
        .minValue = std::numeric_limits<float>::epsilon(),
    }};
    Property<float> m_far{{
        .name = "Far",
        .description = "Distance from the eye to the far plane of the perspective projection",
        .defaultValue = 10'000.0F,
        .minValue = std::numeric_limits<float>::epsilon(),
    }};
    PropertyGroup m_group{{
                              .name = "Perspective Projection",
                              .description = "Group of properties representing a perspective projection",
                          },
                          {
                              m_fovY.getAccessor(),
                              m_aspectRatio.getAccessor(),
                              m_near.getAccessor(),
                              m_far.getAccessor(),
                          },
                          [this] { _onChanged(); }};

    mutable bool m_matrixDirty = true;
    mutable glm::mat4 m_matrix;
};

}  // namespace im3e