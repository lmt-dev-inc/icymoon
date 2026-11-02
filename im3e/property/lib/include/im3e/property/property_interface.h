#pragma once

#include <im3e/utils/core/callback_registry.h>

#include <any>

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
};

class IProperty : public IPropertyNode
{
public:
    virtual void setAnyValue(std::any value) = 0;
    virtual auto getAnyValue() const -> std::any = 0;
};

class IPropertyGroup : public IPropertyNode
{
public:
    virtual auto getChildren() const -> const std::vector<std::weak_ptr<IPropertyNode>> = 0;
};

}  // namespace im3e