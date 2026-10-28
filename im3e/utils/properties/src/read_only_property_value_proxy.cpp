#include "properties.h"

#include <im3e/utils/core/throw_utils.h>

using namespace im3e;

namespace {

class ReadOnlyPropertyValueProxy : public IPropertyValue
{
public:
    ReadOnlyPropertyValueProxy(std::shared_ptr<IPropertyValue> pPropertyValue)
      : m_pPropertyValue(
            throwIfArgNull(std::move(pPropertyValue), "Cannot create read-only proxy for null property value"))
    {
    }

    void registerOnChange(std::weak_ptr<std::function<void()>> pOnChangeCallback) const override
    {
        m_pPropertyValue->registerOnChange(std::move(pOnChangeCallback));
    }

    void setAnyValue(std::any) override
    {
        throw std::runtime_error("Cannot set value of read-only property value proxy");
    }

    auto isReadOnly() const -> bool override { return true; }

    auto getName() const -> std::string override { return m_pPropertyValue->getName(); }
    auto getDescription() const -> std::string override { return m_pPropertyValue->getDescription(); }
    auto getType() const -> std::type_index override { return m_pPropertyValue->getType(); }
    auto getAnyValue() const -> std::any override { return m_pPropertyValue->getAnyValue(); }
    auto getAnyMinValue() const -> std::optional<std::any> override { return m_pPropertyValue->getAnyMinValue(); }
    auto getAnyMaxValue() const -> std::optional<std::any> override { return m_pPropertyValue->getAnyMaxValue(); }

private:
    std::shared_ptr<IPropertyValue> m_pPropertyValue;
};

}  // namespace

auto im3e::createReadOnlyPropertyValueProxy(std::shared_ptr<IPropertyValue> pPropertyValue)
    -> std::shared_ptr<IPropertyValue>
{
    return std::make_shared<ReadOnlyPropertyValueProxy>(std::move(pPropertyValue));
}