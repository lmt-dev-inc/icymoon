#include <im3e/scene/view_transform.h>

#include <glm/gtc/matrix_transform.hpp>

#include <numbers>

using namespace im3e;

ViewTransform::ViewTransform(std::function<void()> onChanged)
  : m_onChanged(std::move(onChanged))
{
}

void ViewTransform::setDirection(const glm::vec3& rDirection, const glm::vec3& rUp)
{
    m_direction.setValue(glm::normalize(rDirection));

    // Update right from the given direction and up:
    const auto right = glm::normalize(glm::cross(rDirection, rUp));
    m_right.setValue(right);

    // Ensure that up is orthogonal to direction:
    m_up.setValue(glm::normalize(glm::cross(right, rDirection)));
}

auto ViewTransform::getMatrix() const -> const glm::mat4&
{
    if (m_matrixDirty)
    {
        m_matrix = glm::lookAt(m_position.getValue(), m_position.getValue() + m_direction.getValue(),
                               m_up.getValue());
        m_matrixDirty = false;
    }
    return m_matrix;
}

void ViewTransform::_onChanged()
{
    if (m_onChanged)
    {
        m_onChanged();
    }
    m_matrixDirty = true;
}