#pragma once

#include <im3e/property/property_change_notifier.h>

#include <functional>
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
    }

    void setValue(const T&) {}
    void setValue(T&&) {}

    auto getConfig() const -> const Config& { return m_config; }
    auto getValue() const -> const T& { return m_config.defaultValue; }

private:
    Config m_config;
    std::function<void()> m_onChange;
};

}  // namespace im3e