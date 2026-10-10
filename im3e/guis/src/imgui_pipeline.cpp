#include "imgui_pipeline.h"

#include <im3e/resources/resources.h>
#include <im3e/utils/core/throw_utils.h>

#include <fmt/format.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_vulkan.h>

using namespace im3e;

namespace {

auto getUiScale(const ILogger& rLogger, GLFWwindow* pGlfwWindow)
{
    float xScale{1.0F}, yScale{1.0F};
    if (pGlfwWindow)
    {
        glfwGetWindowContentScale(pGlfwWindow, &xScale, &yScale);
    }
    rLogger.info(fmt::format("UI scaling detected to be {}", yScale));
    return yScale;
}

void loadFont(ImGuiIO& rIo, float uiScale)
{
    const auto fontPathStr = getGuiFontPath().string();
    auto pFont = rIo.Fonts->AddFontFromFileTTF(fontPathStr.c_str(), uiScale * 16.0F);
    pFont->Scale = 1.0F / uiScale;

    rIo.FontGlobalScale = uiScale;
}

auto convertSrgbToLinear(ImVec4* pColor, size_t colourCount)
{
    for (size_t i = 0U; i < colourCount; i++)
    {
        auto& rColor = *(pColor + i);
        rColor.x = std::pow(rColor.x, 2.2F);
        rColor.y = std::pow(rColor.y, 2.2F);
        rColor.z = std::pow(rColor.z, 2.2F);
    }
}

void initializeImguiStyle(float uiScale)
{
    auto& rStyle = ImGui::GetStyle();
    rStyle.IndentSpacing = 10.0F;
    rStyle.FramePadding.x = 5.6F;
    rStyle.FramePadding.y = 4.4F;
    rStyle.CellPadding.x = 3.6F;
    rStyle.CellPadding.y = 1.6F;
    rStyle.FrameBorderSize = 2.0F;
    rStyle.WindowBorderSize = 2.0F;
    rStyle.ChildBorderSize = 2.0F;
    rStyle.PopupBorderSize = 2.0F;
    rStyle.ScaleAllSizes(uiScale);

    convertSrgbToLinear(&rStyle.Colors[0], ImGuiCol_COUNT);
    rStyle.Colors[ImGuiCol_WindowBg] = ImVec4(0.0F / 255.0F, 1.0F / 255.0F, 3.0F / 255.0F, 240.0F / 255.0F);
}

inline void blitToOutputImage(const VulkanDeviceFcts& rFcts, VkCommandBuffer vkCommandBuffer, const IImage& rSrcImage,
                              IImage& rDstImage)
{
    const auto vkExtent = rDstImage.getVkExtent();

    const VkImageSubresourceLayers vkSubresourceLayers{.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT, .layerCount = 1U};
    const VkOffset3D vkStartOffset{};
    const VkOffset3D vkEndOffset{
        .x = vkStartOffset.x + static_cast<int32_t>(vkExtent.width),
        .y = vkStartOffset.y + static_cast<int32_t>(vkExtent.height),
        .z = 1,
    };

    VkImageBlit vkImageBlit{
        .srcSubresource = vkSubresourceLayers,
        .srcOffsets{vkStartOffset, vkEndOffset},
        .dstSubresource = vkSubresourceLayers,
        .dstOffsets{vkStartOffset, vkEndOffset},
    };

    rFcts.vkCmdBlitImage(vkCommandBuffer, rSrcImage.getVkImage(), VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                         rDstImage.getVkImage(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1U, &vkImageBlit,
                         VK_FILTER_NEAREST);
}

}  // namespace

ImguiPipeline::ImguiPipeline(std::shared_ptr<const IDevice> pDevice, GLFWwindow* pGlfwWindow,
                             std::shared_ptr<ImguiWorkspace> pWorkspace, std::optional<std::string> iniFilename)
  : m_pDevice(throwIfArgNull(std::move(pDevice), "ImGui Pipeline requires a device"))
  , m_pLogger(m_pDevice->createLogger("ImGui Pipeline"))
  , m_pGlfwWindow(pGlfwWindow)
  , m_pWorkspace(throwIfArgNull(std::move(pWorkspace), "ImGui Pipeline requires a workspace"))
  , m_iniFilename(iniFilename)

  , m_pContext(std::make_unique<ImguiContext>())
{
    auto pContextGuard = m_pContext->makeCurrent();

    auto& rIo = ImGui::GetIO();
    rIo.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard | ImGuiConfigFlags_NavEnableGamepad;
    rIo.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

    rIo.IniFilename = iniFilename.has_value() ? m_iniFilename.value().c_str() : nullptr;

    const auto uiScale = getUiScale(*m_pLogger, m_pGlfwWindow);
    loadFont(rIo, uiScale);
    initializeImguiStyle(uiScale);

    m_pLogger->debug("Successfully initialized");
}

ImguiPipeline::~ImguiPipeline()
{
    auto pContextGuard = m_pContext->makeCurrent();
    m_pBackend.reset();
}

void ImguiPipeline::prepareExecution(const ICommandBuffer& rCommandBuffer, const VkExtent2D& rVkViewportSize,
                                     std::shared_ptr<IImage> pOutputImage)
{
    auto pContextGuard = m_pContext->makeCurrent();

    ImGui_ImplVulkan_NewFrame();
    if (m_pGlfwWindow)
    {
        ImGui_ImplGlfw_NewFrame();
    }
    else
    {
        // When there is no Glfw window, we must initialize the display size ourselves
        auto& rIo = ImGui::GetIO();
        rIo.DisplaySize.x = rVkViewportSize.width;
        rIo.DisplaySize.y = rVkViewportSize.height;
    }
    ImGui::NewFrame();

    m_pWorkspace->draw(rCommandBuffer);

    ImGui::Render();
    m_pBackend->prepareExecution(rCommandBuffer);
    {
        auto pBarrier = rCommandBuffer.startScopedBarrier("finalizeImage");
        pBarrier->addImageBarrier(*m_pFrame, ImageBarrierConfig{
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
    blitToOutputImage(m_pDevice->getFcts(), rCommandBuffer.getVkCommandBuffer(), *m_pFrame, *pOutputImage);
}

void ImguiPipeline::resize(const VkExtent2D& rVkExtent, uint32_t frameInFlightCount)
{
    auto pContextGuard = m_pContext->makeCurrent();

    constexpr VkFormat OutputFormat = VK_FORMAT_R16G16B16A16_SFLOAT;

    m_pFrame = m_pDevice->getImageFactory()->createImage(ImageConfig{
        .name = "ImguiPipelineImage",
        .vkExtent = rVkExtent,
        .vkFormat = OutputFormat,
        .vkUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
    });

    m_pBackend.reset();  // reset first, to avoid having more than one backend at a time
    m_pBackend = std::make_unique<ImguiVulkanBackend>(m_pDevice, m_pFrame, frameInFlightCount, m_pGlfwWindow);

    m_pWorkspace->onWindowResized(rVkExtent, OutputFormat, frameInFlightCount);
}