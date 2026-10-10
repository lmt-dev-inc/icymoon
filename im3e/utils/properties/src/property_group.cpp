#include "properties.h"

#include "property_change_notifier.h"

#include <string>

using namespace im3e;

namespace {

class PropertyGroup : public IPropertyGroup
{
public:
    PropertyGroup(std::string_view name, std::vector<std::shared_ptr<IProperty>> pProperties)
      : m_name(name)
      , m_pProperties(std::move(pProperties))
    {
    }

    void registerOnChange(std::weak_ptr<std::function<void()>> pOnChangeCallback) const override
    {
        m_notifier.registerOnChange(std::move(pOnChangeCallback));
    }

    auto getName() const -> std::string override { return m_name; }
    auto getChildren() const -> std::vector<std::shared_ptr<IProperty>> override { return m_pProperties; }

private:
    const std::string m_name;
    std::vector<std::shared_ptr<IProperty>> m_pProperties;
    mutable PropertyChangeNotifier m_notifier;
};

}  // namespace

auto im3e::createPropertyGroup(std::string_view name, std::vector<std::shared_ptr<IProperty>> pProperties)
    -> std::shared_ptr<IPropertyGroup>
{
    return std::make_shared<PropertyGroup>(name, std::move(pProperties));
}