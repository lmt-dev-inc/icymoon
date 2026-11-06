#include <im3e/property/property_group.h>

#include <im3e/utils/core/throw_utils.h>

#include <algorithm>
#include <stdexcept>
#include <utility>

using namespace im3e;

class PropertyGroup::Impl : public IPropertyGroupAccessor, public std::enable_shared_from_this<PropertyGroup::Impl>
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
                throwIfFalse<std::invalid_argument>(
                    std::ranges::none_of(m_children, [&child](const Child& kept) { return kept.node.lock() == child; }),
                    "Invalid property group: the same child was given more than once");
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
    auto getDescription() const -> const std::string& override { return m_config.description; }
    auto getChildren() -> std::vector<std::weak_ptr<IPropertyNode>> override
    {
        return _getChildren<IPropertyNode>();
    }
    auto getChildren() const -> std::vector<std::weak_ptr<const IPropertyNode>> override
    {
        return _getChildren<const IPropertyNode>();
    }

private:
    template <typename NodeT>
    auto _getChildren() const -> std::vector<std::weak_ptr<NodeT>>
    {
        std::vector<std::weak_ptr<NodeT>> children;
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

    void _onChange()
    {
        std::erase_if(m_children, [](const Child& child) { return child.node.expired(); });

        // Stay alive until the end of the notification in case a callback destroys the owning group:
        const auto keepAlive = shared_from_this();
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

PropertyGroup::PropertyGroup(Config config, std::vector<std::weak_ptr<IPropertyNode>> children,
                             std::function<void()> onChange)
  : m_impl(std::make_shared<Impl>(std::move(config), std::move(children)))
{
    if (onChange)
    {
        m_onChangeRegistration = m_impl->registerOnChange(std::move(onChange));
    }
}

auto PropertyGroup::registerOnChange(std::function<void()> onChange) const -> CallbackRegistration
{
    return m_impl->registerOnChange(std::move(onChange));
}

auto PropertyGroup::getConfig() const -> const Config&
{
    return m_impl->getConfig();
}

auto PropertyGroup::getChildren() -> std::vector<std::weak_ptr<IPropertyNode>>
{
    return m_impl->getChildren();
}

auto PropertyGroup::getChildren() const -> std::vector<std::weak_ptr<const IPropertyNode>>
{
    return std::as_const(*m_impl).getChildren();
}

auto PropertyGroup::getAccessor() -> std::weak_ptr<IPropertyGroupAccessor>
{
    return m_impl;
}

auto PropertyGroup::getAccessor() const -> std::weak_ptr<const IPropertyGroupAccessor>
{
    return m_impl;
}
