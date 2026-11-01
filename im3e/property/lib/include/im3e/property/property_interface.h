#pragma once

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
};

class IProperty : public IPropertyNode
{
public:
};

class IPropertyGroup : public IPropertyNode
{
public:
    virtual auto getChildren() const -> const std::vector<std::weak_ptr<IPropertyNode>> = 0;
};

}  // namespace im3e