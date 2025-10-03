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

class IProperty
{
public:
    virtual ~IProperty() = default;

    virtual void registerOnChange(std::weak_ptr<std::function<void()>> pOnChangeCallback) const = 0;

    virtual auto getName() const -> std::string = 0;
};

class IPropertyValue : public IProperty
{
public:
    virtual ~IPropertyValue() = default;

    virtual void setAnyValue(std::any value) = 0;

    virtual auto isReadOnly() const -> bool { return false; }

    virtual auto getDescription() const -> std::string = 0;
    virtual auto getType() const -> std::type_index = 0;
    virtual auto getAnyValue() const -> std::any = 0;
    virtual auto getAnyMinValue() const -> std::optional<std::any> = 0;
    virtual auto getAnyMaxValue() const -> std::optional<std::any> = 0;
};

class IPropertyGroup : public IProperty
{
public:
    virtual ~IPropertyGroup() = default;

    virtual auto getChildren() const -> std::vector<std::shared_ptr<IProperty>> = 0;
};

}  // namespace im3e