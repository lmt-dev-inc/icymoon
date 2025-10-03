#pragma once

#include <im3e/utils/properties/properties.h>

#include <glm/glm.hpp>

#include <functional>

namespace im3e {

class PerspectiveProjection
{
public:
    PerspectiveProjection(std::weak_ptr<std::function<void()>> pOnChanged);

    void setFovY(float fovY) { m_pFovY->setValue(fovY); }
    void setAspectRatio(float aspectRatio) { m_pAspectRatio->setValue(aspectRatio); }
    void setNear(float near) { m_pNear->setValue(near); }
    void setFar(float far) { m_pFar->setValue(far); }

    [[nodiscard]] auto getFovY() const -> float { return m_pFovY->getValue(); }
    [[nodiscard]] auto getAspectRatio() const -> float { return m_pAspectRatio->getValue(); }
    [[nodiscard]] auto getNear() const -> float { return m_pNear->getValue(); }
    [[nodiscard]] auto getFar() const -> float { return m_pFar->getValue(); }
    [[nodiscard]] auto getProperties() const -> std::shared_ptr<IPropertyGroup> { return m_pPropertyGroup; }
    [[nodiscard]] auto getMatrix() const -> const glm::mat4&;

private:
    std::weak_ptr<std::function<void()>> m_pOnChanged;

    std::shared_ptr<PropertyValue<float>> m_pFovY;
    std::shared_ptr<PropertyValue<float>> m_pAspectRatio;
    std::shared_ptr<PropertyValue<float>> m_pNear;
    std::shared_ptr<PropertyValue<float>> m_pFar;
    std::shared_ptr<IPropertyGroup> m_pPropertyGroup;

    mutable bool m_matrixDirty = true;
    mutable glm::mat4 m_matrix;
};

class ViewTransform
{
public:
    ViewTransform(std::weak_ptr<std::function<void()>> pOnChanged);

    void setPosition(const glm::vec3& rPosition) { m_pPosition->setValue(rPosition); }
    void setDirection(const glm::vec3& rDirection, const glm::vec3& rUp);

    [[nodiscard]] auto getPosition() const -> glm::vec3 { return m_pPosition->getValue(); }
    [[nodiscard]] auto getDirection() const -> glm::vec3 { return m_pDirection->getValue(); }
    [[nodiscard]] auto getUp() const -> glm::vec3 { return m_pUp->getValue(); }
    [[nodiscard]] auto getRight() const -> glm::vec3 { return m_pRight->getValue(); }
    [[nodiscard]] auto getProperties() const -> std::shared_ptr<IPropertyGroup> { return m_pPropertyGroup; }
    [[nodiscard]] auto getMatrix() const -> const glm::mat4&;

private:
    std::weak_ptr<std::function<void()>> m_pOnChanged;

    std::shared_ptr<PropertyValue<glm::vec3>> m_pPosition;
    std::shared_ptr<PropertyValue<glm::vec3>> m_pDirection;
    std::shared_ptr<PropertyValue<glm::vec3>> m_pUp;
    std::shared_ptr<PropertyValue<glm::vec3>> m_pRight;
    std::shared_ptr<IPropertyGroup> m_pPropertyGroup;

    mutable bool m_matrixDirty = true;
    mutable glm::mat4 m_matrix;
};

}  // namespace im3e