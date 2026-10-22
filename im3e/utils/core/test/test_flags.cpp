#include <im3e/utils/core/flags.h>

#include <im3e/test_utils/test_utils.h>

using namespace im3e;

namespace {

enum class TestEnum : uint32_t
{
    Value1 = 1U << 0U,
    Value2 = 1U << 1U,
    Value3 = 1U << 2U,
    Value4 = 1U << 3U
};

}  // namespace

namespace im3e {

template <>
inline constexpr bool IsFlagEnum<TestEnum> = true;

}  // namespace im3e

TEST(FlagsTest, defaultConstructor)
{
    const Flags<TestEnum> flags;
    EXPECT_THAT(flags.isEmpty(), IsTrue());
    EXPECT_THAT(flags.has(TestEnum::Value1), IsFalse());
    EXPECT_THAT(flags.has(TestEnum::Value2), IsFalse());
    EXPECT_THAT(flags.has(TestEnum::Value3), IsFalse());
    EXPECT_THAT(flags.has(TestEnum::Value4), IsFalse());
}

TEST(FlagsTest, constructor)
{
    const Flags flags(TestEnum::Value2);
    EXPECT_THAT(flags.isEmpty(), IsFalse());
    EXPECT_THAT(flags.has(TestEnum::Value1), IsFalse());
    EXPECT_THAT(flags.has(TestEnum::Value2), IsTrue());
    EXPECT_THAT(flags.has(TestEnum::Value3), IsFalse());
    EXPECT_THAT(flags.has(TestEnum::Value4), IsFalse());
}

TEST(FlagsTest, has)
{
    const Flags flags = TestEnum::Value1 | TestEnum::Value3;
    EXPECT_THAT(flags.has(TestEnum::Value1), IsTrue());
    EXPECT_THAT(flags.has(TestEnum::Value2), IsFalse());
    EXPECT_THAT(flags.has(TestEnum::Value3), IsTrue());
    EXPECT_THAT(flags.has(TestEnum::Value4), IsFalse());
}

TEST(FlagsTest, hasAll)
{
    const Flags flags = TestEnum::Value1 | TestEnum::Value4;
    EXPECT_THAT(flags.hasAll(TestEnum::Value1 | TestEnum::Value4), IsTrue());
    EXPECT_THAT(flags.hasAll(TestEnum::Value1 | TestEnum::Value2), IsFalse());
}

TEST(FlagsTest, hasAny)
{
    const Flags flags = TestEnum::Value2 | TestEnum::Value3;
    EXPECT_THAT(flags.hasAny(TestEnum::Value1 | TestEnum::Value2), IsTrue());
    EXPECT_THAT(flags.hasAny(TestEnum::Value2 | TestEnum::Value3), IsTrue());
    EXPECT_THAT(flags.hasAny(TestEnum::Value3 | TestEnum::Value4), IsTrue());
    EXPECT_THAT(flags.hasAny(TestEnum::Value1 | TestEnum::Value4), IsFalse());
    EXPECT_THAT(flags.hasAny(TestEnum::Value1), IsFalse());
}
