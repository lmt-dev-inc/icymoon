#include <im3e/scene/map_camera_controller.h>

#include <im3e/utils/core/throw_utils.h>
#include <im3e/utils/ray.h>

#include <glm/gtc/quaternion.hpp>

#include <numbers>

using namespace im3e;

namespace {

auto intersectHorizontalPlane(const Ray& rRay, float planeHeight) -> std::optional<glm::vec3>
{
    // Avoid intersections when the direction is up or at grazing angle to the horizontal plane:
    constexpr float MinGrazing = 1.0e-3F;
    if (rRay.direction.y > -MinGrazing)
    {
        return std::nullopt;
    }

    const auto t = (planeHeight - rRay.origin.y) / rRay.direction.y;
    return rRay.origin + t * rRay.direction;
}

}  // namespace

MapCameraController::MapCameraController(std::shared_ptr<ICamera> pCamera)
  : m_pCamera(throwIfArgNull(std::move(pCamera), "MapCameraController requires a camera"))
{
    _update();
}

void MapCameraController::pan(const glm::vec2& rNdcFrom, const glm::vec2& rNdcTo)
{
    auto targetPoint = m_targetPoint.getValue();
    const auto prevTarget = intersectHorizontalPlane(m_pCamera->createViewRayFromNdc(rNdcFrom), targetPoint.y);
    const auto currTarget = intersectHorizontalPlane(m_pCamera->createViewRayFromNdc(rNdcTo), targetPoint.y);
    if (prevTarget && currTarget)
    {
        targetPoint += *prevTarget - *currTarget;
        m_targetPoint.setValue(targetPoint);
    }
}

void MapCameraController::rotate(float headingDelta, float tiltDelta)
{
    auto heading = m_heading.getValue();
    heading = std::remainder(heading + headingDelta, 2.0F * std::numbers::pi_v<float>);
    m_heading.setValue(heading);

    m_tilt.setValue(m_tilt.getValue() + tiltDelta);
}

void MapCameraController::zoom(float factor)
{
    throwIfFalse<std::invalid_argument>(factor > 0.0F, "Map camera zoom factor must be greater than 0.");
    m_distanceToTarget.setValue(m_distanceToTarget.getValue() * factor);
}

void MapCameraController::_update()
{
    const auto rotationX = glm::angleAxis(m_tilt.getValue(), glm::vec3{1.0F, 0.0F, 0.0F});
    const auto rotationY = glm::angleAxis(m_heading.getValue(), glm::vec3{0.0F, 1.0F, 0.0F});
    const auto rotation = rotationY * rotationX;

    const auto direction = glm::normalize(rotation * glm::vec3{0.0F, -1.0F, 0.0F});
    const auto position = m_targetPoint.getValue() + -direction * m_distanceToTarget.getValue();
    const auto up = rotation * glm::vec3{0.0F, 0.0F, -1.0F};

    auto& rView = m_pCamera->getView();
    rView.setPosition(position);
    rView.setDirection(direction, up);
}