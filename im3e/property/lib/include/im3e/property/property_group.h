#pragma once

#include <im3e/property/property_change_notifier.h>
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
      , m_onChange(std::move(onChange))
    {
    }

    auto getConfig() const -> const Config& { return m_impl->getConfig(); }
    auto getChildren() const -> const std::vector<std::weak_ptr<IPropertyNode>> { return m_impl->getChildren(); }

private:
    class Impl : public IPropertyGroup
    {
    public:
        Impl() = delete;
        Impl(Config config, std::vector<std::weak_ptr<IPropertyNode>> children)
          : m_config(std::move(config))
          , m_children(std::move(children))
        {
        }

        auto registerOnChange(std::function<void()>) const -> PropertySubscription override
        {
            return PropertySubscription();
        }

        auto getConfig() const -> const Config& { return m_config; }
        auto getName() const -> const std::string& override { return m_config.name; }
        auto getChildren() const -> const std::vector<std::weak_ptr<IPropertyNode>> override { return m_children; }

    private:
        Config m_config;
        std::vector<std::weak_ptr<IPropertyNode>> m_children;
    };
    std::shared_ptr<Impl> m_impl;
    std::function<void()> m_onChange;
};

}  // namespace im3e