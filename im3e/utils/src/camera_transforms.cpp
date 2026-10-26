#include "camera_transforms.h"

#include <glm/gtc/matrix_transform.hpp>

#include <numbers>

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