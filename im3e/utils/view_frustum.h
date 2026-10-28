#pragma once

#include <glm/glm.hpp>

#include <array>
#include <numbers>

namespace im3e {

/// @brief Class representing the view frustum of a camera.
class ViewFrustum
{
public:
    /// @brief Plane represented as (a, b, c, d) so that a point (x, y, z) is on the plane if ax + by + cz + d = 0.
    /// This means that:
    ///      - (a, b, c) is the normal to the plane
    ///      - d is formed using a point P_0 on the plane with coordinates (x_0, y_0, z_0):
    ///          d = -(ax_0 + bx_0 + cx_0)
    using Plane = glm::vec4;

    struct PerspectiveConfig
    {
        float fovY = std::numbers::pi_v<float> / 3.0F;
        float aspectRatio = 1.0F;

        float near = 0.1F;
        float far = 1000.0F;

        glm::vec3 position{0.0F, 0.0F, 0.0F};
        glm::vec3 direction{0.0F, 0.0F, -1.0F};
        glm::vec3 up{0.0F, 1.0F, 0.0F};
        glm::vec3 right{1.0F, 0.0F, 0.0F};
    };
    ViewFrustum(PerspectiveConfig config);

    /// @brief Test if a given AABB is partially or fully inside the view frustum.
    /// @param rMin Min point of the AABB
    /// @param rMax Max point of the AABB
    /// @pre rMin is assumed to have min values and rMax to have max values. No validation is performed.
    /// @return True if the given AABB is at least partially inside the current frustum.
    auto isAABBInside(const glm::vec3& rMinPoint, const glm::vec3& rMaxPoint) const -> bool;

    auto getConfig() const -> const PerspectiveConfig& { return m_config; }
    auto getNearPlane() const -> const Plane& { return m_planes[NearPlaneIdx]; }
    auto getFarPlane() const -> const Plane& { return m_planes[FarPlaneIdx]; }
    auto getTopPlane() const -> const Plane& { return m_planes[TopPlaneIdx]; }
    auto getBottomPlane() const -> const Plane& { return m_planes[BottomPlaneIdx]; }
    auto getLeftPlane() const -> const Plane& { return m_planes[LeftPlaneIdx]; }
    auto getRightPlane() const -> const Plane& { return m_planes[RightPlaneIdx]; }

private:
    PerspectiveConfig m_config;

    static constexpr size_t NearPlaneIdx{0U};
    static constexpr size_t FarPlaneIdx{1U};
    static constexpr size_t TopPlaneIdx{2U};
    static constexpr size_t BottomPlaneIdx{3U};
    static constexpr size_t LeftPlaneIdx{4U};
    static constexpr size_t RightPlaneIdx{5U};
    std::array<Plane, 6U> m_planes;
};

}  // namespace im3e