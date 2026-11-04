#include <im3e/property/mock_property_interface.h>

using namespace im3e;

MockPropertyNode::MockPropertyNode()
{
    ON_CALL(*this, registerOnChange(_)).WillByDefault(Invoke([this](auto onChange) {
        return m_callbacks.registerCallback(std::move(onChange));
    }));
}
MockPropertyNode::~MockPropertyNode() = default;

MockProperty::MockProperty()
{
    ON_CALL(*this, registerOnChange(_)).WillByDefault(Invoke([this](auto onChange) {
        return m_callbacks.registerCallback(std::move(onChange));
    }));
}
MockProperty::~MockProperty() = default;

MockPropertyGroup::MockPropertyGroup()
{
    ON_CALL(*this, registerOnChange(_)).WillByDefault(Invoke([this](auto onChange) {
        return m_callbacks.registerCallback(std::move(onChange));
    }));
}
MockPropertyGroup::~MockPropertyGroup() = default;
