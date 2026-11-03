#include <im3e/property/property_group.h>

#include <im3e/test_utils/test_utils.h>

using namespace im3e;

namespace {

auto ConfigEq(const PropertyGroup::Config& expected)
{
    using ConfigT = typename PropertyGroup::Config;
    return testing::AllOf(Field("name", &ConfigT::name, StrEq(expected.name)),
                          Field("description", &ConfigT::description, StrEq(expected.description)));
}

}  // namespace

TEST(PropertyGroupTest, constructor)
{
    const PropertyGroup::Config config{.name = "Test Group", .description = "Description"};
    PropertyGroup group{config, {}};
    EXPECT_THAT(group.getConfig(), ConfigEq(config));
    // EXPECT_THAT(group.getChildren(), ContainerEq({}));
}