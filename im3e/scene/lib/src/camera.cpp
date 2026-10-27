#include <im3e/scene/camera.h>

using namespace im3e;

namespace {

auto createViewFrustum(const PerspectiveProjection& rProjection, const ViewTransform& rView)
{
    return ViewFrustum{ViewFrustum::PerspectiveConfig{
        .fovY = rProjection.getFovY(),
        .aspectRatio = rProjection.getAspectRatio(),
        .near = rProjection.getNear(),
        .far = rProjection.getFar(),

        .position = rView.getPosition(),
        .direction = rView.getDirection(),
        .up = rView.getUp(),
        .right = rView.getRight(),
    }};
}

auto createInverseVpMatrix(const PerspectiveProjection& rProjection, const ViewTransform& rView)
{
    return glm::inverse(rProjection.getMatrix() * rView.getMatrix());
}

}  // namespace

Camera::Camera(std::function<void()> onChanged)
  : m_onChanged(std::move(onChanged))
  , m_projection([this] { _onChanged(); })
  , m_view([this] { _onChanged(); })
  , m_viewFrustum(createViewFrustum(m_projection, m_view))
  , m_inverseVpMatrix(createInverseVpMatrix(m_projection, m_view))
{
}

auto Camera::createProperties() -> std::shared_ptr<IPropertyGroup>
{
    return createPropertyGroup("Camera", {m_projection.getProperties(), m_view.getProperties()});
}

auto Camera::createViewRayFromNdc(const glm::vec2& rNdcPos) const -> Ray
{
    const auto tanHalfFovY = std::tan(m_projection.getFovY() * 0.5F);
    const auto tanHalfFovX = tanHalfFovY * m_projection.getAspectRatio();
    const auto viewDir = glm::vec3{rNdcPos.x * tanHalfFovX, rNdcPos.y * tanHalfFovY, -1.0F};

    return Ray{
        .origin = m_view.getPosition(),
        .direction = glm::normalize(viewDir.x * m_view.getRight() + viewDir.y * m_view.getUp() -
                                    viewDir.z * m_view.getDirection()),
    };
}

void Camera::_onChanged()
{
    m_viewFrustum = createViewFrustum(m_projection, m_view);
    m_inverseVpMatrix = createInverseVpMatrix(m_projection, m_view);
    if (m_onChanged)
    {
        m_onChanged();
    }
}