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

TEST(PropertyValueTTest, constructor)
{
    static constexpr PropertyValueTConfig<bool> BoolConfig{
        .name = "Boolean",
        .description = "Description of test property",
    };
    PropertyValueT<BoolConfig> property;
    EXPECT_THAT(property.getName(), StrEq(BoolConfig.name));
    EXPECT_THAT(property.getDescription(), StrEq(BoolConfig.description));
    EXPECT_THAT(property.getType() == typeid(bool), IsTrue());
    EXPECT_THAT(std::any_cast<bool>(property.getAnyValue()), Eq(bool{}));
    EXPECT_THAT(property.getAnyMinValue().has_value(), IsFalse());
    EXPECT_THAT(property.getAnyMaxValue().has_value(), IsFalse());
    EXPECT_THAT(property.getValue(), Eq(bool{}));
    EXPECT_THAT(property.getMinValue().has_value(), IsFalse());
    EXPECT_THAT(property.getMaxValue().has_value(), IsFalse());
}

TEST(PropertyValueTTest, constructorWithDefaultValue)
{
    static constexpr PropertyValueTConfig<uint32_t> UintConfig{
        .name = "Uint",
        .defaultValue = 42U,
    };
    PropertyValueT<UintConfig> property;
    EXPECT_THAT(property.getName(), StrEq(UintConfig.name));
    EXPECT_THAT(property.getType() == typeid(uint32_t), IsTrue());
    EXPECT_THAT(std::any_cast<uint32_t>(property.getAnyValue()), Eq(UintConfig.defaultValue.value()));
    EXPECT_THAT(property.getAnyMinValue().has_value(), IsFalse());
    EXPECT_THAT(property.getAnyMaxValue().has_value(), IsFalse());
    EXPECT_THAT(property.getValue(), Eq(UintConfig.defaultValue));
    EXPECT_THAT(property.getMinValue().has_value(), IsFalse());
    EXPECT_THAT(property.getMaxValue().has_value(), IsFalse());
}

TEST(PropertyValueTTest, constructorWithMinValue)
{
    static constexpr PropertyValueTConfig<float> FloatConfig{
        .defaultValue = -10.0F,
        .minValue = -5.9F,
    };
    PropertyValueT<FloatConfig> property;
    EXPECT_THAT(std::any_cast<float>(property.getAnyValue()), FloatEq(FloatConfig.minValue.value()));
    EXPECT_THAT(std::any_cast<float>(property.getAnyMinValue().value()), Eq(FloatConfig.minValue.value()));
    EXPECT_THAT(property.getAnyMaxValue().has_value(), IsFalse());
    EXPECT_THAT(property.getValue(), FloatEq(FloatConfig.minValue.value()));
    EXPECT_THAT(property.getMinValue(), Eq(FloatConfig.minValue));
    EXPECT_THAT(property.getMaxValue().has_value(), IsFalse());
}

TEST(PropertyValueTTest, constructorWithMaxValue)
{
    static constexpr PropertyValueTConfig<float> FloatConfig{
        .defaultValue = 100.0F,
        .maxValue = 25.0F,
    };
    PropertyValueT<FloatConfig> property;
    EXPECT_THAT(std::any_cast<float>(property.getAnyValue()), FloatEq(FloatConfig.maxValue.value()));
    EXPECT_THAT(property.getAnyMinValue().has_value(), IsFalse());
    EXPECT_THAT(std::any_cast<float>(property.getAnyMaxValue().value()), Eq(FloatConfig.maxValue.value()));
    EXPECT_THAT(property.getValue(), FloatEq(FloatConfig.maxValue.value()));
    EXPECT_THAT(property.getMinValue().has_value(), IsFalse());
    EXPECT_THAT(property.getMaxValue(), Eq(FloatConfig.maxValue));
}

TEST(PropertyValueTTest, setValue)
{
    static constexpr PropertyValueTConfig<float> FloatConfig{
        .defaultValue = 2.0F,
    };
    PropertyValueT<FloatConfig> property;
    EXPECT_THAT(property.getValue(), FloatEq(2.0F));

    OnChangeReceiver receiver(property);

    const float expectedValue = 5.5F;
    property.setValue(expectedValue);

    EXPECT_THAT(receiver.callCount, Eq(1U));
    EXPECT_THAT(property.getValue(), FloatEq(expectedValue));
}

TEST(PropertyValueTTest, setValueDoesNotNotifyIfGivenSameValue)
{
    constexpr uint32_t TestValue = 50U;

    static constexpr PropertyValueTConfig<uint32_t> IntConfig{
        .defaultValue = TestValue,
    };
    PropertyValueT<IntConfig> property;
    EXPECT_THAT(property.getValue(), Eq(TestValue));

    OnChangeReceiver receiver(property);

    property.setValue(TestValue);

    EXPECT_THAT(receiver.callCount, Eq(0U));
    EXPECT_THAT(property.getValue(), Eq(TestValue));
}

TEST(PropertyValueTTest, setAnyValue)
{
    static constexpr PropertyValueTConfig<int32_t> IntConfig{
        .defaultValue = -5,
    };
    PropertyValueT<IntConfig> property;
    EXPECT_THAT(any_cast<int32_t>(property.getAnyValue()), Eq(IntConfig.defaultValue.value()));

    OnChangeReceiver receiver(property);

    const int32_t expectedValue = 55;
    property.setAnyValue(expectedValue);

    EXPECT_THAT(receiver.callCount, Eq(1U));
    EXPECT_THAT(any_cast<int32_t>(property.getAnyValue()), Eq(expectedValue));
}

TEST(PropertyValueTTest, setAnyValueDoesNotNotifyIfGivenSameValue)
{
    constexpr int32_t TestValue = 65;

    static constexpr PropertyValueTConfig<int32_t> IntConfig{
        .defaultValue = TestValue,
    };
    PropertyValueT<IntConfig> property;
    EXPECT_THAT(any_cast<int32_t>(property.getAnyValue()), Eq(TestValue));

    OnChangeReceiver receiver(property);

    property.setAnyValue(TestValue);

    EXPECT_THAT(receiver.callCount, Eq(0U));
    EXPECT_THAT(any_cast<int32_t>(property.getAnyValue()), Eq(TestValue));
}
