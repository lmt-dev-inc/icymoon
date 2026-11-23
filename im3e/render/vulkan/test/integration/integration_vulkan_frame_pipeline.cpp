#include <im3e/render/vulkan/vulkan_render_engine.h>

#include <im3e/test_utils/pipeline_integration_test.h>

using namespace im3e;

namespace {

struct VulkanFramePipelineIntegrationTest : public PipelineIntegrationTest
{
    VulkanFramePipelineIntegrationTest()
      : m_engine(createVulkanRenderEngine(getDevice()))
    {
    }

    auto createPipeline() { return m_engine->createFramePipeline(); }

    std::unique_ptr<IVulkanRenderEngine> m_engine;
};

}  // namespace

TEST_F(VulkanFramePipelineIntegrationTest, simpleTriangle)
{
    initialize(
        PipelineIntegrationTest::Config{
            .vkOutputExtent = VkExtent2D{.width = 800U, .height = 600U},
            .vkOutputFormat = VK_FORMAT_R8G8B8A8_SRGB,
            .frameInFlightCount = 2U,
        },
        createPipeline());

    runTest();
}
