#pragma once

#include <im3e/scene/camera.h>
#include <im3e/utils/properties/properties.h>

#include <glm/glm.hpp>

#include <memory>

namespace im3e {

class MapCameraController
{
public:
    MapCameraController(std::shared_ptr<Camera> pCamera);
    MapCameraController(const MapCameraController&) = delete;
    MapCameraController(MapCameraController&&) = delete;
    MapCameraController& operator=(const MapCameraController&) = delete;
    MapCameraController& operator=(MapCameraController&&) = delete;

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
    void zoom(float steps);

    [[nodiscard]] auto getTargetPoint() const -> glm::vec3 { return m_pTargetPoint->getValue(); }
    [[nodiscard]] auto getDistanceToTarget() const -> float { return m_pDistanceToTarget->getValue(); }
    [[nodiscard]] auto getHeading() const -> float { return m_pHeading->getValue(); }
    [[nodiscard]] auto getTilt() const -> float { return m_pTilt->getValue(); }
    [[nodiscard]] auto getProperties() const -> std::shared_ptr<IPropertyGroup> { return m_pPropertyGroup; }

private:
    void _update();

    std::shared_ptr<Camera> m_pCamera;

    std::shared_ptr<PropertyValue<glm::vec3>> m_pTargetPoint;
    std::shared_ptr<PropertyValue<float>> m_pDistanceToTarget;
    std::shared_ptr<PropertyValue<float>> m_pHeading;
    std::shared_ptr<PropertyValue<float>> m_pTilt;
    std::shared_ptr<IPropertyGroup> m_pPropertyGroup;
};

}  // namespace im3e