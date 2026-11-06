#pragma once

#include <im3e/property/property.h>
#include <im3e/property/property_group.h>

#include <glm/glm.hpp>

#include <functional>
#include <memory>

namespace im3e {

class ViewTransform
{
public:
    ViewTransform(std::function<void()> onChanged);

    void setPosition(const glm::vec3& rPosition) { m_position.setValue(rPosition); }
    void setDirection(const glm::vec3& rDirection, const glm::vec3& rUp);

    [[nodiscard]] auto getPosition() const -> glm::vec3 { return m_position.getValue(); }
    [[nodiscard]] auto getDirection() const -> glm::vec3 { return m_direction.getValue(); }
    [[nodiscard]] auto getUp() const -> glm::vec3 { return m_up.getValue(); }
    [[nodiscard]] auto getRight() const -> glm::vec3 { return m_right.getValue(); }
    [[nodiscard]] auto getProperties() -> std::weak_ptr<IPropertyGroupAccessor> { return m_group.getAccessor(); }
    [[nodiscard]] auto getProperties() const -> std::weak_ptr<const IPropertyGroupAccessor>
    {
        return m_group.getAccessor();
    }
    [[nodiscard]] auto getMatrix() const -> const glm::mat4&;

private:
    void _onChanged();

    std::function<void()> m_onChanged;

    Property<glm::vec3> m_position{{
        .name = "Position",
        .description = "Camera position in world space",
        .defaultValue = glm::vec3(0.0F, 0.0F, 0.0F),
        .isReadOnly = true,
    }};
    Property<glm::vec3> m_direction{{
        .name = "Direction",
        .description = "Camera direction in world space",
        .defaultValue = glm::vec3(0.0F, 0.0F, -1.0F),
        .isReadOnly = true,
    }};
    Property<glm::vec3> m_up{{
        .name = "Up",
        .description = "Camera up vector in world space",
        .defaultValue = glm::vec3(0.0F, 1.0F, 0.0F),
        .isReadOnly = true,
    }};
    Property<glm::vec3> m_right{{
        .name = "Right",
        .description = "Camera right vector in world space",
        .defaultValue = glm::vec3(1.0F, 0.0F, 0.0F),
        .isReadOnly = true,
    }};
    PropertyGroup m_group{
        {
            .name = "View Transform",
            .description = "Group of properties representing a view transform",
        },
        {
            m_position.getAccessor(),
            m_direction.getAccessor(),
            m_up.getAccessor(),
            m_right.getAccessor(),
        },
        [this] { _onChanged(); },
    };

    mutable bool m_matrixDirty = true;
    mutable glm::mat4 m_matrix;
};

}  // namespace im3e
