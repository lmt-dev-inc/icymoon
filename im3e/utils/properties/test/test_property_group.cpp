#include "properties.h"

#include <im3e/test_utils/test_utils.h>

using namespace im3e;

namespace {

constexpr PropertyValueTConfig<int32_t> IntPropertyConfig{.name = "Int Property"};
constexpr PropertyValueTConfig<bool> BoolPropertyConfig{.name = "Bool Property"};

}  // namespace

TEST(PropertyGroupTest, constructor)
{
    constexpr auto TestGroupName = "Test Group";
    const std::vector<std::shared_ptr<IProperty>> properties{
        std::make_shared<PropertyValueT<IntPropertyConfig>>(),
        std::make_shared<PropertyValueT<BoolPropertyConfig>>(),
        createPropertyGroup("Other Group", {}),
    };
    auto pProperty = createPropertyGroup(TestGroupName, properties);

    EXPECT_THAT(pProperty->getName(), StrEq(TestGroupName));
    EXPECT_THAT(pProperty->getChildren(), ContainerEq(properties));
}