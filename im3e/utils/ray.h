#pragma once

#include <glm/glm.hpp>

namespace im3e {

/// @brief Represents a ray in 3D with an origin and normalized direction.
struct Ray
{
    glm::vec3 origin;
    glm::vec3 direction;

    /// @brief Create a view ray from a NDC position and inverse View-Projection matrix.
    /// @param rNdcPos Target position in NDC coordinates.
    /// @param rInverseVpMatrix Inverse View-Projection matrix of the current view.
    /// @return A view ray going from the origin of the view to the given rNdcPos in scene coordinates.
    static auto createViewRayFromNdc(const glm::vec2& rNdcPos, const glm::mat4& rInverseVpMatrix) -> Ray;
};

}  // namespace im3e