#pragma once

#include <functional>
#include <string>

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
    template <typename... Args>
    explicit PropertyGroup(Config config, std::function<void()> onChange = {})
      : m_config(std::move(config))
      , m_onChange(std::move(onChange))
    {
    }

private:
    Config m_config;
    std::function<void()> m_onChange;
};

}  // namespace im3e