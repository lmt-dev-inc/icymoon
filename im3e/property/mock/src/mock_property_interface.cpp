#include <im3e/property/mock_property_interface.h>

using namespace im3e;

namespace {

template <typename T>
auto createMockToken(T* pointer)
{
    return std::shared_ptr<void>(reinterpret_cast<void*>(pointer), [](void*) {});
}

}  // namespace

MockPropertyNode::MockPropertyNode()
  : m_token(createMockToken(this))
{
    ON_CALL(*this, registerOnChange(_)).WillByDefault(Invoke([this](auto onChange) {
        m_registeredOnChange.emplace_back(std::move(onChange));
        return CallbackRegistration(m_token);
    }));
}
MockPropertyNode::~MockPropertyNode() = default;

MockProperty::MockProperty()
  : m_token(createMockToken(this))
{
    ON_CALL(*this, registerOnChange(_)).WillByDefault(Invoke([this](auto onChange) {
        m_registeredOnChange.emplace_back(std::move(onChange));
        return CallbackRegistration(m_token);
    }));
}
MockProperty::~MockProperty() = default;

MockPropertyGroup::MockPropertyGroup()
  : m_token(createMockToken(this))
{
    ON_CALL(*this, registerOnChange(_)).WillByDefault(Invoke([this](auto onChange) {
        m_registeredOnChange.emplace_back(std::move(onChange));
        return CallbackRegistration(m_token);
    }));
}
MockPropertyGroup::~MockPropertyGroup() = default;