#include "anari_map_camera.h"

#include <im3e/utils/core/throw_utils.h>
#include <im3e/property/properties.h>

#include <fmt/format.h>

using namespace im3e;

namespace {

constexpr float ScrollSensitivity = 0.1F;

// Dragging accross the full viewport width brings back to the original heading i.e. it does a full rotation (2pi):
constexpr float HeadingPerViewportWidth = 2.0f * std::numbers::pi_v<float>;

// Dragging accross half the viewport height tilts by 90° (pi/2) so a full viewport height drags by 180° (pi):
constexpr float TiltPerViewportHeight = std::numbers::pi_v<float>;

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
  , m_pCamera(createCamera([this] { m_needsCommit = true; }))
  , m_cameraController(m_pCamera)
{
    commitChanges();
}

void AnariMapCamera::commitChanges()
{
    if (!m_needsCommit)
    {
        return;
    }
    m_needsCommit = false;

    setAnariPerspectiveParameters(m_pAnDevice->getHandle(), m_pAnCamera.get(), m_pCamera->getProjection(),
                                  m_pCamera->getView());
    anariCommitParameters(m_pAnDevice->getHandle(), m_pAnCamera.get());
}

auto AnariMapCamera::createProperties() -> std::shared_ptr<IPropertyGroup>
{
    return createPropertyGroup("Map Camera", {m_cameraController.getProperties(), m_pCamera->createProperties()});
}

void AnariMapCamera::onMouseMove(const MouseMoveEvent& rEvent)
{
    if (!rEvent.mouseButtons.has(MouseButton::Left))
    {
        return;
    }

    if (rEvent.keyModifiers.has(KeyModifier::Ctrl))
    {
        // NDC spans 2 units per axis; convert both to screen-height units so they share one scale
        const auto headingDelta = -HeadingPerViewportWidth * rEvent.ndcOffset.x / 2.0F;
        const auto tiltDelta = TiltPerViewportHeight * rEvent.ndcOffset.y / 2.0F;

        m_cameraController.rotate(headingDelta, tiltDelta);
    }
    else
    {
        m_cameraController.pan(rEvent.ndcPrevPosition, rEvent.ndcCurrPosition);
    }
}

void AnariMapCamera::onMouseWheel(float scrollSteps)
{
    const auto zoomFactor = std::max(1.0F - scrollSteps * ScrollSensitivity, ScrollSensitivity);
    m_cameraController.zoom(zoomFactor);
}

void AnariMapCamera::setAspectRatio(float aspectRatio)
{
    m_pCamera->getProjection().setAspectRatio(aspectRatio);
}
