#pragma once

#include <im3e/property/property_interface.h>

#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace im3e {

class PropertyGroup : public IPropertyGroup
{
public:
    struct Config
    {
        std::string name;
        std::string description;
    };
    /// @brief Create a group notifying its listeners whenever one of its children changes.
    /// Children are referenced, not owned: a destroyed child disappears from getChildren(), without notification.
    /// @param config Name and description of the group.
    /// @param children Child nodes, in the order getChildren() returns them. Expired children are ignored.
    /// @param onChange Optional callback function, receiving change notifications for as long as the group lives.
    /// @throws std::invalid_argument if the same child is given more than once.
    PropertyGroup(Config config, std::vector<std::weak_ptr<IPropertyNode>> children,
                  std::function<void()> onChange = {});

    /// @brief Receive change notification via a given callback function.
    /// The given function keeps receiving change notifications while the returned registration object remains alive.
    /// @param onChange Callback function.
    /// @return Registration object. It keeps the registration active until destroyed.
    [[nodiscard]] auto registerOnChange(std::function<void()> onChange) const -> CallbackRegistration;

    [[nodiscard]] auto getConfig() const -> const Config&;
    [[nodiscard]] auto getChildren() -> std::vector<std::weak_ptr<IPropertyNode>>;
    [[nodiscard]] auto getChildren() const -> std::vector<std::weak_ptr<const IPropertyNode>>;
    [[nodiscard]] auto getAccessor() -> std::weak_ptr<IPropertyGroupAccessor> override;
    [[nodiscard]] auto getAccessor() const -> std::weak_ptr<const IPropertyGroupAccessor> override;

private:
    class Impl;
    std::shared_ptr<Impl> m_impl;
    CallbackRegistration m_onChangeRegistration;
};

}  // namespace im3e
