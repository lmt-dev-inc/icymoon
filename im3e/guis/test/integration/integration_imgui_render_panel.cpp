#include "src/imgui_render_panel.h"

#include "src/imgui_pipeline.h"
#include "src/imgui_workspace.h"

#include "clear_color_test_pipeline.h"

#include <im3e/test_utils/pipeline_integration_test.h>

#include <array>

using namespace im3e;

struct ImguiRenderPanelIntegration : public PipelineIntegrationTest
{
    auto createImguiPipeline()
    {
        return std::make_unique<ImguiPipeline>(getDevice(), nullptr, m_pWorkspace,
                                               fmt::format("{}.{}.ini", getSuiteName(), getName()));
    }

    auto addRenderPanel(const VkClearColorValue& rVkClearColor)
    {
        auto pRenderPanel = std::make_shared<ImguiRenderPanel>(
            "Render", std::make_unique<ClearColorTestPipeline>(getDevice(), rVkClearColor));
        m_pWorkspace->addPanel(IGuiWorkspace::Location::Center, pRenderPanel, 0.5F);
    }

    auto runPanelTest(VkFormat vkOutputFormat)
    {
        initialize(
            PipelineIntegrationTest::Config{
                .vkOutputExtent = VkExtent2D{800U, 600U},
                .vkOutputFormat = vkOutputFormat,
                .frameInFlightCount = 1U,
            },
            createImguiPipeline());

        runTest(2U);
    }

    std::shared_ptr<ImguiWorkspace> m_pWorkspace = std::make_shared<ImguiWorkspace>("Test Workspace");
};

TEST_F(ImguiRenderPanelIntegration, toSRgb)
{
    constexpr VkClearColorValue ClearColor{0.0177445058F, 0.0656561702F, 0.198943928F, 1.0F};
    constexpr std::array<uint8_t, 4U> ExpectedColor{36U, 72U, 123U, 255U};

    addRenderPanel(ClearColor);
    runPanelTest(VK_FORMAT_R8G8B8A8_SRGB);

    auto pImageMapping = mapOutputImage();
    expectRgbaPixelRegion(*pImageMapping, {8U, 56U}, {792U, 592U}, ExpectedColor);
}

TEST_F(ImguiRenderPanelIntegration, toRgb)
{
    constexpr VkClearColorValue ClearColor{0.5F, 0.5F, 0.5F, 1.0F};
    constexpr std::array<uint8_t, 4U> ExpectedColor{127U, 127U, 127U, 255U};

    addRenderPanel(ClearColor);
    runPanelTest(VK_FORMAT_R8G8B8A8_UNORM);

    auto pImageMapping = mapOutputImage();
    expectRgbaPixelRegion(*pImageMapping, {8U, 56U}, {792U, 592U}, ExpectedColor);
}