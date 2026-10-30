#include <im3e/property/properties.h>

#include <im3e/test_utils/test_utils.h>

using namespace im3e;

TEST(PropertyGroupTest, constructor)
{
    constexpr auto TestGroupName = "Test Group";
    const std::vector<std::shared_ptr<IProperty>> properties{
        std::make_shared<PropertyValue<int32_t>>(PropertyValueConfig<int32_t>{.name = "Int Property"}),
        std::make_shared<PropertyValue<bool>>(PropertyValueConfig<bool>{.name = "Bool Property"}),
        createPropertyGroup("Other Group", {}),
    };
    auto pProperty = createPropertyGroup(TestGroupName, properties);

    EXPECT_THAT(pProperty->getName(), StrEq(TestGroupName));
    EXPECT_THAT(pProperty->getChildren(), ContainerEq(properties));
}