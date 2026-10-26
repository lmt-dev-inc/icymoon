#include "ray.h"

using namespace im3e;

auto Ray::createViewRayFromNdc(const glm::vec2& rNdcPos, const glm::mat4& rInverseVpMatrix) -> Ray
{
    auto nearPos = rInverseVpMatrix * glm::vec4(rNdcPos, 0.0F, 1.0F);
    nearPos /= nearPos.w;
    auto farPos = rInverseVpMatrix * glm::vec4(rNdcPos, 1.0F, 1.0F);
    farPos /= farPos.w;

    return Ray{
        .origin = nearPos,
        .direction = glm::normalize(farPos - nearPos),
    };
}
