#include <im3e/scene/perspective_projection.h>

#include <glm/gtc/matrix_transform.hpp>

#include <numbers>

using namespace im3e;

PerspectiveProjection::PerspectiveProjection(std::function<void()> onChanged)
  : m_onChanged(std::move(onChanged))
{
}

auto PerspectiveProjection::getMatrix() const -> const glm::mat4&
{
    if (m_matrixDirty)
    {
        m_matrix = glm::perspective(m_fovY.getValue(), m_aspectRatio.getValue(), m_near.getValue(), m_far.getValue());
        m_matrixDirty = false;
    }
    return m_matrix;
}

void PerspectiveProjection::_onChanged()
{
    m_matrixDirty = true;
    if (m_onChanged)
    {
        m_onChanged();
    }
}