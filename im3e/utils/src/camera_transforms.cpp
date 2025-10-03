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
  , m_pPropertyGroup(createPropertyGroup("Perspective Projection",
                                         {m_pFovY, createReadOnlyPropertyValueProxy(m_pAspectRatio), m_pNear, m_pFar}))
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

ViewTransform::ViewTransform(std::weak_ptr<std::function<void()>> pOnChanged)
  : m_pOnChanged(std::move(pOnChanged))
  , m_pPosition(std::make_shared<decltype(m_pPosition)::element_type>(PropertyValueConfig<glm::vec3>{
        .name = "Position",
        .description = "Camera position in world space",
        .defaultValue = glm::vec3(0.0F, 0.0F, 0.0F),
        .onChange =
            [this](auto) {
                callIfExists(m_pOnChanged);
                m_matrixDirty = true;
            },
    }))
  , m_pDirection(std::make_shared<decltype(m_pDirection)::element_type>(PropertyValueConfig<glm::vec3>{
        .name = "Direction",
        .description = "Camera direction in world space",
        .defaultValue = glm::vec3(0.0F, 0.0F, -1.0F),
        .onChange =
            [this](auto) {
                callIfExists(m_pOnChanged);
                m_matrixDirty = true;
            },
    }))
  , m_pUp(std::make_shared<decltype(m_pUp)::element_type>(PropertyValueConfig<glm::vec3>{
        .name = "Up",
        .description = "Camera up vector in world space",
        .defaultValue = glm::vec3(0.0F, 1.0F, 0.0F),
        .onChange =
            [this](auto) {
                callIfExists(m_pOnChanged);
                m_matrixDirty = true;
            },
    }))
  , m_pRight(std::make_shared<decltype(m_pRight)::element_type>(PropertyValueConfig<glm::vec3>{
        .name = "Right",
        .description = "Camera right vector in world space",
        .defaultValue = glm::vec3(1.0F, 0.0F, 0.0F),
        .onChange =
            [this](auto) {
                callIfExists(m_pOnChanged);
                m_matrixDirty = true;
            },
    }))
  , m_pPropertyGroup(
        createPropertyGroup("View Transform", {m_pPosition, m_pDirection, createReadOnlyPropertyValueProxy(m_pUp),
                                               createReadOnlyPropertyValueProxy(m_pRight)}))
{
}

void ViewTransform::setDirection(const glm::vec3& rDirection, const glm::vec3& rUp)
{
    m_pDirection->setValue(glm::normalize(rDirection));

    // Update right from the given direction and up:
    const auto right = glm::normalize(glm::cross(rDirection, rUp));
    m_pRight->setValue(right);

    // Ensure that up is orthogonal to direction:
    m_pUp->setValue(glm::normalize(glm::cross(right, rDirection)));

    m_matrixDirty = true;
    callIfExists(m_pOnChanged);
}

auto ViewTransform::getMatrix() const -> const glm::mat4&
{
    if (m_matrixDirty)
    {
        m_matrix = glm::lookAt(m_pPosition->getValue(), m_pPosition->getValue() + m_pDirection->getValue(),
                               m_pUp->getValue());
        m_matrixDirty = false;
    }
    return m_matrix;
}