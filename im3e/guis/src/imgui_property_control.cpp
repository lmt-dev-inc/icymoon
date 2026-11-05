#include "imgui_property_control.h"

#include <im3e/utils/core/throw_utils.h>
#include <im3e/utils/core/types.h>
#include <im3e/utils/imgui_utils.h>

#include <imgui.h>
#include <imgui_stdlib.h>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

using namespace im3e;

namespace {

template <class T>
auto typeIndex()
{
    return std::type_index(typeid(T));
}

auto createReadOnlyScope(const IProperty& property) -> UniquePtrWithDeleter<const void>
{
    if (property.isReadOnly())
    {
        ImGui::BeginDisabled();
        return {&property, [](auto*) { ImGui::EndDisabled(); }};
    }
    return {};
}

class ImguiBasePropertyControl : public IImguiPropertyControl
{
public:
    ImguiBasePropertyControl(const std::shared_ptr<IProperty>& property)
      : m_name(property->getName())
      , m_inputId(fmt::format("##{}", m_name))
      , m_property(property)
    {
    }

    void draw() final
    {
        if (auto property = m_property.lock())
        {
            _draw(*property);
        }
    }

    auto getName() const -> std::string override { return m_name; }

protected:
    /// @brief Draw the control for the property, only called while the property is alive.
    virtual void _draw(IProperty& property) = 0;

    const std::string m_name;
    const std::string m_inputId;

private:
    std::weak_ptr<IProperty> m_property;
};

struct ImguiVoidPropertyControl : public ImguiBasePropertyControl
{
    ImguiVoidPropertyControl(const std::shared_ptr<IProperty>& property)
      : ImguiBasePropertyControl(property)
    {
    }

    void _draw(IProperty&) override {}
};

struct ImguiUnknownPropertyControl : public ImguiBasePropertyControl
{
    ImguiUnknownPropertyControl(const std::shared_ptr<IProperty>& property)
      : ImguiBasePropertyControl(property)
      , m_typeName(property->getType().name())
    {
    }

    void _draw(IProperty&) override
    {
        ImGui::TextColored(ImVec4(1.0F, 0.0F, 0.0F, 1.0F), "Unknown property type: %s", m_typeName.c_str());
    }

    const std::string m_typeName;
};

class ImguiStringPropertyControl : public ImguiBasePropertyControl
{
public:
    ImguiStringPropertyControl(const std::shared_ptr<IProperty>& property)
      : ImguiBasePropertyControl(property)
      , m_value(any_cast<std::string>(property->getAnyValue()))
    {
    }

    void _draw(IProperty& property) override
    {
        auto readOnlyScope = createReadOnlyScope(property);
        if (ImGui::InputText(m_inputId.c_str(), &m_value);
            ImGui::IsItemDeactivatedAfterEdit())
        {
            property.setAnyValue(m_value);
        }
        m_value = any_cast<std::string>(property.getAnyValue());
    }

private:
    std::string m_value;
};

class ImguiBoolPropertyControl : public ImguiBasePropertyControl
{
public:
    ImguiBoolPropertyControl(const std::shared_ptr<IProperty>& property)
      : ImguiBasePropertyControl(property)
      , m_value(any_cast<bool>(property->getAnyValue()))
    {
    }

    void _draw(IProperty& property) override
    {
        auto readOnlyScope = createReadOnlyScope(property);
        if (ImGui::Checkbox(m_inputId.c_str(), &m_value))
        {
            property.setAnyValue(m_value);
        }
        m_value = any_cast<bool>(property.getAnyValue());
    }

private:
    bool m_value;
};

class ImguiInt32PropertyControl : public ImguiBasePropertyControl
{
public:
    ImguiInt32PropertyControl(const std::shared_ptr<IProperty>& property)
      : ImguiBasePropertyControl(property)
      , m_value(any_cast<int32_t>(property->getAnyValue()))
    {
        if (auto anyMinValueOpt = property->getAnyMinValue(); anyMinValueOpt.has_value())
        {
            m_minValue = std::any_cast<int32_t>(anyMinValueOpt.value());
        }
        if (auto anyMaxValueOpt = property->getAnyMaxValue(); anyMaxValueOpt.has_value())
        {
            m_maxValue = std::any_cast<int32_t>(anyMaxValueOpt.value());
        }
    }

    void _draw(IProperty& property) override
    {
        auto readOnlyScope = createReadOnlyScope(property);
        if (m_minValue.has_value() && m_maxValue.has_value())
        {
            if (ImGui::SliderInt(m_inputId.c_str(), &m_value, m_minValue.value(), m_maxValue.value()))
            {
                property.setAnyValue(static_cast<int32_t>(m_value));
            }
        }
        else
        {
            if (ImGui::InputInt(m_inputId.c_str(), &m_value); ImGui::IsItemDeactivatedAfterEdit())
            {
                property.setAnyValue(static_cast<int32_t>(m_value));
            }
        }
        m_value = any_cast<int32_t>(property.getAnyValue());
    }

private:
    int m_value{};
    std::optional<int> m_minValue{};
    std::optional<int> m_maxValue{};
};

class ImguiUint32PropertyControl : public ImguiBasePropertyControl
{
public:
    ImguiUint32PropertyControl(const std::shared_ptr<IProperty>& property)
      : ImguiBasePropertyControl(property)
      , m_value(any_cast<uint32_t>(property->getAnyValue()))
    {
        if (auto anyMinValueOpt = property->getAnyMinValue(); anyMinValueOpt.has_value())
        {
            m_minValue = std::any_cast<uint32_t>(anyMinValueOpt.value());
        }
        if (auto anyMaxValueOpt = property->getAnyMaxValue(); anyMaxValueOpt.has_value())
        {
            m_maxValue = std::any_cast<uint32_t>(anyMaxValueOpt.value());
        }
    }

    void _draw(IProperty& property) override
    {
        auto readOnlyScope = createReadOnlyScope(property);
        if (m_minValue.has_value() && m_maxValue.has_value())
        {
            if (ImGui::SliderInt(m_inputId.c_str(), &m_value, m_minValue.value(), m_maxValue.value()))
            {
                property.setAnyValue(static_cast<uint32_t>(m_value));
            }
        }
        else
        {
            if (ImGui::InputInt(m_inputId.c_str(), &m_value); ImGui::IsItemDeactivatedAfterEdit())
            {
                m_value = std::max(m_value, 0);
                property.setAnyValue(static_cast<uint32_t>(m_value));
            }
        }
        m_value = any_cast<uint32_t>(property.getAnyValue());
    }

private:
    int m_value{};
    std::optional<int> m_minValue;
    std::optional<int> m_maxValue;
};

class ImguiFloatPropertyControl : public ImguiBasePropertyControl
{
public:
    ImguiFloatPropertyControl(const std::shared_ptr<IProperty>& property)
      : ImguiBasePropertyControl(property)
      , m_value(any_cast<float>(property->getAnyValue()))
    {
        if (auto anyMinValueOpt = property->getAnyMinValue(); anyMinValueOpt.has_value())
        {
            m_minValue = std::any_cast<float>(anyMinValueOpt.value());
        }
        if (auto anyMaxValueOpt = property->getAnyMaxValue(); anyMaxValueOpt.has_value())
        {
            m_maxValue = std::any_cast<float>(anyMaxValueOpt.value());
        }
    }

    void _draw(IProperty& property) override
    {
        auto readOnlyScope = createReadOnlyScope(property);
        if (m_minValue.has_value() && m_maxValue.has_value())
        {
            if (ImGui::SliderFloat(m_inputId.c_str(), &m_value, m_minValue.value(), m_maxValue.value()))
            {
                property.setAnyValue(m_value);
            }
        }
        else
        {
            if (ImGui::InputFloat(m_inputId.c_str(), &m_value, 0.0F, 0.0F, "%.3f"); ImGui::IsItemDeactivatedAfterEdit())
            {
                property.setAnyValue(m_value);
            }
        }
        m_value = any_cast<float>(property.getAnyValue());
    }

private:
    float m_value{};
    std::optional<float> m_minValue{};
    std::optional<float> m_maxValue{};
};

class ImguiVec3PropertyControl : public ImguiBasePropertyControl
{
public:
    ImguiVec3PropertyControl(const std::shared_ptr<IProperty>& property)
      : ImguiBasePropertyControl(property)
      , m_value(any_cast<glm::vec3>(property->getAnyValue()))
    {
    }

    void _draw(IProperty& property) override
    {
        auto readOnlyScope = createReadOnlyScope(property);
        if (ImGui::InputFloat3(m_inputId.c_str(), glm::value_ptr(m_value));
            ImGui::IsItemDeactivatedAfterEdit())
        {
            property.setAnyValue(m_value);
        }
        m_value = any_cast<glm::vec3>(property.getAnyValue());
    }

private:
    glm::vec3 m_value;
};

class ImguiVec4PropertyControl : public ImguiBasePropertyControl
{
public:
    ImguiVec4PropertyControl(const std::shared_ptr<IProperty>& property)
      : ImguiBasePropertyControl(property)
      , m_value(any_cast<glm::vec4>(property->getAnyValue()))
    {
    }

    void _draw(IProperty& property) override
    {
        auto readOnlyScope = createReadOnlyScope(property);
        if (ImGui::InputFloat4(m_inputId.c_str(), glm::value_ptr(m_value));
            ImGui::IsItemDeactivatedAfterEdit())
        {
            property.setAnyValue(m_value);
        }
        m_value = any_cast<glm::vec4>(property.getAnyValue());
    }

private:
    glm::vec4 m_value;
};

class ImguiQuatPropertyControl : public ImguiBasePropertyControl
{
public:
    ImguiQuatPropertyControl(const std::shared_ptr<IProperty>& property)
      : ImguiBasePropertyControl(property)
      , m_value(any_cast<glm::quat>(property->getAnyValue()))
    {
    }

    void _draw(IProperty& property) override
    {
        std::vector<float> value{m_value.w, m_value.x, m_value.y, m_value.z};

        auto readOnlyScope = createReadOnlyScope(property);
        if (ImGui::InputFloat4(m_inputId.c_str(), value.data());
            ImGui::IsItemDeactivatedAfterEdit())
        {
            property.setAnyValue(glm::quat(value[0], value[1], value[2], value[3]));
        }
        m_value = any_cast<glm::quat>(property.getAnyValue());
    }

    auto getName() const -> std::string override { return fmt::format("{} [w, x, y, z]", m_name); }

private:
    glm::quat m_value;
};

using ControlFactoryFct = std::function<std::unique_ptr<IImguiPropertyControl>(const std::shared_ptr<IProperty>&)>;

template <typename T, typename ControlType>
auto createControlFactory() -> std::pair<std::type_index, ControlFactoryFct>
{
    return {typeIndex<T>(), [](const auto& property) { return std::make_unique<ControlType>(property); }};
}

}  // namespace

auto im3e::createImguiPropertyControl(std::weak_ptr<IProperty> property) -> std::unique_ptr<IImguiPropertyControl>
{
    const auto lockedProperty = throwIfArgNull(property.lock(), "Cannot create a control for an expired property");

    static const std::unordered_map<std::type_index, ControlFactoryFct> typeToValueControlFactory{
        createControlFactory<void, ImguiVoidPropertyControl>(),
        createControlFactory<std::string, ImguiStringPropertyControl>(),
        createControlFactory<bool, ImguiBoolPropertyControl>(),
        createControlFactory<int32_t, ImguiInt32PropertyControl>(),
        createControlFactory<uint32_t, ImguiUint32PropertyControl>(),
        createControlFactory<float, ImguiFloatPropertyControl>(),
        createControlFactory<glm::vec3, ImguiVec3PropertyControl>(),
        createControlFactory<glm::vec4, ImguiVec4PropertyControl>(),
        createControlFactory<glm::quat, ImguiQuatPropertyControl>(),
    };

    auto itFind = typeToValueControlFactory.find(lockedProperty->getType());
    if (itFind != typeToValueControlFactory.end())
    {
        return itFind->second(lockedProperty);
    }
    return std::make_unique<ImguiUnknownPropertyControl>(lockedProperty);
}
