#include "math_utils.h"

#include <im3e/test_utils/glm.h>
#include <im3e/test_utils/test_utils.h>

#include <fmt/format.h>
#include <gmock/gmock.h>

#include <iostream>

using namespace im3e;

TEST(MathUtilsTest, alignUpSucceeds)
{
    EXPECT_THAT(alignUp(32U, 32U), Eq(32U));
    EXPECT_THAT(alignUp(45U, 64U), Eq(64U));
    EXPECT_THAT(alignUp(64U, 64U), Eq(64U));
    EXPECT_THAT(alignUp(128U, 64U), Eq(128U));
}

TEST(MathUtilsTest, toNormalizedVecSucceeds)
{
    EXPECT_THAT(toNormalizedVec(glm::u8vec4{0U, 0U, 0U, 0U}), FloatEq(glm::vec4{0.0F, 0.0F, 0.0F, 0.0F}));
    EXPECT_THAT(toNormalizedVec(glm::u8vec4{255U, 255U, 255U, 255U}), FloatEq(glm::vec4{1.0F, 1.0F, 1.0F, 1.0F}));
    EXPECT_THAT(toNormalizedVec(glm::u8vec4{64U, 128U, 191U, 255U}),
                FloatEq(glm::vec4{0.2509804F, 0.5019608F, 0.7490196F, 1.0F}));
}
