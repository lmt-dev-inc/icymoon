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

TEST_F(VulkanFramePipelineIntegrationTest, simpleTriangle_rgb8)
{
    initialize(
        PipelineIntegrationTest::Config{
            .vkOutputExtent = VkExtent2D{.width = 800U, .height = 600U},
            .vkOutputFormat = VK_FORMAT_R8G8B8A8_UNORM,
            .frameInFlightCount = 2U,
        },
        createPipeline());

    runTest();

    constexpr auto TriangleColor = std::to_array<uint8_t>({255U, 128U, 0U, 255U});
    constexpr auto BackgroundColor = std::to_array<uint8_t>({0U, 0U, 0U, 0U});

    auto imageMapping = mapOutputImage();
    expectRgbaPixel(*imageMapping, {199U, 450U}, BackgroundColor);
    expectRgbaPixel(*imageMapping, {200U, 449U}, TriangleColor);
    expectRgbaPixel(*imageMapping, {289U, 450U}, BackgroundColor);
    expectRgbaPixel(*imageMapping, {399U, 151U}, TriangleColor);
    expectRgbaPixel(*imageMapping, {401U, 151U}, BackgroundColor);
    expectRgbaPixel(*imageMapping, {438U, 309U}, TriangleColor);
    expectRgbaPixel(*imageMapping, {596U, 446U}, TriangleColor);
    expectRgbaPixel(*imageMapping, {599U, 449U}, TriangleColor);
    expectRgbaPixel(*imageMapping, {600U, 449U}, BackgroundColor);
}

TEST_F(VulkanFramePipelineIntegrationTest, simpleTriangle_srgb8)
{
    initialize(
        PipelineIntegrationTest::Config{
            .vkOutputExtent = VkExtent2D{.width = 800U, .height = 600U},
            .vkOutputFormat = VK_FORMAT_R8G8B8A8_SRGB,
            .frameInFlightCount = 2U,
        },
        createPipeline());

    runTest();

    constexpr auto TriangleColor = std::to_array<uint8_t>({255U, 188U, 0U, 255U});
    constexpr auto BackgroundColor = std::to_array<uint8_t>({0U, 0U, 0U, 0U});

    auto imageMapping = mapOutputImage();
    expectRgbaPixel(*imageMapping, {199U, 450U}, BackgroundColor);
    expectRgbaPixel(*imageMapping, {200U, 449U}, TriangleColor);
    expectRgbaPixel(*imageMapping, {289U, 450U}, BackgroundColor);
    expectRgbaPixel(*imageMapping, {399U, 151U}, TriangleColor);
    expectRgbaPixel(*imageMapping, {401U, 151U}, BackgroundColor);
    expectRgbaPixel(*imageMapping, {438U, 309U}, TriangleColor);
    expectRgbaPixel(*imageMapping, {596U, 446U}, TriangleColor);
    expectRgbaPixel(*imageMapping, {599U, 449U}, TriangleColor);
    expectRgbaPixel(*imageMapping, {600U, 449U}, BackgroundColor);
}
