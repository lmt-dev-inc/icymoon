#include <im3e/render/vulkan/vulkan_render_engine.h>

#include <im3e/test_utils/pipeline_integration_test.h>

#include <array>
#include <cstdint>
#include <memory>

using namespace im3e;

namespace {

struct VulkanFramePipelineIntegrationTest : public PipelineIntegrationTest
{
    VulkanFramePipelineIntegrationTest()
      : m_engine(createVulkanRenderEngine(getDevice()))
    {
    }

    auto createPipeline() -> std::unique_ptr<IFramePipeline> { return m_engine->createFramePipeline(); }

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
    constexpr auto Tolerance = 1U;  // we add a tolerance because of rounding of the triangle color (1.0, 0.5, 0.0)

    auto imageMapping = mapOutputImage();
    expectRgbaPixel(*imageMapping, {199U, 450U}, BackgroundColor, Tolerance);
    expectRgbaPixel(*imageMapping, {200U, 449U}, TriangleColor, Tolerance);
    expectRgbaPixel(*imageMapping, {289U, 450U}, BackgroundColor, Tolerance);
    expectRgbaPixel(*imageMapping, {399U, 151U}, TriangleColor, Tolerance);
    expectRgbaPixel(*imageMapping, {401U, 151U}, BackgroundColor, Tolerance);
    expectRgbaPixel(*imageMapping, {438U, 309U}, TriangleColor, Tolerance);
    expectRgbaPixel(*imageMapping, {596U, 446U}, TriangleColor, Tolerance);
    expectRgbaPixel(*imageMapping, {599U, 449U}, TriangleColor, Tolerance);
    expectRgbaPixel(*imageMapping, {600U, 449U}, BackgroundColor, Tolerance);
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
    constexpr auto Tolerance = 1U;  // we add a tolerance because of rounding of the triangle color (1.0, 0.5, 0.0)

    auto imageMapping = mapOutputImage();
    expectRgbaPixel(*imageMapping, {199U, 450U}, BackgroundColor, Tolerance);
    expectRgbaPixel(*imageMapping, {200U, 449U}, TriangleColor, Tolerance);
    expectRgbaPixel(*imageMapping, {289U, 450U}, BackgroundColor, Tolerance);
    expectRgbaPixel(*imageMapping, {399U, 151U}, TriangleColor, Tolerance);
    expectRgbaPixel(*imageMapping, {401U, 151U}, BackgroundColor, Tolerance);
    expectRgbaPixel(*imageMapping, {438U, 309U}, TriangleColor, Tolerance);
    expectRgbaPixel(*imageMapping, {596U, 446U}, TriangleColor, Tolerance);
    expectRgbaPixel(*imageMapping, {599U, 449U}, TriangleColor, Tolerance);
    expectRgbaPixel(*imageMapping, {600U, 449U}, BackgroundColor, Tolerance);
}
