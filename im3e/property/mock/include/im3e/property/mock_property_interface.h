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
    MOCK_METHOD(const std::string&, getDescription, (), (const, override));

    void notifyChanged() { m_callbacks.invokeAll(); }
    [[nodiscard]] auto getCallbackCount() const -> size_t { return m_callbacks.getCallbackCount(); }

private:
    mutable CallbackRegistry<> m_callbacks;
};

class MockPropertyAccessor : public IPropertyAccessor
{
public:
    MockPropertyAccessor();
    ~MockPropertyAccessor() override;

    MOCK_METHOD(CallbackRegistration, registerOnChange, (std::function<void()>), (const, override));
    MOCK_METHOD(const std::string&, getName, (), (const, override));
    MOCK_METHOD(const std::string&, getDescription, (), (const, override));

    MOCK_METHOD(void, setAnyValue, (std::any), (override));
    MOCK_METHOD(bool, isReadOnly, (), (const, override));
    MOCK_METHOD(std::type_index, getType, (), (const, override));
    MOCK_METHOD(std::any, getAnyValue, (), (const, override));
    MOCK_METHOD(std::optional<std::any>, getAnyMinValue, (), (const, override));
    MOCK_METHOD(std::optional<std::any>, getAnyMaxValue, (), (const, override));

    void notifyChanged() { m_callbacks.invokeAll(); }
    [[nodiscard]] auto getCallbackCount() const -> size_t { return m_callbacks.getCallbackCount(); }

private:
    mutable CallbackRegistry<> m_callbacks;
};

class MockPropertyGroupAccessor : public IPropertyGroupAccessor
{
public:
    MockPropertyGroupAccessor();
    ~MockPropertyGroupAccessor() override;

    MOCK_METHOD(CallbackRegistration, registerOnChange, (std::function<void()>), (const, override));
    MOCK_METHOD(const std::string&, getName, (), (const, override));
    MOCK_METHOD(const std::string&, getDescription, (), (const, override));

    MOCK_METHOD((std::vector<std::weak_ptr<IPropertyNode>>), getChildren, (), (const, override));

    void notifyChanged() { m_callbacks.invokeAll(); }
    [[nodiscard]] auto getCallbackCount() const -> size_t { return m_callbacks.getCallbackCount(); }

private:
    mutable CallbackRegistry<> m_callbacks;
};

}  // namespace im3e
