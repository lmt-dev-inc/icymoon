#include <im3e/scene/camera.h>

#include <im3e/test_utils/glm.h>
#include <im3e/test_utils/test_utils.h>

#include <numbers>

using namespace im3e;

namespace {

struct CameraTest : public Test
{
    MockFunction<void()> mockOnChanged;

    auto createTestCamera() { return createCamera(mockOnChanged.AsStdFunction()); }

    void expectFrustumUpToDate(const ICamera& rCamera)
    {
        const auto& rProjection = rCamera.getProjection();
        const auto& rView = rCamera.getView();
        const auto& rFrustumConfig = rCamera.getViewFrustum().getConfig();
        EXPECT_THAT(rFrustumConfig.fovY, FloatEq(rProjection.getFovY()));
        EXPECT_THAT(rFrustumConfig.aspectRatio, FloatEq(rProjection.getAspectRatio()));
        EXPECT_THAT(rFrustumConfig.near, FloatEq(rProjection.getNear()));
        EXPECT_THAT(rFrustumConfig.far, FloatEq(rProjection.getFar()));
        EXPECT_THAT(rFrustumConfig.position, FloatEq(rView.getPosition()));
        EXPECT_THAT(rFrustumConfig.direction, FloatEq(rView.getDirection()));
        EXPECT_THAT(rFrustumConfig.up, FloatEq(rView.getUp()));
        EXPECT_THAT(rFrustumConfig.right, FloatEq(rView.getRight()));
    }
};

}  // namespace

TEST_F(CameraTest, createViewRayFromNdc_defaultCamera)
{
    EXPECT_CALL(mockOnChanged, Call()).Times(0);
    const auto pCamera = createTestCamera();

    auto ray = pCamera->createViewRayFromNdc(glm::vec2{0.0F, 0.0F});
    EXPECT_THAT(ray.origin, FloatEq(pCamera->getView().getPosition()));
    EXPECT_THAT(ray.direction, FloatEq(pCamera->getView().getDirection()));

    ray = pCamera->createViewRayFromNdc(glm::vec2{1.0F, 1.0F});
    EXPECT_THAT(ray.origin, FloatEq(pCamera->getView().getPosition()));
    EXPECT_THAT(ray.direction, FloatEq(glm::vec3{0.4472136F, 0.4472136F, -0.7745967F}));

    ray = pCamera->createViewRayFromNdc(glm::vec2{-1.0F, -1.0F});
    EXPECT_THAT(ray.origin, FloatEq(pCamera->getView().getPosition()));
    EXPECT_THAT(ray.direction, FloatEq(glm::vec3{-0.4472136F, -0.4472136F, -0.7745967F}));
}

TEST_F(CameraTest, createViewRayFromNdc_differentPosition)
{
    constexpr auto TestPos = glm::vec3{965.0F, -354.5F, 123.4F};

    auto pCamera = createTestCamera();

    EXPECT_CALL(mockOnChanged, Call()).Times(1);
    pCamera->getView().setPosition(TestPos);
    EXPECT_THAT(pCamera->getView().getPosition(), FloatEq(TestPos));

    auto ray = pCamera->createViewRayFromNdc(glm::vec2{0.0F, 0.0F});
    EXPECT_THAT(ray.origin, FloatEq(pCamera->getView().getPosition()));
    EXPECT_THAT(ray.direction, FloatEq(pCamera->getView().getDirection()));

    ray = pCamera->createViewRayFromNdc(glm::vec2{1.0F, 1.0F});
    EXPECT_THAT(ray.origin, FloatEq(pCamera->getView().getPosition()));
    EXPECT_THAT(ray.direction, FloatEq(glm::vec3{0.4472136F, 0.4472136F, -0.7745967F}));

    ray = pCamera->createViewRayFromNdc(glm::vec2{-1.0F, -1.0F});
    EXPECT_THAT(ray.origin, FloatEq(pCamera->getView().getPosition()));
    EXPECT_THAT(ray.direction, FloatEq(glm::vec3{-0.4472136F, -0.4472136F, -0.7745967F}));
}

TEST_F(CameraTest, createViewRayFromNdc_differentDirection)
{
    constexpr auto TestDir = glm::vec3{0.0F, 1.0F, 0.0F};
    constexpr auto TestUp = glm::vec3{0.0F, 0.0F, 1.0F};
    constexpr auto TestRight = glm::vec3{1.0F, 0.0F, 0.0F};

    auto pCamera = createTestCamera();
    auto& rView = pCamera->getView();

    EXPECT_CALL(mockOnChanged, Call()).Times(2);
    rView.setDirection(TestDir, TestUp);
    EXPECT_THAT(rView.getDirection(), FloatEq(TestDir));
    EXPECT_THAT(rView.getUp(), FloatEq(TestUp));
    EXPECT_THAT(rView.getRight(), FloatEq(TestRight));

    auto ray = pCamera->createViewRayFromNdc(glm::vec2{0.0F, 0.0F});
    EXPECT_THAT(ray.origin, FloatEq(rView.getPosition()));
    EXPECT_THAT(ray.direction, FloatEq(rView.getDirection()));

    ray = pCamera->createViewRayFromNdc(glm::vec2{1.0F, 1.0F});
    EXPECT_THAT(ray.origin, FloatEq(rView.getPosition()));
    EXPECT_THAT(ray.direction, FloatEq(glm::vec3{0.4472136F, 0.7745967F, 0.4472136F}));

    ray = pCamera->createViewRayFromNdc(glm::vec2{-1.0F, -1.0F});
    EXPECT_THAT(ray.origin, FloatEq(rView.getPosition()));
    EXPECT_THAT(ray.direction, FloatEq(glm::vec3{-0.4472136F, 0.7745967F, -0.4472136F}));
}

TEST_F(CameraTest, getProjection)
{
    EXPECT_CALL(mockOnChanged, Call()).Times(0);
    const auto pCamera = createTestCamera();
    const auto& rProjection = pCamera->getProjection();
    EXPECT_THAT(rProjection.getFovY(), FloatEq(std::numbers::pi_v<float> / 3.0F));
    EXPECT_THAT(rProjection.getAspectRatio(), FloatEq(1.0F));
    EXPECT_THAT(rProjection.getNear(), FloatEq(0.1F));
    EXPECT_THAT(rProjection.getFar(), FloatEq(10000.0F));
}

TEST_F(CameraTest, getView)
{
    EXPECT_CALL(mockOnChanged, Call()).Times(0);
    const auto pCamera = createTestCamera();
    const auto& rView = pCamera->getView();
    EXPECT_THAT(rView.getPosition(), FloatEq(glm::vec3{0.0F, 0.0F, 0.0F}));
    EXPECT_THAT(rView.getDirection(), FloatEq(glm::vec3{0.0F, 0.0F, -1.0F}));
    EXPECT_THAT(rView.getUp(), FloatEq(glm::vec3{0.0F, 1.0F, 0.0F}));
    EXPECT_THAT(rView.getRight(), FloatEq(glm::vec3{1.0F, 0.0F, 0.0F}));
}

TEST_F(CameraTest, getViewFrustum)
{
    EXPECT_CALL(mockOnChanged, Call()).Times(0);
    const auto pCamera = createTestCamera();
    expectFrustumUpToDate(*pCamera);
}

TEST_F(CameraTest, changeProjection)
{
    auto pCamera = createTestCamera();
    auto& rProjection = pCamera->getProjection();

    EXPECT_CALL(mockOnChanged, Call()).Times(4);
    rProjection.setFovY(std::numbers::pi_v<float> / 2.0F);
    rProjection.setAspectRatio(2.0F);
    rProjection.setNear(1.0F);
    rProjection.setFar(100.0F);

    expectFrustumUpToDate(*pCamera);
}

TEST_F(CameraTest, changeView)
{
    auto pCamera = createTestCamera();
    auto& rView = pCamera->getView();

    EXPECT_CALL(mockOnChanged, Call()).Times(4);
    rView.setPosition(glm::vec3{1.0F, -2.0F, 3.0F});
    rView.setDirection(glm::vec3{-1.0F, 0.0F, 0.0F}, glm::vec3{0.0F, 0.0F, -1.0F});

    expectFrustumUpToDate(*pCamera);
}

TEST_F(CameraTest, changeWithEmptyOnChanged)
{
    auto pCamera = createCamera({});
    auto& rProjection = pCamera->getProjection();
    auto& rView = pCamera->getView();

    EXPECT_CALL(mockOnChanged, Call()).Times(0);
    rProjection.setFovY(std::numbers::pi_v<float> / 2.0F);
    rProjection.setAspectRatio(2.0F);
    rProjection.setNear(1.0F);
    rProjection.setFar(100.0F);
    rView.setPosition(glm::vec3{1.0F, -2.0F, 3.0F});
    rView.setDirection(glm::vec3{-1.0F, 0.0F, 0.0F}, glm::vec3{0.0F, 0.0F, -1.0F});

    expectFrustumUpToDate(*pCamera);
}
