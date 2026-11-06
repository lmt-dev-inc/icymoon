#pragma once

#include <im3e/utils/core/callback_registry.h>

#include <any>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <typeindex>
#include <vector>

namespace im3e {

class IPropertyNode
{
public:
    IPropertyNode() = default;
    IPropertyNode(const IPropertyNode&) = delete;
    IPropertyNode(IPropertyNode&&) = delete;
    IPropertyNode& operator=(const IPropertyNode&) = delete;
    IPropertyNode& operator=(IPropertyNode&&) = delete;
    virtual ~IPropertyNode() = default;

    virtual auto registerOnChange(std::function<void()> onChange) const -> CallbackRegistration = 0;

    virtual auto getName() const -> const std::string& = 0;
    virtual auto getDescription() const -> const std::string& = 0;
};

class IPropertyAccessor : public IPropertyNode
{
public:
    virtual void setAnyValue(std::any value) = 0;

    virtual auto isReadOnly() const -> bool = 0;

    virtual auto getType() const -> std::type_index = 0;
    virtual auto getAnyValue() const -> std::any = 0;
    virtual auto getAnyMinValue() const -> std::optional<std::any> = 0;
    virtual auto getAnyMaxValue() const -> std::optional<std::any> = 0;
};

class IPropertyGroupAccessor : public IPropertyNode
{
public:
    virtual auto getChildren() const -> std::vector<std::weak_ptr<IPropertyNode>> = 0;
};

class IProperty
{
public:
    IProperty() = default;
    IProperty(const IProperty&) = delete;
    IProperty(IProperty&&) = delete;
    IProperty& operator=(const IProperty&) = delete;
    IProperty& operator=(IProperty&&) = delete;
    virtual ~IProperty() = default;

    [[nodiscard]] virtual auto getAccessor() -> std::weak_ptr<IPropertyAccessor> = 0;
    [[nodiscard]] virtual auto getAccessor() const -> std::weak_ptr<const IPropertyAccessor> = 0;
};

class IPropertyGroup
{
public:
    IPropertyGroup() = default;
    IPropertyGroup(const IPropertyGroup&) = delete;
    IPropertyGroup(IPropertyGroup&&) = delete;
    IPropertyGroup& operator=(const IPropertyGroup&) = delete;
    IPropertyGroup& operator=(IPropertyGroup&&) = delete;
    virtual ~IPropertyGroup() = default;

    [[nodiscard]] virtual auto getAccessor() -> std::weak_ptr<IPropertyGroupAccessor> = 0;
    [[nodiscard]] virtual auto getAccessor() const -> std::weak_ptr<const IPropertyGroupAccessor> = 0;
};

}  // namespace im3e
