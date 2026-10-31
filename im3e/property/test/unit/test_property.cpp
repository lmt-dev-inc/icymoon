#include <im3e/property/property.h>

#include <im3e/test_utils/glm.h>
#include <im3e/test_utils/test_utils.h>

using namespace im3e;

namespace {

class PropertyOwner
{
public:
    Property<float> factor{{
        .name = "Factor",
        .description = "This is a factor for testing.",
        .defaultValue = 10.0F,
        .minValue = 0.0F,
        .maxValue = 100.0F,
    }};
    Property<glm::vec3> position{{
        .name = "Position",
        .description = "Position for testing the use of glm in properties.",
        .defaultValue = glm::vec3{0.0F, 0.0F, 0.0F},
    }};
};

}  // namespace

TEST(PropertyTest, asMember)
{
    [[maybe_unused]] PropertyOwner propOwner;
    propOwner.factor.setValue(1.5F);
    const auto factor = propOwner.factor.getValue();
    // EXPECT_THAT(factor, FloatEq(1.5F));
}
