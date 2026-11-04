#pragma once

#include <im3e/property/property.h>
#include <im3e/property/property_group.h>
#include <im3e/scene/camera.h>

#include <glm/glm.hpp>

#include <memory>
#include <numbers>

namespace im3e {

class MapCameraController
{
public:
    MapCameraController(std::shared_ptr<ICamera> pCamera);

    /// @brief Pans the view so that the point on the map under rNdcFrom remains under rNdcTo after.
    /// @param rNdcFrom Starting point of the pan in NDC coordinates.
    /// @param rNdcTo End point of the pan in NDC coordinates.
    void pan(const glm::vec2& rNdcFrom, const glm::vec2& rNdcTo);

    /// @brief Rotates the camera by adding the given heading and tilt values.
    /// @param headingDelta Value to add to the current heading in radians.
    /// @param tiltDelta Value to add to the current tilt in radians.
    void rotate(float headingDelta, float tiltDelta);

    /// @brief Move the camera closer or further away from the target point.
    /// @param factor Factor to apply to the current distance to the target point. Must be greater than 0.
    void zoom(float factor);

    [[nodiscard]] auto getTargetPoint() const -> glm::vec3 { return m_targetPoint.getValue(); }
    [[nodiscard]] auto getDistanceToTarget() const -> float { return m_distanceToTarget.getValue(); }
    [[nodiscard]] auto getHeading() const -> float { return m_heading.getValue(); }
    [[nodiscard]] auto getTilt() const -> float { return m_tilt.getValue(); }
    [[nodiscard]] auto getProperties() -> std::weak_ptr<IPropertyGroup> { return m_group.getInterface(); }
    [[nodiscard]] auto getProperties() const -> std::weak_ptr<const IPropertyGroup> { return m_group.getInterface(); }

private:
    void _update();

    std::shared_ptr<ICamera> m_pCamera;

    Property<glm::vec3> m_targetPoint{{
        .name = "Target Point",
        .description = "Point the camera is targetting",
        .defaultValue = glm::vec3{3700.0F, 0.0F, 4000.0F},
    }};
    Property<float> m_distanceToTarget{{
        .name = "Distance to Target",
        .description = "Distance between the camera position and the target point",
        .defaultValue = 5000.0F,
        .minValue = 0.0F,
    }};
    Property<float> m_heading{{
        .name = "Heading",
        .description = "Horizontal rotation in radians of the map around the target point."
                       "0 means the top of the screen is aligned with the north of the map.",
        .defaultValue = 0.0F,
        .minValue = -std::numbers::pi_v<float>,
        .maxValue = std::numbers::pi_v<float>,
    }};
    Property<float> m_tilt{{
        .name = "Tilt",
        .description = "Vertical angle in radians of the camera relative to the map surface."
                       "0 means the camera points straight down.",
        .defaultValue = 0.0F,
        .minValue = 0.0F,
        .maxValue = std::numbers::pi_v<float> * 70.0F / 180.0F,
    }};
    PropertyGroup m_group{
        {
            .name = "Map Camera Controller",
            .description = "Group of properties controlling a camera over a map",
        },
        {
            m_targetPoint.getInterface(),
            m_distanceToTarget.getInterface(),
            m_heading.getInterface(),
            m_tilt.getInterface(),
        },
        [this] { _update(); },
    };
};

}  // namespace im3e