#include <im3e/property/property.h>

#include <im3e/test_utils/test_utils.h>

using namespace im3e;

namespace {

template <typename T>
auto ConfigEq(const typename Property<T>::Config& expected)
{
    using ConfigT = typename Property<T>::Config;
    return testing::AllOf(Field("name", &ConfigT::name, Eq(expected.name)),
                          Field("description", &ConfigT::description, Eq(expected.description)),
                          Field("defaultValue", &ConfigT::defaultValue, Eq(expected.defaultValue)),
                          Field("minValue", &ConfigT::minValue, Eq(expected.minValue)),
                          Field("maxValue", &ConfigT::maxValue, Eq(expected.maxValue)),
                          Field("isReadOnly", &ConfigT::isReadOnly, Eq(expected.isReadOnly)));
}

// Type that can be compared for equality but not ordered, so min/max clamping cannot apply:
struct Unordered
{
    int value;
    bool operator==(const Unordered&) const = default;
};

}  // namespace

TEST(PropertyTest, constructor)
{
    const auto config = Property<int>::Config{
        .name = "Test Property",
        .description = "Used for testing construction",
        .defaultValue = 42,
    };
    auto property = Property<int>{config};
    EXPECT_THAT(property.getConfig(), ConfigEq<int>(config));
    EXPECT_THAT(property.getValue(), Eq(42));
    EXPECT_THAT(property.getInterface().expired(), IsFalse());
}

TEST(PropertyTest, constructor_withMinMax)
{
    const auto config = Property<float>::Config{
        .name = "Test",
        .description = "Floating-point value",
        .defaultValue = 0.5F,
        .minValue = 0.0F,
        .maxValue = 1.0F,
    };
    auto property = Property<float>(config);
    EXPECT_THAT(property.getConfig(), ConfigEq<float>(config));
    EXPECT_THAT(property.getValue(), FloatEq(0.5F));
    EXPECT_THAT(property.getInterface().expired(), IsFalse());
}

TEST(PropertyTest, constructor_withMin)
{
    auto property = Property<float>{{
        .defaultValue = 1.0F,
        .minValue = 2.0F,
    }};
    EXPECT_THAT(property.getValue(), FloatEq(2.0F));
    EXPECT_THAT(property.getConfig().defaultValue, FloatEq(1.0F));
}

TEST(PropertyTest, constructor_withMax)
{
    auto property = Property<float>{{
        .defaultValue = 10.0F,
        .maxValue = 5.0F,
    }};
    EXPECT_THAT(property.getValue(), FloatEq(5.0F));
    EXPECT_THAT(property.getConfig().defaultValue, FloatEq(10.0F));
}

TEST(PropertyTest, constructor_withMinGreaterThanMax)
{
    EXPECT_THROW((Property<int>{{.defaultValue = 5, .minValue = 10, .maxValue = 0}}), std::invalid_argument);
}

TEST(PropertyTest, constructor_withUnorderedType)
{
    Property<Unordered> property{{.defaultValue = Unordered{.value = 2}}};
    EXPECT_THAT(property.getValue().value, Eq(2));
}

TEST(PropertyTest, constructor_withUnorderedTypeAndMin)
{
    EXPECT_THROW(Property<Unordered> property{{.minValue = Unordered{.value = 2}}}, std::invalid_argument);
}

TEST(PropertyTest, constructor_withUnorderedTypeAndMax)
{
    EXPECT_THROW(Property<Unordered> property{{.maxValue = Unordered{.value = 2}}}, std::invalid_argument);
}

// Regression test where defaultValue used to be incorrectly moved at construction:
TEST(PropertyTest, constructor_keepsDefaultValueInConfig)
{
    auto property = Property<std::string>{{.defaultValue = "hello"}};
    EXPECT_THAT(property.getConfig().defaultValue, Eq("hello"));
    EXPECT_THAT(property.getValue(), Eq("hello"));
}

TEST(PropertyTest, registerOnChange)
{
    Property<uint32_t> property{{.defaultValue = 2U}};
    MockFunction<void()> onChange;
    auto onChangeRegistration = property.registerOnChange(onChange.AsStdFunction());

    EXPECT_CALL(onChange, Call());
    property.setValue(3U);
}

TEST(PropertyTest, registerOnChange_withNullFunction)
{
    Property<uint32_t> property{{.defaultValue = 4U}};
    EXPECT_THROW(property.registerOnChange({}), std::invalid_argument);
}

TEST(PropertyTest, registerOnChange_canBeCancelled)
{
    Property<uint32_t> property{{.defaultValue = 2U}};
    MockFunction<void()> onChange;
    auto onChangeRegistration = property.registerOnChange(onChange.AsStdFunction());
    onChangeRegistration.reset();

    EXPECT_CALL(onChange, Call()).Times(0);
    property.setValue(3U);
}

TEST(PropertyTest, registerOnChange_callbackSeesNewValue)
{
    Property<int> property{{.defaultValue = 2}};
    MockFunction<void()> onChange;
    auto onChangeRegistration = property.registerOnChange(onChange.AsStdFunction());

    EXPECT_CALL(onChange, Call()).WillOnce([&] { EXPECT_THAT(property.getValue(), Eq(3)); });
    property.setValue(3);
}

TEST(PropertyTest, update)
{
    Property<int> property{{.defaultValue = 212}};
    MockFunction<void()> onChange;
    auto onChangeRegistration = property.registerOnChange(onChange.AsStdFunction());

    EXPECT_CALL(onChange, Call());
    property.update([](int value) {
        value += 10;
        return value;
    });
    EXPECT_THAT(property.getValue(), Eq(222));
}

TEST(PropertyTest, update_withNoChange)
{
    Property<int> property{{.defaultValue = 543}};
    MockFunction<void()> onChange;
    auto onChangeRegistration = property.registerOnChange(onChange.AsStdFunction());

    EXPECT_CALL(onChange, Call()).Times(0);
    property.update([](int value) { return value; });
    EXPECT_THAT(property.getValue(), Eq(543));
}

TEST(PropertyTest, update_withMinValue)
{
    Property<int> property{{.defaultValue = 543, .minValue = 10}};
    MockFunction<void()> onChange;
    auto onChangeRegistration = property.registerOnChange(onChange.AsStdFunction());

    EXPECT_CALL(onChange, Call());
    property.update([](int) { return 2; });
    EXPECT_THAT(property.getValue(), Eq(10));
}

TEST(PropertyTest, update_withMinValueAndNoChange)
{
    Property<int> property{{.defaultValue = 10, .minValue = 10}};
    MockFunction<void()> onChange;
    auto onChangeRegistration = property.registerOnChange(onChange.AsStdFunction());

    EXPECT_CALL(onChange, Call()).Times(0);
    property.update([](int) { return 2; });
    EXPECT_THAT(property.getValue(), Eq(10));
}

TEST(PropertyTest, update_withMaxValue)
{
    Property<int> property{{.defaultValue = 25, .maxValue = 100}};
    MockFunction<void()> onChange;
    auto onChangeRegistration = property.registerOnChange(onChange.AsStdFunction());

    EXPECT_CALL(onChange, Call());
    property.update([](int) { return 125; });
    EXPECT_THAT(property.getValue(), Eq(100));
}

TEST(PropertyTest, update_withMaxValueAndNoChange)
{
    Property<int> property{{.defaultValue = 100, .maxValue = 100}};
    MockFunction<void()> onChange;
    auto onChangeRegistration = property.registerOnChange(onChange.AsStdFunction());

    EXPECT_CALL(onChange, Call()).Times(0);
    property.update([](int) { return 125; });
    EXPECT_THAT(property.getValue(), Eq(100));
}

TEST(PropertyTest, update_withReadOnly)
{
    MockFunction<void()> mockOnChange;
    Property<int> property{{.defaultValue = 1, .isReadOnly = true}, mockOnChange.AsStdFunction()};

    // Read-only only prevents changes via the interface, the owner can still change the value:
    EXPECT_CALL(mockOnChange, Call());
    property.update([](int value) { return value + 1; });
    EXPECT_THAT(property.getValue(), Eq(2));
}

TEST(PropertyTest, setValue)
{
    MockFunction<void()> mockOnChange;
    Property<int> property{{.defaultValue = 12}, mockOnChange.AsStdFunction()};

    EXPECT_CALL(mockOnChange, Call());
    property.setValue(24);
    EXPECT_THAT(property.getValue(), Eq(24));
}

TEST(PropertyTest, setValue_withNoChange)
{
    MockFunction<void()> mockOnChange;
    Property<int> property{{.defaultValue = 12}, mockOnChange.AsStdFunction()};

    EXPECT_CALL(mockOnChange, Call()).Times(0);
    property.setValue(12);
    EXPECT_THAT(property.getValue(), Eq(12));
}

TEST(PropertyTest, setValue_withMin)
{
    MockFunction<void()> mockOnChange;
    Property<int> property{{.defaultValue = 12, .minValue = 10}, mockOnChange.AsStdFunction()};

    EXPECT_CALL(mockOnChange, Call());
    property.setValue(8);
    EXPECT_THAT(property.getValue(), Eq(10));
}

TEST(PropertyTest, setValue_withMinAndNoChange)
{
    MockFunction<void()> mockOnChange;
    Property<int> property{{.defaultValue = 5, .minValue = 5}, mockOnChange.AsStdFunction()};

    // We should not receive a notification as the input value is below min and the current value is already min:
    EXPECT_CALL(mockOnChange, Call()).Times(0);
    property.setValue(3);
    EXPECT_THAT(property.getValue(), Eq(5));
}

TEST(PropertyTest, setValue_withMax)
{
    MockFunction<void()> mockOnChange;
    Property<int> property{{.defaultValue = 10, .maxValue = 12}, mockOnChange.AsStdFunction()};

    EXPECT_CALL(mockOnChange, Call());
    property.setValue(14);
    EXPECT_THAT(property.getValue(), Eq(12));
}

TEST(PropertyTest, setValue_withMaxAndNoChange)
{
    MockFunction<void()> mockOnChange;
    Property<int> property{{.defaultValue = 25, .maxValue = 25}, mockOnChange.AsStdFunction()};

    // We should not receive a notification as the input value is above max and the current value is already max:
    EXPECT_CALL(mockOnChange, Call()).Times(0);
    property.setValue(32);
    EXPECT_THAT(property.getValue(), Eq(25));
}

TEST(PropertyTest, setValue_withUnorderedType)
{
    MockFunction<void()> mockOnChange;
    Property<Unordered> property{{.defaultValue = {1}}, mockOnChange.AsStdFunction()};

    EXPECT_CALL(mockOnChange, Call());
    property.setValue({2});
    EXPECT_THAT(property.getValue().value, Eq(2));
}

TEST(PropertyTest, setValue_withUnorderedTypeAndNoChange)
{
    MockFunction<void()> mockOnChange;
    Property<Unordered> property{{.defaultValue = {1}}, mockOnChange.AsStdFunction()};

    EXPECT_CALL(mockOnChange, Call()).Times(0);
    property.setValue({1});
    EXPECT_THAT(property.getValue().value, Eq(1));
}

TEST(PropertyTest, setValue_withReadOnly)
{
    MockFunction<void()> mockOnChange;
    Property<int> property{{.defaultValue = 1, .isReadOnly = true}, mockOnChange.AsStdFunction()};

    // Read-only only prevents changes via the interface, the owner can still change the value:
    EXPECT_CALL(mockOnChange, Call());
    property.setValue(2);
    EXPECT_THAT(property.getValue(), Eq(2));
}

TEST(PropertyTest, interface)
{
    Property<int> property{{.name = "Test Property", .defaultValue = 7}};
    auto interface = property.getInterface().lock();
    ASSERT_THAT(interface, NotNull());
    EXPECT_THAT(interface->getName(), Eq("Test Property"));
    EXPECT_THAT(std::any_cast<int>(interface->getAnyValue()), Eq(7));
}

TEST(PropertyTest, interface_registerOnChange)
{
    Property<int> property{{.defaultValue = 7}};
    auto interface = property.getInterface().lock();
    MockFunction<void()> onChange;
    auto onChangeRegistration = interface->registerOnChange(onChange.AsStdFunction());

    EXPECT_CALL(onChange, Call());
    property.setValue(8);
}

TEST(PropertyTest, setAnyValue)
{
    MockFunction<void()> mockOnChange;
    Property<int> property{{.defaultValue = 7}, mockOnChange.AsStdFunction()};
    auto interface = property.getInterface().lock();

    EXPECT_CALL(mockOnChange, Call());
    interface->setAnyValue(8);
    EXPECT_THAT(property.getValue(), Eq(8));
}

TEST(PropertyTest, setAnyValue_withNoChange)
{
    MockFunction<void()> mockOnChange;
    Property<int> property{{.defaultValue = 4}, mockOnChange.AsStdFunction()};
    auto interface = property.getInterface().lock();

    EXPECT_CALL(mockOnChange, Call()).Times(0);
    interface->setAnyValue(4);
    EXPECT_THAT(property.getValue(), Eq(4));
}

TEST(PropertyTest, setAnyValue_withMin)
{
    MockFunction<void()> mockOnChange;
    Property<int> property{{.defaultValue = 12, .minValue = 10}, mockOnChange.AsStdFunction()};
    auto interface = property.getInterface().lock();

    EXPECT_CALL(mockOnChange, Call());
    interface->setAnyValue(8);
    EXPECT_THAT(property.getValue(), Eq(10));
}

TEST(PropertyTest, setAnyValue_withMax)
{
    MockFunction<void()> mockOnChange;
    Property<int> property{{.defaultValue = 10, .maxValue = 12}, mockOnChange.AsStdFunction()};
    auto interface = property.getInterface().lock();

    EXPECT_CALL(mockOnChange, Call());
    interface->setAnyValue(14);
    EXPECT_THAT(property.getValue(), Eq(12));
}

TEST(PropertyTest, setAnyValue_withWrongType)
{
    MockFunction<void()> mockOnChange;
    Property<float> property{{.defaultValue = 0.5F}, mockOnChange.AsStdFunction()};
    auto interface = property.getInterface().lock();

    // A double is not implicitly converted to float when stored in std::any:
    EXPECT_CALL(mockOnChange, Call()).Times(0);
    EXPECT_THROW(interface->setAnyValue(1.2), std::bad_any_cast);
    EXPECT_THAT(property.getValue(), FloatEq(0.5F));
}

TEST(PropertyTest, setAnyValue_withReadOnly)
{
    MockFunction<void()> mockOnChange;
    Property<int> property{{.defaultValue = 1, .isReadOnly = true}, mockOnChange.AsStdFunction()};
    auto interface = property.getInterface().lock();

    EXPECT_CALL(mockOnChange, Call()).Times(0);
    EXPECT_THROW(interface->setAnyValue(2), std::logic_error);
    EXPECT_THAT(property.getValue(), Eq(1));
}

TEST(PropertyTest, interfaceExpiresWithProperty)
{
    std::weak_ptr<IProperty> interface;
    {
        Property<int> property{{.defaultValue = 1}};
        interface = property.getInterface();
        EXPECT_THAT(interface.expired(), IsFalse());
    }
    EXPECT_THAT(interface.expired(), IsTrue());
}

TEST(PropertyTest, interfaceOutlivesProperty)
{
    MockFunction<void()> mockOnChange;
    MockFunction<void()> registeredOnChange;
    CallbackRegistration onChangeRegistration;

    std::shared_ptr<IProperty> interface;
    {
        Property<float> property{{.defaultValue = 0.5F}, mockOnChange.AsStdFunction()};
        onChangeRegistration = property.registerOnChange(registeredOnChange.AsStdFunction());
        interface = property.getInterface().lock();
        EXPECT_THAT(interface, NotNull());
    }

    // Callbacks fire for as long as their registration is alive. mockOnChange's registration was owned by Property,
    // so it no longer fires. registeredOnChange's registration is owned by this test, so it keeps firing: the
    // interface keeps working as a standalone value while it is referenced.
    EXPECT_CALL(mockOnChange, Call()).Times(0);
    EXPECT_CALL(registeredOnChange, Call());
    interface->setAnyValue(1.2F);
    EXPECT_THAT(std::any_cast<float>(interface->getAnyValue()), FloatEq(1.2F));
}
