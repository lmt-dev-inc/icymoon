#include "transform.h"

#include "math_utils.h"

#include <im3e/test_utils/glm.h>
#include <im3e/test_utils/test_utils.h>

#include <fmt/format.h>
#include <gmock/gmock.h>
#include <glm/ext/matrix_transform.hpp>
#include <glm/gtx/quaternion.hpp>

#include <iostream>

using namespace im3e;

TEST(TransformTest, constructor)
{
    Transform transform;
    EXPECT_THAT(transform.getTranslation(), Eq(glm::vec3(0.0F, 0.0F, 0.0F)));
    EXPECT_THAT(transform.getScale(), Eq(glm::vec3(1.0F, 1.0F, 1.0F)));
    EXPECT_THAT(transform.getOrientation(), Eq(glm::quat(1.0F, 0.0F, 0.0F, 0.0F)));
    EXPECT_THAT(transform.toMatrix(), FloatEq(glm::identity<glm::mat4>()));
}

TEST(TransformTest, translate)
{
    constexpr auto Translation1 = glm::vec3{1.0F, 0.5F, 2.0F};
    constexpr auto Translation2 = glm::vec3{-0.4F, 0.25F, -1.0F};
    Transform transform;

    transform.translate(Translation1);
    EXPECT_THAT(transform.getTranslation(), FloatEq(Translation1));
    EXPECT_THAT(transform.toMatrix(), FloatEq(glm::translate(Translation1)));

    transform.translate(Translation2);
    EXPECT_THAT(transform.getTranslation(), FloatEq(Translation1 + Translation2));
    EXPECT_THAT(transform.toMatrix(), FloatEq(glm::translate(Translation1 + Translation2)));
}

TEST(TransformTest, scale)
{
    constexpr auto Scale1 = glm::vec3{2.0F, 3.0F, 4.0F};
    constexpr auto Scale2 = glm::vec3{0.5F, -1.0F, 2.0F};
    Transform transform;

    transform.scale(Scale1);
    EXPECT_THAT(transform.getScale(), FloatEq(Scale1));
    EXPECT_THAT(transform.toMatrix(), FloatEq(glm::scale(Scale1)));

    transform.scale(Scale2);
    EXPECT_THAT(transform.getScale(), FloatEq(Scale1 * Scale2));
    EXPECT_THAT(transform.toMatrix(), FloatEq(glm::scale(Scale1 * Scale2)));
}

TEST(TransformTest, rotate)
{
    const auto rotation1 = glm::rotation(glm::vec3(0.0F, 0.0F, -1.0F), glm::vec3(0.0F, 1.0F, 0.0F));
    const auto rotation2 = glm::rotation(glm::vec3(0.0F, 1.0F, 0.0F), glm::vec3(1.0F, 0.0F, 0.0F));

    Transform transform;

    transform.rotate(90.0_fdeg, glm::vec3(1.0F, 0.0F, 0.0F));
    EXPECT_THAT(transform.getOrientation(), FloatEq(rotation1));
    EXPECT_THAT(transform.toMatrix(), FloatEq(glm::mat4_cast(rotation1)));

    transform.rotate(-90.0_fdeg, glm::vec3(0.0F, 0.0F, 1.0F));
    EXPECT_THAT(transform.getOrientation(), FloatEq(rotation2 * rotation1));
    EXPECT_THAT(transform.toMatrix(), FloatEq(glm::mat4_cast(rotation2 * rotation1)));
}

TEST(TransformTest, reset)
{
    Transform transform;
    transform.setTranslation(glm::vec3(1.0F, 2.0F, 3.0F));
    transform.setScale(glm::vec3(-12.0F, 5.0F, 4.0F));
    transform.setOrientation(glm::quat(2.0F, 3.0F, 4.0F, 5.0F));

    transform.reset();
    EXPECT_THAT(transform.getTranslation(), Eq(glm::vec3(0.0F, 0.0F, 0.0F)));
    EXPECT_THAT(transform.getScale(), Eq(glm::vec3(1.0F, 1.0F, 1.0F)));
    EXPECT_THAT(transform.getOrientation(), Eq(glm::quat(1.0F, 0.0F, 0.0F, 0.0F)));
    EXPECT_THAT(transform.toMatrix(), FloatEq(glm::identity<glm::mat4>()));
}

TEST(TransformTest, setRotation)
{
    Transform transform;

    transform.setRotation(90.0_fdeg, glm::vec3(0.0F, 0.0F, 1.0F));
    EXPECT_THAT(transform.getOrientation(),
                Eq(glm::rotation(glm::vec3(1.0F, 0.0F, 0.0F), glm::vec3(0.0F, 1.0F, 0.0F))));
}
