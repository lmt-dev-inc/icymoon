#include "camera_transforms.h"

#include <glm/gtc/matrix_transform.hpp>

using namespace im3e;

namespace {

void callIfExists(const std::weak_ptr<std::function<void()>>& rpOnChanged)
{
    if (auto pOnChanged = rpOnChanged.lock())
    {
        (*pOnChanged)();
    }
}

}  // namespace

PerspectiveProjection::PerspectiveProjection(std::weak_ptr<std::function<void()>> pOnChanged)
  : m_pOnChanged(std::move(pOnChanged))
  , m_pFovY(std::make_shared<decltype(m_pFovY)::element_type>(PropertyValueConfig<float>{
        .name = "FOV Y",
        .description = "Vertical Field of View",
        .defaultValue = std::numbers::pi_v<float> / 3.0F,
        .minValue = 0.0F,
        .maxValue = std::numbers::pi_v<float>,
        .onChange =
            [this](auto) {
                callIfExists(m_pOnChanged);
                m_matrixDirty = true;
            },
    }))
  , m_pAspectRatio(std::make_shared<decltype(m_pAspectRatio)::element_type>(PropertyValueConfig<float>{
        .name = "Aspect Ratio",
        .description = "Aspect Ratio of the perspective projection",
        .defaultValue = 1.0F,
        .minValue = 0.0F,
        .onChange =
            [this](auto) {
                callIfExists(m_pOnChanged);
                m_matrixDirty = true;
            },
    }))
  , m_pNear(std::make_shared<decltype(m_pNear)::element_type>(PropertyValueConfig<float>{
        .name = "Near",
        .description = "Distance from the eye to the near plane of the perspective projection",
        .defaultValue = 0.1F,
        .minValue = std::numeric_limits<float>::epsilon(),
        .onChange =
            [this](auto) {
                callIfExists(m_pOnChanged);
                m_matrixDirty = true;
            },
    }))
  , m_pFar(std::make_shared<decltype(m_pFar)::element_type>(PropertyValueConfig<float>{
        .name = "Far",
        .description = "Distance from the eye to the far plane of the perspective projection",
        .defaultValue = 10'000.0F,
        .minValue = std::numeric_limits<float>::epsilon(),
        .onChange =
            [this](auto) {
                callIfExists(m_pOnChanged);
                m_matrixDirty = true;
            },
    }))
  , m_pPropertyGroup(createPropertyGroup("Perspective Projection", {m_pFovY, m_pAspectRatio, m_pNear, m_pFar}))
{
}

auto PerspectiveProjection::getMatrix() const -> const glm::mat4&
{
    if (m_matrixDirty)
    {
        m_matrix = glm::perspective(m_pFovY->getValue(), m_pAspectRatio->getValue(), m_pNear->getValue(),
                                    m_pFar->getValue());
        m_matrixDirty = false;
    }
    return m_matrix;
}