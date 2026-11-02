#pragma once

#include <im3e/property/property_interface.h>
#include <im3e/utils/core/throw_utils.h>
#include <im3e/utils/core/types.h>

#include <any>
#include <functional>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>

namespace im3e {

template <typename T>
class Property
{
public:
    Property() = delete;
    Property(const Property&) = delete;
    Property(Property&&) = delete;
    Property& operator=(const Property&) = delete;
    Property& operator=(Property&&) = delete;

    struct Config
    {
        std::string name;
        std::string description;
        T defaultValue;
        std::optional<T> minValue;
        std::optional<T> maxValue;

        // When set to true, changes through the interface to the property are not allowed.
        // Changes through Property (setValue and update) are still allowed.
        bool isReadOnly = false;
    };
    explicit Property(Config config, std::function<void()> onChange = {})
      : m_impl(std::make_shared<Impl>(std::move(config)))
    {
        if (onChange)
        {
            m_onChangeRegistration = m_impl->registerOnChange(std::move(onChange));
        }
    }

    /// @brief Receive change notification via a given callback function.
    /// The given function keeps receiving change notifications while the returned registration object remains alive.
    /// @param onChange Callback function.
    /// @return Registration object. It keeps the registration active until destroyed.
    [[nodiscard]] auto registerOnChange(std::function<void()> onChange) const -> CallbackRegistration
    {
        return m_impl->registerOnChange(std::move(onChange));
    }

    /// @brief Change the property value via an update function.
    /// This allows to perform multiple value changes while only notifying once for change.
    /// @param updateFct Function to execute to perform changes on the property.
    template <typename Fct>
        requires std::is_invocable_r_v<T, Fct, T>
    void update(Fct&& updateFct)
    {
        m_impl->setValue(updateFct(m_impl->getValue()));
    }

    void setValue(const T& value) { m_impl->setValue(value); }
    void setValue(T&& value) { m_impl->setValue(std::move(value)); }

    auto getInterface() -> std::weak_ptr<IProperty> { return m_impl; }
    auto getInterface() const -> std::weak_ptr<const IProperty> { return m_impl; }
    auto getConfig() const -> const Config& { return m_impl->getConfig(); }
    auto getValue() const -> const T& { return m_impl->getValue(); }

private:
    class Impl : public IProperty
    {
    public:
        Impl() = delete;
        explicit Impl(Config config)
          : m_config(std::move(config))
          , m_value(_clamp(m_config.defaultValue))
        {
            if constexpr (HasLessThanOperator<T>::value)
            {
                if (m_config.minValue.has_value() && m_config.maxValue.has_value())
                {
                    throwIfFalse<std::invalid_argument>(
                        m_config.minValue.value() <= m_config.maxValue.value(),
                        "Invalid property configuration: minValue must be lower or equal to maxValue");
                }
            }
            else
            {
                throwIfFalse<std::invalid_argument>(
                    !m_config.minValue.has_value() && !m_config.maxValue.has_value(),
                    "Invalid property configuration: minValue and maxValue require an ordered type.");
            }
        }

        auto registerOnChange(std::function<void()> onChange) const -> CallbackRegistration override
        {
            return m_callbacks.registerCallback(std::move(onChange));
        }

        void setValue(const T& value) { _setValue(value); }
        void setValue(T&& value) { _setValue(std::move(value)); }
        void setAnyValue(std::any value) override
        {
            throwIfFalse<std::logic_error>(!m_config.isReadOnly, "Cannot change value of read-only property");
            _setValue(std::any_cast<T>(std::move(value)));
        }

        auto getConfig() const -> const Config& { return m_config; }
        auto getName() const -> const std::string& override { return m_config.name; }
        auto getValue() const -> const T& { return m_value; }
        auto getAnyValue() const -> std::any override { return m_value; }

    private:
        template <typename U>
        void _setValue(U&& value)
        {
            const T& clamped = _clamp(value);
            if (m_value == clamped)
            {
                return;
            }

            if (&clamped == &value)
            {
                m_value = std::forward<U>(value);
            }
            else
            {
                m_value = clamped;
            }
            m_callbacks.invokeAll();
        }

        auto _clamp(const T& value) const -> const T&
        {
            if constexpr (HasLessThanOperator<T>::value)
            {
                if (m_config.minValue.has_value() && value < m_config.minValue.value())
                {
                    return m_config.minValue.value();
                }
                if (m_config.maxValue.has_value() && m_config.maxValue.value() < value)
                {
                    return m_config.maxValue.value();
                }
            }
            return value;
        }

        Config m_config;
        T m_value;
        mutable CallbackRegistry<> m_callbacks;
    };
    std::shared_ptr<Impl> m_impl;
    CallbackRegistration m_onChangeRegistration;
};

}  // namespace im3e
