#include "properties.h"

#include <im3e/test_utils/test_utils.h>

using namespace im3e;

namespace {

struct OnChangeReceiver
{
    OnChangeReceiver(IProperty& rProperty) { rProperty.registerOnChange(pOnChange); }

    uint32_t callCount{};
    std::shared_ptr<std::function<void()>> pOnChange = std::make_shared<std::function<void()>>([this] { callCount++; });
};

}  // namespace

TEST(PropertyValueTest, constructor)
{
    const PropertyValueConfig<bool> boolConfig{
        .name = "Boolean",
        .description = "Description of test property",
    };
    PropertyValue<bool> property(boolConfig);
    EXPECT_THAT(property.getName(), StrEq(boolConfig.name));
    EXPECT_THAT(property.getDescription(), StrEq(boolConfig.description));
    EXPECT_THAT(property.getType() == typeid(bool), IsTrue());
    EXPECT_THAT(std::any_cast<bool>(property.getAnyValue()), Eq(bool{}));
    EXPECT_THAT(property.getAnyMinValue().has_value(), IsFalse());
    EXPECT_THAT(property.getAnyMaxValue().has_value(), IsFalse());
    EXPECT_THAT(property.getValue(), Eq(bool{}));
    EXPECT_THAT(property.getMinValue().has_value(), IsFalse());
    EXPECT_THAT(property.getMaxValue().has_value(), IsFalse());
}

TEST(PropertyValueTest, constructorWithDefaultValue)
{
    const PropertyValueConfig<uint32_t> uintConfig{
        .name = "Uint",
        .defaultValue = 42U,
    };
    PropertyValue<uint32_t> property(uintConfig);
    EXPECT_THAT(property.getName(), StrEq(uintConfig.name));
    EXPECT_THAT(property.getType() == typeid(uint32_t), IsTrue());
    EXPECT_THAT(std::any_cast<uint32_t>(property.getAnyValue()), Eq(uintConfig.defaultValue));
    EXPECT_THAT(property.getAnyMinValue().has_value(), IsFalse());
    EXPECT_THAT(property.getAnyMaxValue().has_value(), IsFalse());
    EXPECT_THAT(property.getValue(), Eq(uintConfig.defaultValue));
    EXPECT_THAT(property.getMinValue().has_value(), IsFalse());
    EXPECT_THAT(property.getMaxValue().has_value(), IsFalse());
}

TEST(PropertyValueTest, constructorWithMinValue)
{
    const PropertyValueConfig<float> floatConfig{
        .defaultValue = -10.0F,
        .minValue = -5.9F,
    };
    PropertyValue<float> property(floatConfig);
    EXPECT_THAT(std::any_cast<float>(property.getAnyValue()), FloatEq(floatConfig.minValue.value()));
    EXPECT_THAT(std::any_cast<float>(property.getAnyMinValue().value()), Eq(floatConfig.minValue.value()));
    EXPECT_THAT(property.getAnyMaxValue().has_value(), IsFalse());
    EXPECT_THAT(property.getValue(), FloatEq(floatConfig.minValue.value()));
    EXPECT_THAT(property.getMinValue(), Eq(floatConfig.minValue));
    EXPECT_THAT(property.getMaxValue().has_value(), IsFalse());
}

TEST(PropertyValueTest, constructorWithMaxValue)
{
    const PropertyValueConfig<float> floatConfig{
        .defaultValue = 100.0F,
        .maxValue = 25.0F,
    };
    PropertyValue<float> property(floatConfig);
    EXPECT_THAT(std::any_cast<float>(property.getAnyValue()), FloatEq(floatConfig.maxValue.value()));
    EXPECT_THAT(property.getAnyMinValue().has_value(), IsFalse());
    EXPECT_THAT(std::any_cast<float>(property.getAnyMaxValue().value()), Eq(floatConfig.maxValue.value()));
    EXPECT_THAT(property.getValue(), FloatEq(floatConfig.maxValue.value()));
    EXPECT_THAT(property.getMinValue().has_value(), IsFalse());
    EXPECT_THAT(property.getMaxValue(), Eq(floatConfig.maxValue));
}

TEST(PropertyValueTest, setValue)
{
    const PropertyValueConfig<float> floatConfig{
        .defaultValue = 2.0F,
    };
    PropertyValue<float> property(floatConfig);
    EXPECT_THAT(property.getValue(), FloatEq(2.0F));

    OnChangeReceiver receiver(property);

    const float expectedValue = 5.5F;
    property.setValue(expectedValue);

    EXPECT_THAT(receiver.callCount, Eq(1U));
    EXPECT_THAT(property.getValue(), FloatEq(expectedValue));
}

TEST(PropertyValueTest, setValueDoesNotNotifyIfGivenSameValue)
{
    constexpr uint32_t TestValue = 50U;

    const PropertyValueConfig<uint32_t> intConfig{
        .defaultValue = TestValue,
    };
    PropertyValue<uint32_t> property(intConfig);
    EXPECT_THAT(property.getValue(), Eq(TestValue));

    OnChangeReceiver receiver(property);

    property.setValue(TestValue);

    EXPECT_THAT(receiver.callCount, Eq(0U));
    EXPECT_THAT(property.getValue(), Eq(TestValue));
}

TEST(PropertyValueTest, setAnyValue)
{
    const PropertyValueConfig<int32_t> intConfig{
        .defaultValue = -5,
    };
    PropertyValue<int32_t> property(intConfig);
    EXPECT_THAT(any_cast<int32_t>(property.getAnyValue()), Eq(intConfig.defaultValue));

    OnChangeReceiver receiver(property);

    const int32_t expectedValue = 55;
    property.setAnyValue(expectedValue);

    EXPECT_THAT(receiver.callCount, Eq(1U));
    EXPECT_THAT(any_cast<int32_t>(property.getAnyValue()), Eq(expectedValue));
}

TEST(PropertyValueTest, setAnyValueDoesNotNotifyIfGivenSameValue)
{
    constexpr int32_t TestValue = 65;

    const PropertyValueConfig<int32_t> intConfig{
        .defaultValue = TestValue,
    };
    PropertyValue<int32_t> property(intConfig);
    EXPECT_THAT(any_cast<int32_t>(property.getAnyValue()), Eq(TestValue));

    OnChangeReceiver receiver(property);

    property.setAnyValue(TestValue);

    EXPECT_THAT(receiver.callCount, Eq(0U));
    EXPECT_THAT(any_cast<int32_t>(property.getAnyValue()), Eq(TestValue));
}
