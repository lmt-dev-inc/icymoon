#include <im3e/scene/perspective_projection.h>

#include <im3e/test_utils/glm.h>
#include <im3e/test_utils/test_utils.h>

#include <glm/gtc/matrix_transform.hpp>

#include <numbers>

using namespace im3e;

namespace {

constexpr auto Pi = std::numbers::pi_v<float>;

struct PerspectiveProjectionTest : public Test
{
    MockFunction<void()> mockOnChangedFct;

    auto createProjection() { return PerspectiveProjection(mockOnChangedFct.AsStdFunction()); }
};

}  // namespace

TEST_F(PerspectiveProjectionTest, constructor)
{
    constexpr auto FovY = Pi / 3.0F;
    constexpr auto AspectRatio = 1.0F;
    constexpr auto Near = 0.1F;
    constexpr auto Far = 10000.0F;

    const auto proj = createProjection();
    EXPECT_THAT(proj.getFovY(), FloatEq(FovY));
    EXPECT_THAT(proj.getAspectRatio(), FloatEq(AspectRatio));
    EXPECT_THAT(proj.getNear(), FloatEq(Near));
    EXPECT_THAT(proj.getFar(), FloatEq(Far));
    EXPECT_THAT(proj.getMatrix(), FloatEq(glm::perspective(FovY, AspectRatio, Near, Far)));
}

TEST_F(PerspectiveProjectionTest, construction_withEmptyOnChangedFct)
{
    PerspectiveProjection proj({});

    // Empty std::function throw when called, this is to make sure we validate the function before trying to call it:
    EXPECT_NO_THROW(proj.setFovY(Pi / 2.0F));
    EXPECT_NO_THROW(proj.setAspectRatio(1.3F));
    EXPECT_NO_THROW(proj.setNear(0.25F));
    EXPECT_NO_THROW(proj.setFar(1000.0F));
}

TEST_F(PerspectiveProjectionTest, setFovY)
{
    constexpr auto TestFovY = Pi / 2.0F;
    auto proj = createProjection();

    EXPECT_CALL(mockOnChangedFct, Call());
    proj.setFovY(TestFovY);
    EXPECT_THAT(proj.getFovY(), FloatEq(TestFovY));
    EXPECT_THAT(proj.getMatrix(),
                FloatEq(glm::perspective(TestFovY, proj.getAspectRatio(), proj.getNear(), proj.getFar())));
}

TEST_F(PerspectiveProjectionTest, setAspectRatio)
{
    constexpr auto TestAspectRatio = 1.5F;
    auto proj = createProjection();

    EXPECT_CALL(mockOnChangedFct, Call());
    proj.setAspectRatio(TestAspectRatio);
    EXPECT_THAT(proj.getAspectRatio(), FloatEq(TestAspectRatio));
    EXPECT_THAT(proj.getMatrix(),
                FloatEq(glm::perspective(proj.getFovY(), TestAspectRatio, proj.getNear(), proj.getFar())));
}

TEST_F(PerspectiveProjectionTest, setNear)
{
    constexpr auto TestNear = 0.5F;
    auto proj = createProjection();

    EXPECT_CALL(mockOnChangedFct, Call());
    proj.setNear(TestNear);
    EXPECT_THAT(proj.getNear(), FloatEq(TestNear));
    EXPECT_THAT(proj.getMatrix(),
                FloatEq(glm::perspective(proj.getFovY(), proj.getAspectRatio(), TestNear, proj.getFar())));
}

TEST_F(PerspectiveProjectionTest, setFar)
{
    constexpr auto TestFar = 500.0F;
    auto proj = createProjection();

    EXPECT_CALL(mockOnChangedFct, Call());
    proj.setFar(TestFar);
    EXPECT_THAT(proj.getFar(), FloatEq(TestFar));
    EXPECT_THAT(proj.getMatrix(),
                FloatEq(glm::perspective(proj.getFovY(), proj.getAspectRatio(), proj.getNear(), TestFar)));
}

TEST_F(PerspectiveProjectionTest, getMatrix_calledTwiceWithoutChange)
{
    const auto proj = createProjection();
    const auto expectedMatrix = glm::perspective(proj.getFovY(), proj.getAspectRatio(), proj.getNear(), proj.getFar());

    // Second call takes the cached path, it must return the same matrix as the first one:
    EXPECT_THAT(proj.getMatrix(), FloatEq(expectedMatrix));
    EXPECT_THAT(proj.getMatrix(), FloatEq(expectedMatrix));
}
