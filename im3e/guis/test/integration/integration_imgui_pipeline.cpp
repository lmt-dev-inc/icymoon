#include "src/imgui_pipeline.h"

#include "src/imgui_workspace.h"

#include <im3e/test_utils/pipeline_integration_test.h>
#include <im3e/utils/imgui_utils.h>

#include <fmt/format.h>

#include <array>

using namespace im3e;

namespace {

inline void blitImage(const VulkanDeviceFcts& rFcts, VkCommandBuffer vkCommandBuffer, const VkExtent2D& rVkExtent,
                      VkImage vkSrcImage, VkImage vkDstImage)
{
    const VkImageSubresourceLayers vkSubresourceLayers{.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT, .layerCount = 1U};
    const VkOffset3D vkStartOffset{};
    const VkOffset3D vkEndOffset{
        .x = vkStartOffset.x + static_cast<int32_t>(rVkExtent.width),
        .y = vkStartOffset.y + static_cast<int32_t>(rVkExtent.height),
        .z = 1,
    };

    VkImageBlit vkImageBlit{
        .srcSubresource = vkSubresourceLayers,
        .srcOffsets{vkStartOffset, vkEndOffset},
        .dstSubresource = vkSubresourceLayers,
        .dstOffsets{vkStartOffset, vkEndOffset},
    };

    rFcts.vkCmdBlitImage(vkCommandBuffer, vkSrcImage, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, vkDstImage,
                         VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1U, &vkImageBlit, VK_FILTER_NEAREST);
}

class TestImguiPanel : public IGuiPanel
{
public:
    TestImguiPanel(std::string_view content)
      : m_content(content)
    {
    }

    void draw(const ICommandBuffer&) override { ImGui::Text("%s", m_content.c_str()); }

    auto getName() const -> std::string override { return m_content; }

private:
    std::string m_content;
};

class TestImguiPanelWithDemo : public IGuiPanel
{
public:
    void draw(const ICommandBuffer&) override { ImGui::ShowDemoWindow(); }

    auto getName() const -> std::string override { return "ImGui Demo"; }
};

struct ImguiPipelineIntegration : public PipelineIntegrationTest
{
    auto createImguiPipeline()
    {
        return std::make_unique<ImguiPipeline>(getDevice(), nullptr, m_pWorkspace,
                                               fmt::format("{}.{}.ini", getSuiteName(), getName()));
    }

    std::shared_ptr<ImguiWorkspace> m_pWorkspace = std::make_shared<ImguiWorkspace>("Test Workspace");
};

}  // namespace

TEST_F(ImguiPipelineIntegration, emptyWorkspace)
{
    initialize(
        PipelineIntegrationTest::Config{
            .vkOutputExtent = VkExtent2D{800U, 600U},
            .vkOutputFormat = VK_FORMAT_R8G8B8A8_SRGB,
            .frameInFlightCount = 2U,
        },
        createImguiPipeline());
    runTest(3U);

    auto pImageMapping = mapOutputImage();
    expectRgbaPixel(*pImageMapping, {137U, 158U}, {0U, 0U, 0U, 0U});
    expectRgbaPixel(*pImageMapping, {376U, 253U}, {0U, 0U, 0U, 0U});
    expectRgbaPixel(*pImageMapping, {766U, 22U}, {62U, 62U, 71U, 255U});
    expectRgbaPixel(*pImageMapping, {745U, 24U}, {88U, 88U, 103U, 160U});
    expectRgbaPixel(*pImageMapping, {589, 5U}, {28U, 28U, 28U, 255U});
}

TEST_F(ImguiPipelineIntegration, withTestPanels)
{
    m_pWorkspace->addPanel(IGuiWorkspace::Location::Left, std::make_shared<TestImguiPanel>("Left Panel"));
    m_pWorkspace->addPanel(IGuiWorkspace::Location::Top, std::make_shared<TestImguiPanel>("Top Panel"));
    m_pWorkspace->addPanel(IGuiWorkspace::Location::Bottom, std::make_shared<TestImguiPanel>("Bottom Panel"));
    m_pWorkspace->addPanel(IGuiWorkspace::Location::Right, std::make_shared<TestImguiPanel>("Right Panel"));
    m_pWorkspace->addPanel(IGuiWorkspace::Location::Center, std::make_shared<TestImguiPanel>("Center Panel"));

    initialize(
        PipelineIntegrationTest::Config{
            .vkOutputExtent = VkExtent2D{800U, 600U},
            .vkOutputFormat = VK_FORMAT_R8G8B8A8_SRGB,
            .frameInFlightCount = 2U,
        },
        createImguiPipeline());
    runTest(3U);

    auto pImageMapping = mapOutputImage();
    expectRgbaPixel(*pImageMapping, {768U, 160U}, {0U, 13U, 28U, 254U});
    expectRgbaPixel(*pImageMapping, {60U, 10U}, {28U, 28U, 28U, 255U});
    expectRgbaPixel(*pImageMapping, {379U, 23U}, {82U, 82U, 95U, 255U});
    expectRgbaPixel(*pImageMapping, {55U, 238U}, {0U, 13U, 28U, 254U});
    expectRgbaPixel(*pImageMapping, {200U, 288U}, {79U, 80U, 95U, 255U});
    expectRgbaPixel(*pImageMapping, {290U, 192U}, {28U, 64U, 109U, 255U});
    expectRgbaPixel(*pImageMapping, {550U, 167U}, {0U, 13U, 28U, 254U});
    expectRgbaPixel(*pImageMapping, {531U, 511U}, {28U, 64U, 109U, 255U});
    expectRgbaPixel(*pImageMapping, {272U, 595U}, {0U, 13U, 28U, 254U});
}

TEST_F(ImguiPipelineIntegration, withImguiDemo)
{
    m_pWorkspace->addPanel(IGuiWorkspace::Location::Center, std::make_shared<TestImguiPanelWithDemo>());

    initialize(
        PipelineIntegrationTest::Config{
            .vkOutputExtent = VkExtent2D{1280U, 800U},
            .vkOutputFormat = VK_FORMAT_R8G8B8A8_SRGB,
            .frameInFlightCount = 2U,
        },
        createImguiPipeline());
    runTest(3U);

    auto pImageMapping = mapOutputImage();
    expectRgbaPixel(*pImageMapping, {664U, 28U}, {255U, 255U, 255U, 255U});
    expectRgbaPixel(*pImageMapping, {804U, 29U}, {38U, 73U, 124U, 255U});
    expectRgbaPixel(*pImageMapping, {650U, 341U}, {79U, 80U, 95U, 255U});
    expectRgbaPixel(*pImageMapping, {1193U, 695U}, {26U, 73U, 124U, 255U});
    expectRgbaPixel(*pImageMapping, {1182U, 226U}, {34U, 89U, 150U, 255U});
}