#include "anari_map_camera.h"

#include <im3e/utils/core/throw_utils.h>

#include <fmt/format.h>
#include <glm/gtc/quaternion.hpp>

using namespace im3e;

namespace {

constexpr float ScrollSensitivity = 0.1F;

auto createAnariCamera(const ILogger& rLogger, ANARIDevice anDevice)
{
    auto anCamera = anariNewCamera(anDevice, "perspective");
    return std::unique_ptr<anari::api::Camera, std::function<void(anari::api::Camera*)>>(
        anCamera, [anDevice, pLogger = &rLogger](auto* anCamera) {
            anariRelease(anDevice, anCamera);
            pLogger->debug("Released camera");
        });
}

}  // namespace

AnariMapCamera::AnariMapCamera(std::shared_ptr<AnariDevice> pAnDevice)
  : m_pAnDevice(throwIfArgNull(std::move(pAnDevice), "ANARI Map Camera requires an ANARI Device"))
  , m_pLogger(m_pAnDevice->createLogger("AnariMapCamera"))
  , m_pAnCamera(createAnariCamera(*m_pLogger, m_pAnDevice->getHandle()))
  , m_viewFrustum(_createViewFrustum())
{
    _update();  // calculate initial view state
    commitChanges();
}

void AnariMapCamera::commitChanges()
{
    if (!m_needsCommit)
    {
        return;
    }
    m_needsCommit = false;

    m_perspective.setCameraParameters(m_pAnDevice->getHandle(), m_pAnCamera.get());
    m_view.setCameraParameters(m_pAnDevice->getHandle(), m_pAnCamera.get());
    anariCommitParameters(m_pAnDevice->getHandle(), m_pAnCamera.get());
}

void AnariMapCamera::onMouseMove(const glm::vec2& rClipOffset, const std::array<bool, 3U>& rMouseButtonsDown)
{
    if (rMouseButtonsDown[static_cast<size_t>(MouseButton::Left)])
    {
        constexpr auto HalfPi = std::numbers::pi_v<float> / 2.0F;

        const auto angleXOffset = -rClipOffset.y * HalfPi;
        const auto angleYOffset = -rClipOffset.x * HalfPi / 2.0F;

        m_view.angleX = std::clamp(m_view.angleX + angleXOffset, 0.0F, HalfPi);
        m_view.angleY += angleYOffset;
        _update();
    }
    else if (rMouseButtonsDown[static_cast<size_t>(MouseButton::Middle)])
    {
        const auto inverseVpMatrix = glm::inverse(m_perspective.generateMatrix() * m_view.generateMatrix());
        const auto sceneVec = inverseVpMatrix * glm::vec4(rClipOffset, 0.0F, 0.0F);
        const auto sceneOffset = sceneVec.xyz() * m_view.distanceToTarget;
        m_view.targetPoint.x += sceneOffset.x;
        m_view.targetPoint.z += sceneOffset.z;
        _update();
    }
}

void AnariMapCamera::onMouseWheel(float scrollSteps)
{
    m_view.distanceToTarget = m_view.distanceToTarget * (1.0F - scrollSteps * ScrollSensitivity);
    _update();
}

void AnariMapCamera::setAspectRatio(float aspectRatio)
{
    if (m_perspective.aspectRatio == aspectRatio)
    {
        return;
    }
    m_perspective.aspectRatio = aspectRatio;
    m_needsCommit = true;
}

void AnariMapCamera::_update()
{
    m_view.update();

    // TODO: replace with view frustum update
    m_viewFrustum = _createViewFrustum();
    /*const auto& rPosition = m_view.getPosition();
    const auto& rDirection = m_view.getDirection();
    const auto& rUp = m_view.getUp();
    const auto& rRight = m_view.getRight();

    // TODO: figure out the rest including:
    // - what is d for near and far planes
    // - what is d for top plane
    // - calculate the rest of the planes
    // - how can this be tested?
    m_viewFrustum.near = glm::vec4{rDirection, rPosition + rDirection * m_perspective.near};
    m_viewFrustum.far = glm::vec4{-rDirection, rPosition + rDirection * m_perspective.far};

    const auto topNormal = -glm::angleAxis((m_perspective.fovY - std::numbers::pi_v<float>) / 2.0F, rRight) *
                           rDirection;
    m_viewFrustum.top = glm::vec4{topNormal, 0.0F};*/

    m_needsCommit = true;
}

auto AnariMapCamera::_createViewFrustum() const -> ViewFrustum
{
    return ViewFrustum{ViewFrustum::PerspectiveConfig{
        .fovY = m_perspective.fovY,
        .aspectRatio = m_perspective.aspectRatio,

        .near = m_perspective.near,
        .far = m_perspective.far,

        .position = m_view.getPosition(),
        .direction = m_view.getDirection(),
        .up = m_view.getUp(),
        .right = m_view.getRight(),
    }};
}

void AnariMapCamera::PerspectiveState::setCameraParameters(ANARIDevice anDevice, ANARICamera anCamera) const
{
    anariSetParameter(anDevice, anCamera, "fovy", ANARI_FLOAT32, &fovY);
    anariSetParameter(anDevice, anCamera, "aspect", ANARI_FLOAT32, &aspectRatio);
    anariSetParameter(anDevice, anCamera, "near", ANARI_FLOAT32, &near);
    anariSetParameter(anDevice, anCamera, "far", ANARI_FLOAT32, &far);
}

auto AnariMapCamera::PerspectiveState::generateMatrix() const -> glm::mat4
{
    return glm::perspective(fovY, aspectRatio, near, far);
}

void AnariMapCamera::ViewState::update()
{
    const auto rotationX = glm::angleAxis(angleX, glm::vec3{1.0F, 0.0F, 0.0F});
    const auto rotationY = glm::angleAxis(angleY, glm::vec3{0.0F, 1.0F, 0.0F});
    const auto rotation = rotationY * rotationX;

    m_direction = glm::normalize(rotation * glm::vec3{0.0F, -1.0F, 0.0F});
    m_position = targetPoint + -m_direction * distanceToTarget;
    m_up = rotation * glm::vec3{0.0F, 0.0F, -1.0F};
    m_right = glm::normalize(glm::cross(m_direction, m_up));
    m_up = glm::normalize(glm::cross(m_right, m_direction));
}

void AnariMapCamera::ViewState::setCameraParameters(ANARIDevice anDevice, ANARICamera anCamera) const
{
    anariSetParameter(anDevice, anCamera, "position", ANARI_FLOAT32_VEC3, &m_position);
    anariSetParameter(anDevice, anCamera, "up", ANARI_FLOAT32_VEC3, &m_up);
    anariSetParameter(anDevice, anCamera, "direction", ANARI_FLOAT32_VEC3, &m_direction);
}

auto AnariMapCamera::ViewState::generateMatrix() const -> glm::mat4
{
    return glm::lookAt(m_direction, m_position, m_up);
}