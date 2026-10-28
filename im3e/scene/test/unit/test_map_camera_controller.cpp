#include <im3e/scene/map_camera_controller.h>

#include <im3e/scene/mock_camera.h>
#include <im3e/test_utils/glm.h>
#include <im3e/test_utils/test_utils.h>

#include <memory>
#include <numbers>

using namespace im3e;

namespace {

constexpr auto Pi = std::numbers::pi_v<float>;

struct MapCameraControllerTest : public Test
{
    std::shared_ptr<MockCamera> m_pMockCamera = std::make_shared<NiceMock<MockCamera>>();

    auto createTestController() { return MapCameraController(m_pMockCamera); }
    auto createTestRay(const glm::vec3& rDirection)
    {
        return Ray{
            .origin = m_pMockCamera->getMockView().getPosition(),
            .direction = glm::normalize(rDirection),
        };
    }
};

}  // namespace

TEST_F(MapCameraControllerTest, constructor_withNullCamera)
{
    EXPECT_THROW(MapCameraController controller(nullptr), std::invalid_argument);
}

TEST_F(MapCameraControllerTest, constructor)
{
    EXPECT_CALL(*m_pMockCamera, getView());

    const auto controller = createTestController();
    EXPECT_THAT(controller.getTargetPoint(), FloatEq(glm::vec3{3700.0F, 0.0F, 4000.0F}));
    EXPECT_THAT(controller.getDistanceToTarget(), FloatEq(5000.0F));
    EXPECT_THAT(controller.getHeading(), FloatEq(0.0F));
    EXPECT_THAT(controller.getTilt(), FloatEq(0.0F));
    EXPECT_THAT(controller.getProperties(), NotNull());

    auto& rView = m_pMockCamera->getMockView();
    EXPECT_THAT(rView.getPosition(), FloatEq(glm::vec3{3700.0F, 5000.0F, 4000.0F}));
    EXPECT_THAT(rView.getDirection(), FloatEq(glm::vec3{0.0F, -1.0F, 0.0F}));
}

TEST_F(MapCameraControllerTest, pan)
{
    constexpr auto NdcFrom = glm::vec2{0.0F, 0.5F};
    constexpr auto NdcTo = glm::vec2{0.5F, 0.0F};
    auto controller = createTestController();
    const auto rayFrom = createTestRay(glm::vec3{0.0F, -1.0F, 0.0F});
    const auto rayTo = createTestRay(glm::vec3{1.0F, -1.0F, 0.0F});

    EXPECT_CALL(*m_pMockCamera, createViewRayFromNdc(FloatEq(NdcFrom))).WillOnce(Return(rayFrom));
    EXPECT_CALL(*m_pMockCamera, createViewRayFromNdc(FloatEq(NdcTo))).WillOnce(Return(rayTo));
    EXPECT_CALL(m_pMockCamera->getMockOnChanged(), Call());
    controller.pan(NdcFrom, NdcTo);

    EXPECT_THAT(controller.getTargetPoint(), FloatEq(glm::vec3{-1300.0F, 0.0F, 4000.0F}));
}

TEST_F(MapCameraControllerTest, pan_atGrazingAngle)
{
    constexpr auto NdcFrom = glm::vec2{-1.0F, -1.0F};
    constexpr auto NdcTo = glm::vec2{0.0F, 0.0F};
    auto controller = createTestController();
    const auto rayFrom = createTestRay(glm::vec3{0.0F, 0.0F, -1.0F});
    const auto rayTo = createTestRay(glm::vec3{0.0F, 1.0F, -1.0F});

    EXPECT_CALL(*m_pMockCamera, createViewRayFromNdc(FloatEq(NdcFrom))).WillOnce(Return(rayFrom));
    EXPECT_CALL(*m_pMockCamera, createViewRayFromNdc(FloatEq(NdcTo))).WillOnce(Return(rayTo));
    EXPECT_CALL(m_pMockCamera->getMockOnChanged(), Call()).Times(0);
    const auto prevTargetPoint = controller.getTargetPoint();
    controller.pan(NdcFrom, NdcTo);

    // At grazing angles, we expect pan to do nothing:
    EXPECT_THAT(controller.getTargetPoint(), FloatEq(prevTargetPoint));
}

TEST_F(MapCameraControllerTest, rotate)
{
    constexpr auto TestHeading = Pi / 2.0F;
    constexpr auto TestTilt = Pi / 3.0F;
    auto controller = createTestController();

    EXPECT_CALL(m_pMockCamera->getMockOnChanged(), Call()).Times(6);
    controller.rotate(TestHeading, TestTilt);

    EXPECT_THAT(controller.getHeading(), FloatEq(TestHeading));
    EXPECT_THAT(controller.getTilt(), FloatEq(TestTilt));
}

TEST_F(MapCameraControllerTest, rotate_withLargeHeading)
{
    auto controller = createTestController();
    controller.rotate(Pi * 3.0F, 0.0F);
    EXPECT_THAT(controller.getHeading(), FloatEq(Pi));

    // Current heading is now Pi, by rotating again with 2.5Pi, we expect a heading of 3.5Pi which should be wrapped to
    // -0.5Pi:
    controller.rotate(Pi * 2.5F, 0.0F);
    EXPECT_THAT(controller.getHeading(), FloatEq(-0.5 * Pi));
}

TEST_F(MapCameraControllerTest, rotate_withNegativeHeading)
{
    auto controller = createTestController();
    controller.rotate(-Pi, 0.0F);
    EXPECT_THAT(controller.getHeading(), FloatEq(-Pi));

    // Our heading is now -Pi so adding another -Pi/2 should wrap around and return Pi/2:
    controller.rotate(-0.5 * Pi, 0.0F);
    EXPECT_THAT(controller.getHeading(), FloatEq(0.5 * Pi));
}

TEST_F(MapCameraControllerTest, zoom)
{
    constexpr auto ZoomFactor = 0.5F;
    auto controller = createTestController();
    const auto prevDistance = controller.getDistanceToTarget();

    EXPECT_CALL(m_pMockCamera->getMockOnChanged(), Call());
    controller.zoom(ZoomFactor);
    EXPECT_THAT(controller.getDistanceToTarget(), FloatEq(prevDistance * ZoomFactor));
}

TEST_F(MapCameraControllerTest, zoom_with0Factor)
{
    auto controller = createTestController();
    EXPECT_THROW(controller.zoom(0.0F), std::invalid_argument);
}

TEST_F(MapCameraControllerTest, zoom_withNegativeFactor)
{
    auto controller = createTestController();
    EXPECT_THROW(controller.zoom(-0.1F), std::invalid_argument);
}