#include "src/imgui_vulkan_backend.h"

#include "src/imgui_context.h"

#include <im3e/test_utils/pipeline_integration_test.h>
#include <im3e/utils/imgui_utils.h>

#include <fmt/format.h>
#include <imgui_impl_vulkan.h>

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

struct ImguiVulkanBackendIntegration : public PipelineIntegrationTest
{
    ImguiContext m_imguiContext;
};

}  // namespace

TEST_F(ImguiVulkanBackendIntegration, renderImgui)
{
    constexpr VkExtent2D TestOutputExtent{800U, 600U};
    constexpr auto TestOutputFormat = VK_FORMAT_R8G8B8A8_UNORM;

    std::shared_ptr<IImage> pGuiImage = getDevice()->getImageFactory()->createImage(ImageConfig{
        .name = "ImguiBackendImage",
        .vkExtent = TestOutputExtent,
        .vkFormat = TestOutputFormat,
        .vkUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
    });

    // A ImGui context must be active while using the backend:
    auto pContextGuard = m_imguiContext.makeCurrent();
    auto pBackend = std::make_shared<ImguiVulkanBackend>(getDevice(), pGuiImage, 1U, nullptr);

    initialize(
        PipelineIntegrationTest::Config{
            .vkOutputExtent = TestOutputExtent,
            .vkOutputFormat = VK_FORMAT_R8G8B8A8_UNORM,
            .frameInFlightCount = 2U,
        },
        [&](ICommandBuffer& rCommandBuffer, std::shared_ptr<IImage> pOutputImage) {
            ImGui_ImplVulkan_NewFrame();

            // When doing offscreen rendering, a display size must be manually specified:
            auto& rIo = ImGui::GetIO();
            rIo.DisplaySize.x = pOutputImage->getVkExtent().width;
            rIo.DisplaySize.y = pOutputImage->getVkExtent().height;

            ImGui::NewFrame();
            {
                ImGui::SetNextWindowPos(ImVec2(0, 0));
                ImGui::SetNextWindowSize(rIo.DisplaySize);
                ImguiScope window(ImGui::Begin("Test Window"), &ImGui::End, true);
            }
            ImGui::Render();

            pBackend->prepareExecution(rCommandBuffer);
            {
                auto pBarrier = rCommandBuffer.startScopedBarrier("finalizeImage");
                pBarrier->addImageBarrier(*pGuiImage, ImageBarrierConfig{
                                                          .vkDstStageMask = VK_PIPELINE_STAGE_2_BLIT_BIT,
                                                          .vkDstAccessMask = VK_ACCESS_2_TRANSFER_READ_BIT,
                                                          .vkLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                                                      });
                pBarrier->addImageBarrier(*pOutputImage, ImageBarrierConfig{
                                                             .vkDstStageMask = VK_PIPELINE_STAGE_2_BLIT_BIT,
                                                             .vkDstAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT,
                                                             .vkLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                                                         });
            }
            blitImage(getDevice()->getFcts(), rCommandBuffer.getVkCommandBuffer(), TestOutputExtent,
                      pGuiImage->getVkImage(), pOutputImage->getVkImage());
        });
    runTest(2U);

    auto pImageMapping = mapOutputImage();
    auto expectRgbaPixel = [&](uint32_t x, uint32_t y, std::array<uint8_t, 4U> expected) {
        const auto& rRgbaPixel = *reinterpret_cast<const std::array<uint8_t, 4U>*>(pImageMapping->getPixel(x, y));
        EXPECT_THAT(rRgbaPixel, ContainerEq(expected)) << fmt::format("Pixel at [{}; {}]", x, y);
    };
    expectRgbaPixel(0U, 0U, {76U, 92U, 125U, 255U});
    expectRgbaPixel(1U, 1U, {41U, 74U, 122U, 255U});
    expectRgbaPixel(11U, 8U, {255U, 255U, 255U, 255U});
    expectRgbaPixel(85U, 10U, {41U, 74U, 122U, 255U});
    expectRgbaPixel(82U, 143U, {14U, 14U, 14U, 240U});
    expectRgbaPixel(798U, 535U, {14U, 14U, 14U, 240U});
    expectRgbaPixel(799U, 524U, {62U, 62U, 71U, 248U});
    expectRgbaPixel(798U, 598U, {24U, 41U, 61U, 243U});
    expectRgbaPixel(799U, 599U, {62U, 62U, 71U, 248U});
}