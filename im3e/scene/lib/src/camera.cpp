#include <im3e/scene/camera.h>

using namespace im3e;

namespace {

auto createInverseVpMatrix(const PerspectiveProjection& rProjection, const ViewTransform& rView)
{
    return glm::inverse(rProjection.getMatrix() * rView.getMatrix());
}

}  // namespace

namespace im3e {

class Camera : public ICamera
{
public:
    explicit Camera(std::function<void()> onChanged);
    Camera(const Camera&) = delete;
    Camera(Camera&&) = delete;
    Camera& operator=(const Camera&) = delete;
    Camera& operator=(Camera&&) = delete;

    /// @brief Create a view ray from a NDC position.
    /// @param rNdcPos Target position in NDC coordinates.
    /// @return A view ray going from the origin of the camera to the given rNdcPos in scene coordinates.
    auto createViewRayFromNdc(const glm::vec2& rNdcPos) const -> Ray override;

    auto createProperties() -> std::shared_ptr<IPropertyGroup> override;

    auto getProjection() -> PerspectiveProjection& override { return m_projection; }
    auto getProjection() const -> const PerspectiveProjection& override { return m_projection; }
    auto getView() -> ViewTransform& override { return m_view; }
    auto getView() const -> const ViewTransform& override { return m_view; }
    auto getViewFrustum() const -> const ViewFrustum& override { return m_viewFrustum; }

private:
    void _onChanged();

    std::function<void()> m_onChanged;

    PerspectiveProjection m_projection;
    ViewTransform m_view;

    ViewFrustum m_viewFrustum;
    glm::mat4 m_inverseVpMatrix;
};

}  // namespace im3e

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

auto im3e::createViewFrustum(const PerspectiveProjection& rProjection, const ViewTransform& rView) -> ViewFrustum
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

auto im3e::createCamera(std::function<void()> onChanged) -> std::shared_ptr<ICamera>
{
    return std::make_shared<Camera>(std::move(onChanged));
}