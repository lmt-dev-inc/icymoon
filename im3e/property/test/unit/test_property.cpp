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
    EXPECT_THAT(property.getAccessor().expired(), IsFalse());

    const auto& constProperty = property;
    EXPECT_THAT(constProperty.getAccessor().expired(), IsFalse());
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
    EXPECT_THAT(property.getAccessor().expired(), IsFalse());
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
    MockFunction<void(const uint32_t&)> onChange;
    auto onChangeRegistration = property.registerOnChange(onChange.AsStdFunction());

    EXPECT_CALL(onChange, Call(3U));
    property.setValue(3U);
}

TEST(PropertyTest, registerOnChange_withNullFunction)
{
    Property<uint32_t> property{{.defaultValue = 4U}};
    EXPECT_THROW(auto registration = property.registerOnChange({}), std::invalid_argument);
}

TEST(PropertyTest, registerOnChange_canBeCancelled)
{
    Property<uint32_t> property{{.defaultValue = 2U}};
    MockFunction<void(const uint32_t&)> onChange;
    auto onChangeRegistration = property.registerOnChange(onChange.AsStdFunction());
    onChangeRegistration.reset();

    EXPECT_CALL(onChange, Call(_)).Times(0);
    property.setValue(3U);
}

TEST(PropertyTest, registerOnChange_callbackSeesNewValue)
{
    Property<int> property{{.defaultValue = 2}};
    MockFunction<void(const int&)> onChange;
    auto onChangeRegistration = property.registerOnChange(onChange.AsStdFunction());

    EXPECT_CALL(onChange, Call(3)).WillOnce([&] { EXPECT_THAT(property.getValue(), Eq(3)); });
    property.setValue(3);
}

TEST(PropertyTest, update)
{
    Property<int> property{{.defaultValue = 212}};
    MockFunction<void(const int&)> onChange;
    auto onChangeRegistration = property.registerOnChange(onChange.AsStdFunction());

    EXPECT_CALL(onChange, Call(222));
    property.update([](int value) {
        value += 10;
        return value;
    });
    EXPECT_THAT(property.getValue(), Eq(222));
}

TEST(PropertyTest, update_withNoChange)
{
    Property<int> property{{.defaultValue = 543}};
    MockFunction<void(const int&)> onChange;
    auto onChangeRegistration = property.registerOnChange(onChange.AsStdFunction());

    EXPECT_CALL(onChange, Call(_)).Times(0);
    property.update([](int value) { return value; });
    EXPECT_THAT(property.getValue(), Eq(543));
}

TEST(PropertyTest, update_withMinValue)
{
    Property<int> property{{.defaultValue = 543, .minValue = 10}};
    MockFunction<void(const int&)> onChange;
    auto onChangeRegistration = property.registerOnChange(onChange.AsStdFunction());

    EXPECT_CALL(onChange, Call(10));
    property.update([](int) { return 2; });
    EXPECT_THAT(property.getValue(), Eq(10));
}

TEST(PropertyTest, update_withMinValueAndNoChange)
{
    Property<int> property{{.defaultValue = 10, .minValue = 10}};
    MockFunction<void(const int&)> onChange;
    auto onChangeRegistration = property.registerOnChange(onChange.AsStdFunction());

    EXPECT_CALL(onChange, Call(_)).Times(0);
    property.update([](int) { return 2; });
    EXPECT_THAT(property.getValue(), Eq(10));
}

TEST(PropertyTest, update_withMaxValue)
{
    Property<int> property{{.defaultValue = 25, .maxValue = 100}};
    MockFunction<void(const int&)> onChange;
    auto onChangeRegistration = property.registerOnChange(onChange.AsStdFunction());

    EXPECT_CALL(onChange, Call(100));
    property.update([](int) { return 125; });
    EXPECT_THAT(property.getValue(), Eq(100));
}

TEST(PropertyTest, update_withMaxValueAndNoChange)
{
    Property<int> property{{.defaultValue = 100, .maxValue = 100}};
    MockFunction<void(const int&)> onChange;
    auto onChangeRegistration = property.registerOnChange(onChange.AsStdFunction());

    EXPECT_CALL(onChange, Call(_)).Times(0);
    property.update([](int) { return 125; });
    EXPECT_THAT(property.getValue(), Eq(100));
}

TEST(PropertyTest, update_withReadOnly)
{
    MockFunction<void(const int&)> mockOnChange;
    Property<int> property{{.defaultValue = 1, .isReadOnly = true}, mockOnChange.AsStdFunction()};

    // Read-only only prevents changes via the accessor, the owner can still change the value:
    EXPECT_CALL(mockOnChange, Call(2));
    property.update([](int value) { return value + 1; });
    EXPECT_THAT(property.getValue(), Eq(2));
}

TEST(PropertyTest, setValue)
{
    MockFunction<void(const int&)> mockOnChange;
    Property<int> property{{.defaultValue = 12}, mockOnChange.AsStdFunction()};

    EXPECT_CALL(mockOnChange, Call(24));
    property.setValue(24);
    EXPECT_THAT(property.getValue(), Eq(24));
}

TEST(PropertyTest, setValue_withNoChange)
{
    MockFunction<void(const int&)> mockOnChange;
    Property<int> property{{.defaultValue = 12}, mockOnChange.AsStdFunction()};

    EXPECT_CALL(mockOnChange, Call(_)).Times(0);
    property.setValue(12);
    EXPECT_THAT(property.getValue(), Eq(12));
}

TEST(PropertyTest, setValue_withMin)
{
    MockFunction<void(const int&)> mockOnChange;
    Property<int> property{{.defaultValue = 12, .minValue = 10}, mockOnChange.AsStdFunction()};

    EXPECT_CALL(mockOnChange, Call(10));
    property.setValue(8);
    EXPECT_THAT(property.getValue(), Eq(10));
}

TEST(PropertyTest, setValue_withMinAndNoChange)
{
    MockFunction<void(const int&)> mockOnChange;
    Property<int> property{{.defaultValue = 5, .minValue = 5}, mockOnChange.AsStdFunction()};

    // We should not receive a notification as the input value is below min and the current value is already min:
    EXPECT_CALL(mockOnChange, Call(_)).Times(0);
    property.setValue(3);
    EXPECT_THAT(property.getValue(), Eq(5));
}

TEST(PropertyTest, setValue_withMax)
{
    MockFunction<void(const int&)> mockOnChange;
    Property<int> property{{.defaultValue = 10, .maxValue = 12}, mockOnChange.AsStdFunction()};

    EXPECT_CALL(mockOnChange, Call(12));
    property.setValue(14);
    EXPECT_THAT(property.getValue(), Eq(12));
}

TEST(PropertyTest, setValue_withMaxAndNoChange)
{
    MockFunction<void(const int&)> mockOnChange;
    Property<int> property{{.defaultValue = 25, .maxValue = 25}, mockOnChange.AsStdFunction()};

    // We should not receive a notification as the input value is above max and the current value is already max:
    EXPECT_CALL(mockOnChange, Call(_)).Times(0);
    property.setValue(32);
    EXPECT_THAT(property.getValue(), Eq(25));
}

TEST(PropertyTest, setValue_withUnorderedType)
{
    MockFunction<void(const Unordered&)> mockOnChange;
    Property<Unordered> property{{.defaultValue = {1}}, mockOnChange.AsStdFunction()};

    EXPECT_CALL(mockOnChange, Call(Unordered{2}));
    property.setValue({2});
    EXPECT_THAT(property.getValue().value, Eq(2));
}

TEST(PropertyTest, setValue_withUnorderedTypeAndNoChange)
{
    MockFunction<void(const Unordered&)> mockOnChange;
    Property<Unordered> property{{.defaultValue = {1}}, mockOnChange.AsStdFunction()};

    EXPECT_CALL(mockOnChange, Call(_)).Times(0);
    property.setValue({1});
    EXPECT_THAT(property.getValue().value, Eq(1));
}

TEST(PropertyTest, setValue_withReadOnly)
{
    MockFunction<void(const int&)> mockOnChange;
    Property<int> property{{.defaultValue = 1, .isReadOnly = true}, mockOnChange.AsStdFunction()};

    // Read-only only prevents changes via the accessor, the owner can still change the value:
    EXPECT_CALL(mockOnChange, Call(2));
    property.setValue(2);
    EXPECT_THAT(property.getValue(), Eq(2));
}

TEST(PropertyTest, accessor)
{
    Property<int> property{{
        .name = "Test Property",
        .description = "Test Description",
        .defaultValue = 7,
    }};
    auto accessor = property.getAccessor().lock();
    ASSERT_THAT(accessor, NotNull());
    EXPECT_THAT(accessor->getName(), Eq("Test Property"));
    EXPECT_THAT(accessor->getDescription(), Eq("Test Description"));
    EXPECT_THAT(accessor->getType() == typeid(int), IsTrue());
    EXPECT_THAT(accessor->isReadOnly(), IsFalse());
    EXPECT_THAT(std::any_cast<int>(accessor->getAnyValue()), Eq(7));
    EXPECT_THAT(accessor->getAnyMinValue().has_value(), IsFalse());
    EXPECT_THAT(accessor->getAnyMaxValue().has_value(), IsFalse());
}

TEST(PropertyTest, accessor_withMinMax)
{
    Property<float> property{{
        .defaultValue = 0.5F,
        .minValue = 0.25F,
        .maxValue = 0.75F,
    }};
    auto accessor = property.getAccessor().lock();
    ASSERT_THAT(accessor, NotNull());
    EXPECT_THAT(accessor->getType() == typeid(float), IsTrue());

    const auto anyMinValueOpt = accessor->getAnyMinValue();
    ASSERT_THAT(anyMinValueOpt.has_value(), IsTrue());
    EXPECT_THAT(std::any_cast<float>(anyMinValueOpt.value()), FloatEq(0.25F));

    const auto anyMaxValueOpt = accessor->getAnyMaxValue();
    ASSERT_THAT(anyMaxValueOpt.has_value(), IsTrue());
    EXPECT_THAT(std::any_cast<float>(anyMaxValueOpt.value()), FloatEq(0.75F));
}

TEST(PropertyTest, accessor_withReadOnly)
{
    Property<int> property{{
        .defaultValue = 1,
        .isReadOnly = true,
    }};
    auto accessor = property.getAccessor().lock();
    ASSERT_THAT(accessor, NotNull());
    EXPECT_THAT(accessor->isReadOnly(), IsTrue());
}

TEST(PropertyTest, accessor_registerOnChange)
{
    Property<int> property{{.defaultValue = 7}};
    auto accessor = property.getAccessor().lock();
    MockFunction<void()> onChange;
    auto onChangeRegistration = accessor->registerOnChange(onChange.AsStdFunction());

    EXPECT_CALL(onChange, Call());
    property.setValue(8);
}

TEST(PropertyTest, setAnyValue)
{
    MockFunction<void(const int&)> mockOnChange;
    Property<int> property{{.defaultValue = 7}, mockOnChange.AsStdFunction()};
    auto accessor = property.getAccessor().lock();

    EXPECT_CALL(mockOnChange, Call(8));
    accessor->setAnyValue(8);
    EXPECT_THAT(property.getValue(), Eq(8));
}

TEST(PropertyTest, setAnyValue_withNoChange)
{
    MockFunction<void(const int&)> mockOnChange;
    Property<int> property{{.defaultValue = 4}, mockOnChange.AsStdFunction()};
    auto accessor = property.getAccessor().lock();

    EXPECT_CALL(mockOnChange, Call(_)).Times(0);
    accessor->setAnyValue(4);
    EXPECT_THAT(property.getValue(), Eq(4));
}

TEST(PropertyTest, setAnyValue_withMin)
{
    MockFunction<void(const int&)> mockOnChange;
    Property<int> property{{.defaultValue = 12, .minValue = 10}, mockOnChange.AsStdFunction()};
    auto accessor = property.getAccessor().lock();

    EXPECT_CALL(mockOnChange, Call(10));
    accessor->setAnyValue(8);
    EXPECT_THAT(property.getValue(), Eq(10));
}

TEST(PropertyTest, setAnyValue_withMax)
{
    MockFunction<void(const int&)> mockOnChange;
    Property<int> property{{.defaultValue = 10, .maxValue = 12}, mockOnChange.AsStdFunction()};
    auto accessor = property.getAccessor().lock();

    EXPECT_CALL(mockOnChange, Call(12));
    accessor->setAnyValue(14);
    EXPECT_THAT(property.getValue(), Eq(12));
}

TEST(PropertyTest, setAnyValue_withWrongType)
{
    MockFunction<void(const float&)> mockOnChange;
    Property<float> property{{.defaultValue = 0.5F}, mockOnChange.AsStdFunction()};
    auto accessor = property.getAccessor().lock();

    // A double is not implicitly converted to float when stored in std::any:
    EXPECT_CALL(mockOnChange, Call(_)).Times(0);
    EXPECT_THROW(accessor->setAnyValue(1.2), std::bad_any_cast);
    EXPECT_THAT(property.getValue(), FloatEq(0.5F));
}

TEST(PropertyTest, setAnyValue_withReadOnly)
{
    MockFunction<void(const int&)> mockOnChange;
    Property<int> property{{.defaultValue = 1, .isReadOnly = true}, mockOnChange.AsStdFunction()};
    auto accessor = property.getAccessor().lock();

    EXPECT_CALL(mockOnChange, Call(_)).Times(0);
    EXPECT_THROW(accessor->setAnyValue(2), std::logic_error);
    EXPECT_THAT(property.getValue(), Eq(1));
}

TEST(PropertyTest, accessorExpiresWithProperty)
{
    std::weak_ptr<IPropertyAccessor> accessor;
    {
        Property<int> property{{.defaultValue = 1}};
        accessor = property.getAccessor();
        EXPECT_THAT(accessor.expired(), IsFalse());
    }
    EXPECT_THAT(accessor.expired(), IsTrue());
}

TEST(PropertyTest, accessorOutlivesProperty)
{
    MockFunction<void(const float&)> mockOnChange;
    MockFunction<void(const float&)> registeredOnChange;
    CallbackRegistration onChangeRegistration;

    std::shared_ptr<IPropertyAccessor> accessor;
    {
        Property<float> property{{.defaultValue = 0.5F}, mockOnChange.AsStdFunction()};
        onChangeRegistration = property.registerOnChange(registeredOnChange.AsStdFunction());
        accessor = property.getAccessor().lock();
        EXPECT_THAT(accessor, NotNull());
    }

    // Callbacks fire for as long as their registration is alive. mockOnChange's registration was owned by Property,
    // so it no longer fires. registeredOnChange's registration is owned by this test, so it keeps firing: the
    // accessor keeps working as a standalone value while it is referenced.
    EXPECT_CALL(mockOnChange, Call(_)).Times(0);
    EXPECT_CALL(registeredOnChange, Call(1.2F));
    accessor->setAnyValue(1.2F);
    EXPECT_THAT(std::any_cast<float>(accessor->getAnyValue()), FloatEq(1.2F));
}
