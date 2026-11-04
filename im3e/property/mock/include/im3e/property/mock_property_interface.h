#pragma once

#include <im3e/property/property_interface.h>
#include <im3e/test_utils/test_utils.h>

namespace im3e {

// The mocks register callbacks in a real CallbackRegistry by default, so that registrations behave as with a real
// property: a callback stops being called once its registration is released. notifyChanged() simulates a change.

class MockPropertyNode : public IPropertyNode
{
public:
    MockPropertyNode();
    ~MockPropertyNode() override;

    MOCK_METHOD(CallbackRegistration, registerOnChange, (std::function<void()>), (const, override));
    MOCK_METHOD(const std::string&, getName, (), (const, override));

    void notifyChanged() { m_callbacks.invokeAll(); }

private:
    mutable CallbackRegistry<> m_callbacks;
};

class MockProperty : public IProperty
{
public:
    MockProperty();
    ~MockProperty() override;

    MOCK_METHOD(CallbackRegistration, registerOnChange, (std::function<void()>), (const, override));
    MOCK_METHOD(const std::string&, getName, (), (const, override));

    MOCK_METHOD(void, setAnyValue, (std::any), (override));
    MOCK_METHOD(std::any, getAnyValue, (), (const, override));

    void notifyChanged() { m_callbacks.invokeAll(); }

private:
    mutable CallbackRegistry<> m_callbacks;
};

class MockPropertyGroup : public IPropertyGroup
{
public:
    MockPropertyGroup();
    ~MockPropertyGroup() override;

    MOCK_METHOD(CallbackRegistration, registerOnChange, (std::function<void()>), (const, override));
    MOCK_METHOD(const std::string&, getName, (), (const, override));

    MOCK_METHOD((std::vector<std::weak_ptr<IPropertyNode>>), getChildren, (), (const, override));

    void notifyChanged() { m_callbacks.invokeAll(); }

private:
    mutable CallbackRegistry<> m_callbacks;
};

}  // namespace im3e
