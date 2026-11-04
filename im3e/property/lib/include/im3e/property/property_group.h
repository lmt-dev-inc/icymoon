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
                  std::function<void()> onChange = {});

    [[nodiscard]] auto registerOnChange(std::function<void()> onChange) const -> CallbackRegistration;

    [[nodiscard]] auto getConfig() const -> const Config&;
    [[nodiscard]] auto getChildren() const -> std::vector<std::weak_ptr<IPropertyNode>>;
    [[nodiscard]] auto getInterface() -> std::weak_ptr<IPropertyGroup>;
    [[nodiscard]] auto getInterface() const -> std::weak_ptr<const IPropertyGroup>;

private:
    class Impl;
    std::shared_ptr<Impl> m_impl;
    CallbackRegistration m_onChangeRegistration;
};

}  // namespace im3e
