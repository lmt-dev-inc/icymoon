#pragma once

#include <im3e/property/property_change_notifier.h>
#include <im3e/property/property_interface.h>

#include <functional>
#include <memory>
#include <optional>
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

        // When set to true, indirect changes to the property are not allows.
        // setValue() is still allowed.
        bool isReadOnly = false;
    };
    explicit Property(Config config, std::function<void()> onChange = {})
      : m_config(std::move(config))
      , m_onChange(std::move(onChange))
      , m_impl(std::make_shared<Impl>(m_config.defaultValue))
    {
    }

    /// @brief Receive change notification via a given callback function.
    /// The given function keeps receiving change notifications while the returned subscription object remains alive.
    /// @param onChange Callback function.
    /// @return Subscription object. It keeps the registration active until destroyed.
    [[nodiscard]] auto registerOnChange(std::function<void()>) const -> PropertySubscription
    {
        return PropertySubscription();
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
    auto getConfig() const -> const Config& { return m_config; }
    auto getValue() const -> const T& { return m_impl->getValue(); }

private:
    Config m_config;
    std::function<void()> m_onChange;

    class Impl : public IProperty
    {
    public:
        Impl() = delete;
        explicit Impl(T value)
          : m_value(std::move(value))
        {
        }

        void setValue(const T& value) { m_value = value; }
        void setValue(T&& value) { m_value = std::move(value); }

        auto getValue() const -> const T& { return m_value; }

    private:
        T m_value;
    };
    std::shared_ptr<Impl> m_impl;
};

}  // namespace im3e