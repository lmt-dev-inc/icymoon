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
    [[nodiscard]] auto getChildren() const -> std::vector<std::weak_ptr<IPropertyNode>>
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
            for (auto& childWeakPtr : children)
            {
                if (auto child = childWeakPtr.lock())
                {
                    m_children.emplace_back(Child{.node = std::move(childWeakPtr),
                                                  .registration = child->registerOnChange([this] { _onChange(); })});
                }
            }
        }

        auto registerOnChange(std::function<void()> onChange) const -> CallbackRegistration override
        {
            return m_callbacks.registerCallback(std::move(onChange));
        }

        auto getConfig() const -> const Config& { return m_config; }
        auto getName() const -> const std::string& override { return m_config.name; }
        auto getChildren() const -> std::vector<std::weak_ptr<IPropertyNode>> override
        {
            std::vector<std::weak_ptr<IPropertyNode>> children;
            children.reserve(m_children.size());
            for (const auto& child : m_children)
            {
                if (!child.node.expired())
                {
                    children.emplace_back(child.node);
                }
            }
            return children;
        }

    private:
        void _onChange()
        {
            std::erase_if(m_children, [](const Child& child) { return child.node.expired(); });
            m_callbacks.invokeAll();
        }

        Config m_config;
        struct Child
        {
            std::weak_ptr<IPropertyNode> node;
            CallbackRegistration registration;
        };
        std::vector<Child> m_children;
        mutable CallbackRegistry<> m_callbacks;
    };
    std::shared_ptr<Impl> m_impl;
    CallbackRegistration m_onChangeRegistration;
};

}  // namespace im3e