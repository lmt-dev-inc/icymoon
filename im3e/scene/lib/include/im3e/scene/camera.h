#pragma once

#include <im3e/scene/perspective_projection.h>
#include <im3e/scene/view_transform.h>
#include <im3e/utils/ray.h>
#include <im3e/utils/view_frustum.h>

#include <glm/glm.hpp>

#include <functional>
#include <memory>

namespace im3e {

class ICamera
{
public:
    virtual ~ICamera() = default;

    /// @brief Create a view ray from a NDC position.
    /// @param rNdcPos Target position in NDC coordinates.
    /// @return A view ray going from the origin of the camera to the given rNdcPos in scene coordinates.
    virtual auto createViewRayFromNdc(const glm::vec2& rNdcPos) const -> Ray = 0;

    virtual auto createProperties() -> std::shared_ptr<IPropertyGroup> = 0;

    virtual auto getProjection() -> PerspectiveProjection& = 0;
    virtual auto getProjection() const -> const PerspectiveProjection& = 0;
    virtual auto getView() -> ViewTransform& = 0;
    virtual auto getView() const -> const ViewTransform& = 0;
    virtual auto getViewFrustum() const -> const ViewFrustum& = 0;
};

auto createViewFrustum(const PerspectiveProjection& rProjection, const ViewTransform& rView) -> ViewFrustum;
auto createCamera(std::function<void()> onChanged = {}) -> std::shared_ptr<ICamera>;

}  // namespace im3e