#pragma once

#include <im3e/property/properties.h>

#include <glm/glm.hpp>

#include <functional>

namespace im3e {

class ViewTransform
{
public:
    ViewTransform(std::function<void()> onChanged);
    ViewTransform(const ViewTransform&) = delete;
    ViewTransform(ViewTransform&&) = delete;
    ViewTransform& operator=(const ViewTransform&) = delete;
    ViewTransform& operator=(ViewTransform&&) = delete;

    void setPosition(const glm::vec3& rPosition) { m_pPosition->setValue(rPosition); }
    void setDirection(const glm::vec3& rDirection, const glm::vec3& rUp);

    [[nodiscard]] auto getPosition() const -> glm::vec3 { return m_pPosition->getValue(); }
    [[nodiscard]] auto getDirection() const -> glm::vec3 { return m_pDirection->getValue(); }
    [[nodiscard]] auto getUp() const -> glm::vec3 { return m_pUp->getValue(); }
    [[nodiscard]] auto getRight() const -> glm::vec3 { return m_pRight->getValue(); }
    [[nodiscard]] auto getProperties() const -> std::shared_ptr<IPropertyGroup> { return m_pPropertyGroup; }
    [[nodiscard]] auto getMatrix() const -> const glm::mat4&;

private:
    void _onChanged();

    std::function<void()> m_onChanged;

    std::shared_ptr<PropertyValue<glm::vec3>> m_pPosition;
    std::shared_ptr<PropertyValue<glm::vec3>> m_pDirection;
    std::shared_ptr<PropertyValue<glm::vec3>> m_pUp;
    std::shared_ptr<PropertyValue<glm::vec3>> m_pRight;
    std::shared_ptr<IPropertyGroup> m_pPropertyGroup;

    mutable bool m_matrixDirty = true;
    mutable glm::mat4 m_matrix;
};

}  // namespace im3e