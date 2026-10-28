#include <im3e/scene/view_transform.h>

#include <im3e/test_utils/glm.h>
#include <im3e/test_utils/test_utils.h>

using namespace im3e;

namespace {

struct ViewTransformTest : public Test
{
    MockFunction<void()> mockOnChangedFct;

    auto createTransform() { return ViewTransform(mockOnChangedFct.AsStdFunction()); }
    auto createExpectedMatrix(const ViewTransform& transform)
    {
        return glm::lookAt(transform.getPosition(), transform.getPosition() + transform.getDirection(),
                           transform.getUp());
    }
};

}  // namespace

TEST_F(ViewTransformTest, constructor)
{
    const auto transform = createTransform();
    EXPECT_THAT(transform.getPosition(), FloatEq(glm::vec3{0.0F, 0.0F, 0.0F}));
    EXPECT_THAT(transform.getDirection(), FloatEq(glm::vec3{0.0F, 0.0F, -1.0F}));
    EXPECT_THAT(transform.getUp(), FloatEq(glm::vec3{0.0F, 1.0F, 0.0F}));
    EXPECT_THAT(transform.getRight(), FloatEq(glm::vec3{1.0F, 0.0F, 0.0F}));
    EXPECT_THAT(transform.getMatrix(), FloatEq(createExpectedMatrix(transform)));
}

TEST_F(ViewTransformTest, constructor_withEmptyOnChangedFct)
{
    ViewTransform transform({});

    // Empty std::function throw when called, this is to make sure we validate the function before trying to call it:
    EXPECT_NO_THROW(transform.setPosition(glm::vec3{1.0F, 2.0F, 3.0F}));
    EXPECT_NO_THROW(transform.setDirection(glm::vec3{0.1F, 0.2F, 0.3F}, glm::vec3{0.4F, 0.5F, 0.6F}));
}

TEST_F(ViewTransformTest, setPosition)
{
    constexpr auto TestPosition = glm::vec3{100.0F, 200.0F, 300.0F};
    auto transform = createTransform();

    EXPECT_CALL(mockOnChangedFct, Call());
    transform.setPosition(TestPosition);
    EXPECT_THAT(transform.getPosition(), FloatEq(TestPosition));
    EXPECT_THAT(transform.getMatrix(), FloatEq(createExpectedMatrix(transform)));
}

TEST_F(ViewTransformTest, setDirection)
{
    constexpr auto TestDir = glm::vec3{1.0F, 0.0F, 0.0F};
    constexpr auto TestUp = glm::vec3{0.0F, 0.0F, -1.0F};
    constexpr auto TestRight = glm::vec3{0.0F, 1.0F, 0.0F};
    auto transform = createTransform();
    const auto position = transform.getPosition();

    // We expect three property changes from the transform: direction, up, and right vectors.
    EXPECT_CALL(mockOnChangedFct, Call()).Times(3);

    transform.setDirection(TestDir, TestUp);
    EXPECT_THAT(transform.getPosition(), FloatEq(position));
    EXPECT_THAT(transform.getDirection(), FloatEq(TestDir));
    EXPECT_THAT(transform.getUp(), FloatEq(TestUp));
    EXPECT_THAT(transform.getRight(), FloatEq(TestRight));
    EXPECT_THAT(transform.getMatrix(), FloatEq(createExpectedMatrix(transform)));
}

TEST_F(ViewTransformTest, getMatrix_calledTwiceWithoutChange)
{
    const auto transform = createTransform();
    const auto expectedMatrix = createExpectedMatrix(transform);

    // Second call takes the cached path, it must return the same matrix as the first one:
    EXPECT_THAT(transform.getMatrix(), FloatEq(expectedMatrix));
    EXPECT_THAT(transform.getMatrix(), FloatEq(expectedMatrix));
}