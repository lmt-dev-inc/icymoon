#include <im3e/property/mock_property_interface.h>

using namespace im3e;

MockPropertyNode::MockPropertyNode()
{
    ON_CALL(*this, registerOnChange(_)).WillByDefault(Invoke([this](auto onChange) {
        return m_callbacks.registerCallback(std::move(onChange));
    }));
}
MockPropertyNode::~MockPropertyNode() = default;

MockPropertyAccessor::MockPropertyAccessor()
{
    ON_CALL(*this, registerOnChange(_)).WillByDefault(Invoke([this](auto onChange) {
        return m_callbacks.registerCallback(std::move(onChange));
    }));
}
MockPropertyAccessor::~MockPropertyAccessor() = default;

MockPropertyGroupAccessor::MockPropertyGroupAccessor()
{
    ON_CALL(*this, registerOnChange(_)).WillByDefault(Invoke([this](auto onChange) {
        return m_callbacks.registerCallback(std::move(onChange));
    }));
}
MockPropertyGroupAccessor::~MockPropertyGroupAccessor() = default;
