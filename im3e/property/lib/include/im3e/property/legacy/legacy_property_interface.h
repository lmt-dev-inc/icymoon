#pragma once

#include <any>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <typeindex>
#include <vector>

namespace im3e {

class ILegacyProperty
{
public:
    virtual ~ILegacyProperty() = default;

    virtual void registerOnChange(std::weak_ptr<std::function<void()>> pOnChangeCallback) const = 0;

    virtual auto getName() const -> std::string = 0;
};

class ILegacyPropertyValue : public ILegacyProperty
{
public:
    virtual ~ILegacyPropertyValue() = default;

    virtual void setAnyValue(std::any value) = 0;

    virtual auto isReadOnly() const -> bool { return false; }

    virtual auto getDescription() const -> std::string = 0;
    virtual auto getType() const -> std::type_index = 0;
    virtual auto getAnyValue() const -> std::any = 0;
    virtual auto getAnyMinValue() const -> std::optional<std::any> = 0;
    virtual auto getAnyMaxValue() const -> std::optional<std::any> = 0;
};

class ILegacyPropertyGroup : public ILegacyProperty
{
public:
    virtual ~ILegacyPropertyGroup() = default;

    virtual auto getChildren() const -> std::vector<std::shared_ptr<ILegacyProperty>> = 0;
};

}  // namespace im3e