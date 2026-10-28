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
  , m_pTargetPoint(std::make_shared<decltype(m_pTargetPoint)::element_type>(PropertyValueConfig<glm::vec3>{
        .name = "Target Point",
        .description = "Point the camera is targetting",
        .defaultValue = glm::vec3{3700.0F, 0.0F, 4000.0F},
        .onChange = [this](auto) { _update(); },
    }))
  , m_pDistanceToTarget(std::make_shared<decltype(m_pDistanceToTarget)::element_type>(PropertyValueConfig<float>{
        .name = "Distance to Target",
        .description = "Distance between the camera position and the target point",
        .defaultValue = 5000.0F,
        .minValue = 0.0F,
        .onChange = [this](auto) { _update(); },
    }))
  , m_pHeading(std::make_shared<decltype(m_pHeading)::element_type>(PropertyValueConfig<float>{
        .name = "Heading",
        .description = "Horizontal rotation in radians of the map around the target point."
                       "0 means the top of the screen is aligned with the north of the map.",
        .defaultValue = 0.0F,
        .minValue = -std::numbers::pi_v<float>,
        .maxValue = std::numbers::pi_v<float>,
        .onChange = [this](auto) { _update(); },
    }))
  , m_pTilt(std::make_shared<decltype(m_pTilt)::element_type>(PropertyValueConfig<float>{
        .name = "Tilt",
        .description = "Vertical angle in radians of the camera relative to the map surface."
                       "0 means the camera points straight down.",
        .defaultValue = 0.0F,
        .minValue = 0.0F,
        .maxValue = std::numbers::pi_v<float> * 70.0F / 180.0F,
        .onChange = [this](auto) { _update(); },
    }))
  , m_pPropertyGroup(
        createPropertyGroup("Map Camera Controller", {m_pTargetPoint, m_pDistanceToTarget, m_pHeading, m_pTilt}))
{
    _update();
}

void MapCameraController::pan(const glm::vec2& rNdcFrom, const glm::vec2& rNdcTo)
{
    auto targetPoint = m_pTargetPoint->getValue();
    const auto prevTarget = intersectHorizontalPlane(m_pCamera->createViewRayFromNdc(rNdcFrom), targetPoint.y);
    const auto currTarget = intersectHorizontalPlane(m_pCamera->createViewRayFromNdc(rNdcTo), targetPoint.y);
    if (prevTarget && currTarget)
    {
        targetPoint += *prevTarget - *currTarget;
        m_pTargetPoint->setValue(targetPoint);
    }
}

void MapCameraController::rotate(float headingDelta, float tiltDelta)
{
    auto heading = m_pHeading->getValue();
    heading = std::remainder(heading + headingDelta, 2.0F * std::numbers::pi_v<float>);
    m_pHeading->setValue(heading);

    m_pTilt->setValue(m_pTilt->getValue() + tiltDelta);
}

void MapCameraController::zoom(float factor)
{
    throwIfFalse<std::invalid_argument>(factor > 0.0F, "Map camera zoom factor must be greater than 0.");
    m_pDistanceToTarget->setValue(m_pDistanceToTarget->getValue() * factor);
}

void MapCameraController::_update()
{
    const auto rotationX = glm::angleAxis(m_pTilt->getValue(), glm::vec3{1.0F, 0.0F, 0.0F});
    const auto rotationY = glm::angleAxis(m_pHeading->getValue(), glm::vec3{0.0F, 1.0F, 0.0F});
    const auto rotation = rotationY * rotationX;

    const auto direction = glm::normalize(rotation * glm::vec3{0.0F, -1.0F, 0.0F});
    const auto position = m_pTargetPoint->getValue() + -direction * m_pDistanceToTarget->getValue();
    const auto up = rotation * glm::vec3{0.0F, 0.0F, -1.0F};

    auto& rView = m_pCamera->getView();
    rView.setPosition(position);
    rView.setDirection(direction, up);
}