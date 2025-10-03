#include "anari_map_camera.h"

#include <im3e/utils/core/throw_utils.h>
#include <im3e/utils/properties/properties.h>

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

auto setAnariPerspectiveParameters(ANARIDevice anDevice, ANARICamera anCamera,
                                   const PerspectiveProjection& rPerspective, const ViewTransform& rView)
{
    const auto fovY = rPerspective.getFovY();
    anariSetParameter(anDevice, anCamera, "fovy", ANARI_FLOAT32, &fovY);

    const auto aspectRatio = rPerspective.getAspectRatio();
    anariSetParameter(anDevice, anCamera, "aspect", ANARI_FLOAT32, &aspectRatio);

    const auto near = rPerspective.getNear();
    anariSetParameter(anDevice, anCamera, "near", ANARI_FLOAT32, &near);

    const auto far = rPerspective.getFar();
    anariSetParameter(anDevice, anCamera, "far", ANARI_FLOAT32, &far);

    const auto position = rView.getPosition();
    anariSetParameter(anDevice, anCamera, "position", ANARI_FLOAT32_VEC3, &position);

    const auto up = rView.getUp();
    anariSetParameter(anDevice, anCamera, "up", ANARI_FLOAT32_VEC3, &up);

    const auto direction = rView.getDirection();
    anariSetParameter(anDevice, anCamera, "direction", ANARI_FLOAT32_VEC3, &direction);
}

}  // namespace

AnariMapCamera::AnariMapCamera(std::shared_ptr<AnariDevice> pAnDevice)
  : m_pAnDevice(throwIfArgNull(std::move(pAnDevice), "ANARI Map Camera requires an ANARI Device"))
  , m_pLogger(m_pAnDevice->createLogger("AnariMapCamera"))
  , m_pAnCamera(createAnariCamera(*m_pLogger, m_pAnDevice->getHandle()))

  , m_pOnTransformChanged(std::make_shared<std::function<void()>>([this] { m_needsCommit = true; }))

  , m_perspective(m_pOnTransformChanged)
  , m_view(m_pOnTransformChanged)

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

    setAnariPerspectiveParameters(m_pAnDevice->getHandle(), m_pAnCamera.get(), m_perspective, m_view);
    anariCommitParameters(m_pAnDevice->getHandle(), m_pAnCamera.get());
}

auto AnariMapCamera::createProperties() -> std::shared_ptr<IPropertyGroup>
{
    return createPropertyGroup("Map Camera", {m_perspective.getProperties(), m_view.getProperties()});
}

void AnariMapCamera::onMouseMove(const glm::vec2& rClipOffset, const std::array<bool, 3U>& rMouseButtonsDown)
{
    if (rMouseButtonsDown[static_cast<size_t>(MouseButton::Left)])
    {
        constexpr auto HalfPi = std::numbers::pi_v<float> / 2.0F;

        const auto angleXOffset = -rClipOffset.y * HalfPi;
        const auto angleYOffset = -rClipOffset.x * HalfPi / 2.0F;

        m_angleX = std::clamp(m_angleX + angleXOffset, 0.0F, HalfPi);
        m_angleY += angleYOffset;
        _update();
    }
    else if (rMouseButtonsDown[static_cast<size_t>(MouseButton::Middle)])
    {
        const auto inverseVpMatrix = glm::inverse(m_perspective.getMatrix() * m_view.getMatrix());
        const auto sceneVec = inverseVpMatrix * glm::vec4(rClipOffset, 0.0F, 0.0F);
        const auto sceneOffset = sceneVec.xyz() * m_distanceToTarget;
        m_targetPoint.x += sceneOffset.x;
        m_targetPoint.z += sceneOffset.z;
        _update();
    }
}

void AnariMapCamera::onMouseWheel(float scrollSteps)
{
    m_distanceToTarget = m_distanceToTarget * (1.0F - scrollSteps * ScrollSensitivity);
    _update();
}

void AnariMapCamera::setAspectRatio(float aspectRatio)
{
    m_perspective.setAspectRatio(aspectRatio);
}

void AnariMapCamera::_update()
{
    const auto rotationX = glm::angleAxis(m_angleX, glm::vec3{1.0F, 0.0F, 0.0F});
    const auto rotationY = glm::angleAxis(m_angleY, glm::vec3{0.0F, 1.0F, 0.0F});
    const auto rotation = rotationY * rotationX;

    const auto direction = glm::normalize(rotation * glm::vec3{0.0F, -1.0F, 0.0F});
    const auto position = m_targetPoint + -direction * m_distanceToTarget;
    const auto up = rotation * glm::vec3{0.0F, 0.0F, -1.0F};

    m_view.setPosition(position);
    m_view.setDirection(direction, up);

    m_viewFrustum = _createViewFrustum();
    m_needsCommit = true;
}

auto AnariMapCamera::_createViewFrustum() const -> ViewFrustum
{
    return ViewFrustum{ViewFrustum::PerspectiveConfig{
        .fovY = m_perspective.getFovY(),
        .aspectRatio = m_perspective.getAspectRatio(),

        .near = m_perspective.getNear(),
        .far = m_perspective.getFar(),

        .position = m_view.getPosition(),
        .direction = m_view.getDirection(),
        .up = m_view.getUp(),
        .right = m_view.getRight(),
    }};
}
