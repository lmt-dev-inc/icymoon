#include "imgui_property_control.h"

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

class ImguiBasePropertyControl : public IImguiPropertyControl
{
public:
    ImguiBasePropertyControl(std::shared_ptr<IPropertyValue> pProperty)
      : m_pProperty(std::move(pProperty))
    {
    }

    auto getName() const -> std::string override { return std::string{m_pProperty->getName()}; }

protected:
    std::shared_ptr<IPropertyValue> m_pProperty;
};

struct ImguiVoidPropertyControl : public ImguiBasePropertyControl
{
    ImguiVoidPropertyControl(std::shared_ptr<IPropertyValue> pProperty)
      : ImguiBasePropertyControl(std::move(pProperty))
    {
    }

    void draw() override {}
};

struct ImguiUnknownPropertyControl : public ImguiBasePropertyControl
{
    ImguiUnknownPropertyControl(std::shared_ptr<IPropertyValue> pProperty)
      : ImguiBasePropertyControl(std::move(pProperty))
    {
    }

    void draw() override
    {
        ImGui::TextColored(ImVec4(1.0F, 0.0F, 0.0F, 1.0F), "Unknown property type: %s", m_pProperty->getType().name());
    }
};

class ImguiStringPropertyControl : public ImguiBasePropertyControl
{
public:
    ImguiStringPropertyControl(std::shared_ptr<IPropertyValue> pProperty)
      : ImguiBasePropertyControl(std::move(pProperty))
      , m_value(any_cast<std::string>(m_pProperty->getAnyValue()))
    {
    }

    void draw() override
    {
        if (ImGui::InputText(fmt::format("##{}", m_pProperty->getName()).c_str(), &m_value))
        {
            m_pProperty->setAnyValue(m_value);
        }
        m_value = any_cast<std::string>(m_pProperty->getAnyValue());
    }

private:
    std::string m_value;
};

class ImguiBoolPropertyControl : public ImguiBasePropertyControl
{
public:
    ImguiBoolPropertyControl(std::shared_ptr<IPropertyValue> pProperty)
      : ImguiBasePropertyControl(std::move(pProperty))
      , m_value(any_cast<bool>(m_pProperty->getAnyValue()))
    {
    }

    void draw() override
    {
        if (ImGui::Checkbox(fmt::format("##{}", m_pProperty->getName()).c_str(), &m_value))
        {
            m_pProperty->setAnyValue(m_value);
        }
        m_value = any_cast<bool>(m_pProperty->getAnyValue());
    }

private:
    bool m_value;
};

class ImguiInt32PropertyControl : public ImguiBasePropertyControl
{
public:
    ImguiInt32PropertyControl(std::shared_ptr<IPropertyValue> pProperty)
      : ImguiBasePropertyControl(std::move(pProperty))
      , m_value(any_cast<int32_t>(m_pProperty->getAnyValue()))
    {
        if (auto anyMinValueOpt = m_pProperty->getAnyMinValue(); anyMinValueOpt.has_value())
        {
            m_minValue = std::any_cast<int32_t>(anyMinValueOpt.value());
        }
        if (auto anyMaxValueOpt = m_pProperty->getAnyMaxValue(); anyMaxValueOpt.has_value())
        {
            m_maxValue = std::any_cast<int32_t>(anyMaxValueOpt.value());
        }
    }

    void draw() override
    {
        const auto inputId = fmt::format("##{}", m_pProperty->getName());
        if (m_minValue.has_value() && m_maxValue.has_value())
        {
            if (ImGui::SliderInt(inputId.c_str(), &m_value, m_minValue.value(), m_maxValue.value()))
            {
                m_pProperty->setAnyValue(static_cast<int32_t>(m_value));
            }
        }
        else
        {
            if (ImGui::InputInt(inputId.c_str(), &m_value))
            {
                m_pProperty->setAnyValue(static_cast<int32_t>(m_value));
            }
        }
        m_value = any_cast<int32_t>(m_pProperty->getAnyValue());
    }

private:
    int m_value{};
    std::optional<int> m_minValue{};
    std::optional<int> m_maxValue{};
};

class ImguiUint32PropertyControl : public ImguiBasePropertyControl
{
public:
    ImguiUint32PropertyControl(std::shared_ptr<IPropertyValue> pProperty)
      : ImguiBasePropertyControl(std::move(pProperty))
      , m_value(any_cast<uint32_t>(m_pProperty->getAnyValue()))
    {
        if (auto anyMinValueOpt = m_pProperty->getAnyMinValue(); anyMinValueOpt.has_value())
        {
            m_minValue = std::any_cast<uint32_t>(anyMinValueOpt.value());
        }
        if (auto anyMaxValueOpt = m_pProperty->getAnyMaxValue(); anyMaxValueOpt.has_value())
        {
            m_maxValue = std::any_cast<uint32_t>(anyMaxValueOpt.value());
        }
    }

    void draw() override
    {
        const auto inputId = fmt::format("##{}", m_pProperty->getName());
        if (m_minValue.has_value() && m_maxValue.has_value())
        {
            if (ImGui::SliderInt(inputId.c_str(), &m_value, m_minValue.value(), m_maxValue.value()))
            {
                m_pProperty->setAnyValue(static_cast<uint32_t>(m_value));
            }
        }
        else
        {
            if (ImGui::InputInt(inputId.c_str(), &m_value))
            {
                m_value = std::max(m_value, 0);
                m_pProperty->setAnyValue(static_cast<uint32_t>(m_value));
            }
        }
        m_value = any_cast<uint32_t>(m_pProperty->getAnyValue());
    }

private:
    int m_value{};
    std::optional<int> m_minValue;
    std::optional<int> m_maxValue;
};

class ImguiFloatPropertyControl : public ImguiBasePropertyControl
{
public:
    ImguiFloatPropertyControl(std::shared_ptr<IPropertyValue> pProperty)
      : ImguiBasePropertyControl(std::move(pProperty))
      , m_value(any_cast<float>(m_pProperty->getAnyValue()))
    {
        if (auto anyMinValueOpt = m_pProperty->getAnyMinValue(); anyMinValueOpt.has_value())
        {
            m_minValue = std::any_cast<float>(anyMinValueOpt.value());
        }
        if (auto anyMaxValueOpt = m_pProperty->getAnyMaxValue(); anyMaxValueOpt.has_value())
        {
            m_maxValue = std::any_cast<float>(anyMaxValueOpt.value());
        }
    }

    void draw() override
    {
        const auto inputId = fmt::format("##{}", m_pProperty->getName());
        if (m_minValue.has_value() && m_maxValue.has_value())
        {
            if (ImGui::SliderFloat(inputId.c_str(), &m_value, m_minValue.value(), m_maxValue.value()))
            {
                m_pProperty->setAnyValue(m_value);
            }
        }
        else
        {
            if (ImGui::InputFloat(inputId.c_str(), &m_value, 0.0F, 0.0F))
            {
                m_pProperty->setAnyValue(m_value);
            }
        }
        m_value = any_cast<float>(m_pProperty->getAnyValue());
    }

private:
    float m_value{};
    std::optional<float> m_minValue{};
    std::optional<float> m_maxValue{};
};

class ImguiVec3PropertyControl : public ImguiBasePropertyControl
{
public:
    ImguiVec3PropertyControl(std::shared_ptr<IPropertyValue> pProperty)
      : ImguiBasePropertyControl(std::move(pProperty))
      , m_value(any_cast<glm::vec3>(m_pProperty->getAnyValue()))
    {
    }

    void draw() override
    {
        if (ImGui::InputFloat3(fmt::format("##{}", m_pProperty->getName()).c_str(), glm::value_ptr(m_value)))
        {
            m_pProperty->setAnyValue(m_value);
        }
        m_value = any_cast<glm::vec3>(m_pProperty->getAnyValue());
    }

private:
    glm::vec3 m_value;
};

class ImguiVec4PropertyControl : public ImguiBasePropertyControl
{
public:
    ImguiVec4PropertyControl(std::shared_ptr<IPropertyValue> pProperty)
      : ImguiBasePropertyControl(std::move(pProperty))
      , m_value(any_cast<glm::vec4>(m_pProperty->getAnyValue()))
    {
    }

    void draw() override
    {
        if (ImGui::InputFloat4(fmt::format("##{}", m_pProperty->getName()).c_str(), glm::value_ptr(m_value)))
        {
            m_pProperty->setAnyValue(m_value);
        }
        m_value = any_cast<glm::vec4>(m_pProperty->getAnyValue());
    }

private:
    glm::vec4 m_value;
};

class ImguiQuatPropertyControl : public ImguiBasePropertyControl
{
public:
    ImguiQuatPropertyControl(std::shared_ptr<IPropertyValue> pProperty)
      : ImguiBasePropertyControl(std::move(pProperty))
      , m_value(any_cast<glm::quat>(m_pProperty->getAnyValue()))
    {
    }

    void draw() override
    {
        std::vector<float> value{m_value.w, m_value.x, m_value.y, m_value.z};

        if (ImGui::InputFloat4(fmt::format("##{}", m_pProperty->getName()).c_str(), value.data()))
        {
            m_pProperty->setAnyValue(glm::quat(value[0], value[1], value[2], value[3]));
        }
        m_value = any_cast<glm::quat>(m_pProperty->getAnyValue());
    }

    auto getName() const -> std::string override { return fmt::format("{} [w, x, y, z]", m_pProperty->getName()); }

private:
    glm::quat m_value;
};

using ControlFactoryFct = std::function<std::unique_ptr<IImguiPropertyControl>(std::shared_ptr<IPropertyValue>)>;

template <typename T, typename ControlType>
auto createControlFactory() -> std::pair<std::type_index, ControlFactoryFct>
{
    return {typeIndex<T>(), [](auto pProperty) { return std::make_unique<ControlType>(std::move(pProperty)); }};
}

}  // namespace

auto im3e::createImguiPropertyControl(std::shared_ptr<IProperty> pProperty) -> std::unique_ptr<IImguiPropertyControl>
{
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

    if (auto pValueProperty = std::dynamic_pointer_cast<IPropertyValue>(pProperty))
    {
        auto itFind = typeToValueControlFactory.find(pValueProperty->getType());
        if (itFind != typeToValueControlFactory.end())
        {
            return itFind->second(std::move(pValueProperty));
        }
        return std::make_unique<ImguiUnknownPropertyControl>(std::move(pValueProperty));
    }

    throw std::runtime_error("createImguiPropertyControl only supports IPropertyValue at the moment");
}
