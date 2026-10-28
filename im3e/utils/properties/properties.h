#pragma once

#include "property_change_notifier.h"

#include <im3e/utils/core/types.h>
#include <im3e/utils/properties/api/property.h>

#include <optional>
#include <string>
#include <type_traits>

namespace im3e {

template <typename T>
struct PropertyValueTConfig
{
    using Type = T;
    const std::string_view name;
    const std::string_view description;
    std::optional<T> defaultValue;
    std::optional<T> minValue;
    std::optional<T> maxValue;
};

// clang-format off
template <typename T> struct IsPropertyValueTConfigT : std::false_type{};
template <typename T> struct IsPropertyValueTConfigT<PropertyValueTConfig<T>> : std::true_type{};
template <typename T> inline constexpr bool IsPropertyValueTConfig = IsPropertyValueTConfigT<T>::value;

template <const auto& Config> 
concept CPropertyValueTConfig = IsPropertyValueTConfig<std::remove_cvref_t<decltype(Config)>>;
// clang-format on

template <const auto& Config>
    requires CPropertyValueTConfig<Config>
class PropertyValueT : public IPropertyValue
{
public:
    using Type = typename std::remove_cvref_t<decltype(Config)>::Type;

    PropertyValueT()
    {
        if (Config.minValue.has_value())
        {
            m_value = std::max(Config.minValue.value(), m_value);
        }
        if (Config.maxValue.has_value())
        {
            m_value = std::min(Config.maxValue.value(), m_value);
        }
    }

    void registerOnChange(std::weak_ptr<std::function<void()>> pOnChangeCallback) const override
    {
        m_changeNotifier.registerOnChange(std::move(pOnChangeCallback));
    }

    void setValue(Type value)
    {
        if (value != m_value)
        {
            m_value = std::move(value);
            m_changeNotifier.notifyChanged();
        }
    }

    void setAnyValue(std::any anyValue) override
    {
        auto value = std::any_cast<Type>(anyValue);
        setValue(std::move(value));
    }

    auto getName() const -> std::string override { return std::string{Config.name}; }
    auto getDescription() const -> std::string override { return std::string{Config.description}; }
    auto getType() const -> std::type_index override { return typeid(Type); }
    auto getAnyValue() const -> std::any override { return m_value; }
    auto getAnyMinValue() const -> std::optional<std::any> override
    {
        if (Config.minValue.has_value())
        {
            return Config.minValue.value();
        }
        return std::nullopt;
    }
    auto getAnyMaxValue() const -> std::optional<std::any> override
    {
        if (Config.maxValue.has_value())
        {
            return Config.maxValue.value();
        }
        return std::nullopt;
    }
    auto getValue() const -> Type { return m_value; }
    auto getMinValue() const -> std::optional<Type> { return Config.minValue; }
    auto getMaxValue() const -> std::optional<Type> { return Config.maxValue; }

private:
    mutable PropertyChangeNotifier m_changeNotifier;
    Type m_value = Config.defaultValue.value_or(Type{});
};

template <typename T>
struct PropertyValueConfig
{
    using Type = T;
    const std::string name;
    const std::string description;
    T defaultValue{};
    std::optional<T> minValue{};
    std::optional<T> maxValue{};
    std::function<void(T newValue)> onChange{};
};

template <typename T>
class PropertyValue : public IPropertyValue
{
public:
    using Type = T;

    PropertyValue(PropertyValueConfig<T> config)
      : m_config(std::move(config))
      , m_value(config.defaultValue)
    {
        if constexpr (HasLessThanOperator<Type>::value)
        {
            if (m_config.minValue.has_value())
            {
                m_value = std::max(m_config.minValue.value(), m_value);
            }
            if (m_config.maxValue.has_value())
            {
                m_value = std::min(m_config.maxValue.value(), m_value);
            }
        }
    }

    void registerOnChange(std::weak_ptr<std::function<void()>> pOnChangeCallback) const override
    {
        m_notifier.registerOnChange(std::move(pOnChangeCallback));
    }

    void setValue(Type value)
    {
        if (m_value == value)
        {
            return;
        }
        const auto previousValue = m_value;
        m_value = std::move(value);

        if constexpr (HasLessThanOperator<Type>::value)
        {
            if (m_config.minValue.has_value())
            {
                m_value = std::max(m_config.minValue.value(), m_value);
            }
            if (m_config.maxValue.has_value())
            {
                m_value = std::min(m_config.maxValue.value(), m_value);
            }
        }
        if (m_value == previousValue)
        {
            return;
        }

        m_notifier.notifyChanged();
        if (m_config.onChange)
        {
            m_config.onChange(m_value);
        }
    }

    void setAnyValue(std::any anyValue) override
    {
        auto value = std::any_cast<Type>(anyValue);
        setValue(std::move(value));
    }

    auto getName() const -> std::string override { return m_config.name; }
    auto getDescription() const -> std::string override { return m_config.description; }
    auto getType() const -> std::type_index override { return typeid(Type); }
    auto getAnyValue() const -> std::any override { return m_value; }
    auto getAnyMinValue() const -> std::optional<std::any> override
    {
        if (m_config.minValue.has_value())
        {
            return m_config.minValue.value();
        }
        return std::nullopt;
    }
    auto getAnyMaxValue() const -> std::optional<std::any> override
    {
        if (m_config.maxValue.has_value())
        {
            return m_config.maxValue.value();
        }
        return std::nullopt;
    }
    auto getValue() const -> Type { return m_value; }
    auto getMinValue() const -> std::optional<Type> { return m_config.minValue; }
    auto getMaxValue() const -> std::optional<Type> { return m_config.maxValue; }

private:
    PropertyValueConfig<T> m_config;
    mutable PropertyChangeNotifier m_notifier;
    Type m_value;
};

auto createPropertyGroup(std::string_view name, std::vector<std::shared_ptr<IProperty>> pProperties)
    -> std::shared_ptr<IPropertyGroup>;

/// @brief Create a read-only proxy for a property value.
/// This allows to pass around property values without allowing modification of the value.
/// The proxy will forward change notifications from the original property value.
///
/// @param pPropertyValue The property value to create a read-only proxy for.
///
/// @return A read-only proxy for the given property value.
auto createReadOnlyPropertyValueProxy(std::shared_ptr<IPropertyValue> pPropertyValue)
    -> std::shared_ptr<IPropertyValue>;

}  // namespace im3e
