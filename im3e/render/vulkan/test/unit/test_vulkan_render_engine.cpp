#include <im3e/render/vulkan/vulkan_render_engine.h>

#include <im3e/test_utils/test_utils.h>
#include <im3e/vulkan/mock_device.h>

#include <memory>
#include <stdexcept>

using namespace im3e;

namespace {

struct VulkanRenderEngineTest : public Test
{
    auto createEngine() { return createVulkanRenderEngine(m_device); }

    NiceMock<MockDevice> m_mockDevice;
    std::shared_ptr<IDevice> m_device{m_mockDevice.createMockProxy()};
};

}  // namespace

TEST_F(VulkanRenderEngineTest, constructor_withoutDevice)
{
    EXPECT_THROW(createVulkanRenderEngine(nullptr), std::invalid_argument);
}

TEST_F(VulkanRenderEngineTest, constructor)
{
    EXPECT_CALL(m_mockDevice, createLogger(_)).Times(AnyNumber());
    EXPECT_CALL(m_mockDevice, createLogger(StrEq("VulkanRenderEngine")));

    auto engine = createEngine();
    EXPECT_THAT(engine, NotNull());
}

TEST_F(VulkanRenderEngineTest, createFramePipeline)
{
    auto engine = createEngine();

    auto framePipeline = engine->createFramePipeline();
    ASSERT_THAT(framePipeline, NotNull());
    EXPECT_THAT(framePipeline->getDevice(), Eq(m_device));
}

TEST_F(VulkanRenderEngineTest, framePipelinesShareDevice)
{
    auto engine = createEngine();

    auto framePipeline1 = engine->createFramePipeline();
    auto framePipeline2 = engine->createFramePipeline();
    ASSERT_THAT(framePipeline1, NotNull());
    ASSERT_THAT(framePipeline2, NotNull());
    EXPECT_THAT(framePipeline1.get(), Ne(framePipeline2.get()));
    EXPECT_THAT(framePipeline1->getDevice(), Eq(framePipeline2->getDevice()));
}

TEST_F(VulkanRenderEngineTest, framePipelineOutlivesEngine)
{
    auto engine = createEngine();
    auto framePipeline = engine->createFramePipeline();
    ASSERT_THAT(framePipeline, NotNull());

    engine.reset();
    EXPECT_THAT(framePipeline->getDevice(), Eq(m_device));
}
