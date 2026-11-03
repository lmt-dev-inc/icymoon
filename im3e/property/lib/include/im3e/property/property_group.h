#pragma once

#include <im3e/property/property_interface.h>

#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace im3e {

class PropertyGroup
{
public:
    PropertyGroup() = delete;
    PropertyGroup(const PropertyGroup&) = delete;
    PropertyGroup(PropertyGroup&&) = delete;
    PropertyGroup& operator=(const PropertyGroup&) = delete;
    PropertyGroup& operator=(PropertyGroup&&) = delete;

    struct Config
    {
        std::string name;
        std::string description;
    };
    PropertyGroup(Config config, std::vector<std::weak_ptr<IPropertyNode>> children,
                  std::function<void()> onChange = {})
      : m_impl(std::make_shared<Impl>(std::move(config), std::move(children)))
    {
        if (onChange)
        {
            m_onChangeRegistration = m_impl->registerOnChange(std::move(onChange));
        }
    }

    [[nodiscard]] auto registerOnChange(std::function<void()> onChange) const -> CallbackRegistration
    {
        return m_impl->registerOnChange(std::move(onChange));
    }

    [[nodiscard]] auto getConfig() const -> const Config& { return m_impl->getConfig(); }
    [[nodiscard]] auto getChildren() const -> const std::vector<std::weak_ptr<IPropertyNode>>
    {
        return m_impl->getChildren();
    }
    [[nodiscard]] auto getInterface() -> std::weak_ptr<IPropertyGroup> { return m_impl; }
    [[nodiscard]] auto getInterface() const -> std::weak_ptr<const IPropertyGroup> { return m_impl; }

private:
    class Impl : public IPropertyGroup
    {
    public:
        Impl() = delete;
        Impl(Config config, std::vector<std::weak_ptr<IPropertyNode>> children)
          : m_config(std::move(config))
        {
            m_children.reserve(children.size());
            m_childRegistrations.reserve(children.size());
            for (auto& childWeakPtr : children)
            {
                if (auto child = childWeakPtr.lock())
                {
                    m_childRegistrations.emplace_back(child->registerOnChange([this] { m_callbacks.invokeAll(); }));
                    m_children.emplace_back(std::move(childWeakPtr));
                }
            }
        }

        auto registerOnChange(std::function<void()> onChange) const -> CallbackRegistration override
        {
            return m_callbacks.registerCallback(std::move(onChange));
        }

        auto getConfig() const -> const Config& { return m_config; }
        auto getName() const -> const std::string& override { return m_config.name; }
        auto getChildren() const -> std::vector<std::weak_ptr<IPropertyNode>> override { return m_children; }

    private:
        Config m_config;
        std::vector<std::weak_ptr<IPropertyNode>> m_children;
        std::vector<CallbackRegistration> m_childRegistrations;
        mutable CallbackRegistry<> m_callbacks;
    };
    std::shared_ptr<Impl> m_impl;
    CallbackRegistration m_onChangeRegistration;
};

}  // namespace im3e