#pragma once

#include <glm/glm.hpp>

namespace im3e {

/// @brief Represents a ray in 3D with an origin and normalized direction.
struct Ray
{
    glm::vec3 origin;
    glm::vec3 direction;
};

}  // namespace im3e