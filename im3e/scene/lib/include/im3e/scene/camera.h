#pragma once

#include <im3e/scene/perspective_projection.h>
#include <im3e/scene/view_transform.h>
#include <im3e/utils/ray.h>
#include <im3e/utils/view_frustum.h>

#include <glm/glm.hpp>

#include <functional>

namespace im3e {

class Camera
{
public:
    explicit Camera(std::function<void()> onChanged = {});
    Camera(const Camera&) = delete;
    Camera(Camera&&) = delete;
    Camera& operator=(const Camera&) = delete;
    Camera& operator=(Camera&&) = delete;

    /// @brief Create a view ray from a NDC position.
    /// @param rNdcPos Target position in NDC coordinates.
    /// @return A view ray going from the origin of the camera to the given rNdcPos in scene coordinates.
    auto createViewRayFromNdc(const glm::vec2& rNdcPos) const -> Ray;

    auto createProperties() -> std::shared_ptr<IPropertyGroup>;

    auto getProjection() -> PerspectiveProjection& { return m_projection; }
    auto getProjection() const -> const PerspectiveProjection& { return m_projection; }
    auto getView() -> ViewTransform& { return m_view; }
    auto getView() const -> const ViewTransform& { return m_view; }
    auto getViewFrustum() const -> const ViewFrustum& { return m_viewFrustum; }

private:
    void _onChanged();

    std::function<void()> m_onChanged;

    PerspectiveProjection m_projection;
    ViewTransform m_view;

    ViewFrustum m_viewFrustum;
    glm::mat4 m_inverseVpMatrix;
};

}  // namespace im3e