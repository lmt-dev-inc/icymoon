#include "imgui_render_panel.h"

#include <im3e/utils/core/throw_utils.h>

#include <imgui.h>
#include <imgui_impl_vulkan.h>

#include <array>

using namespace im3e;

namespace {

constexpr auto RenderOutputFinalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

auto getWindowContentRegionSize()
{
    const auto imguiContentRegionMin = ImGui::GetWindowContentRegionMin();
    const auto imguiContentRegionMax = ImGui::GetWindowContentRegionMax();
    return ImVec2{
        ceilf(imguiContentRegionMax.x - imguiContentRegionMin.x),
        ceilf(imguiContentRegionMax.y - imguiContentRegionMin.y),
    };
}

auto toVkExtent2D(const ImVec2& rImVec2)
{
    return VkExtent2D{
        .width = static_cast<uint32_t>(rImVec2.x),
        .height = static_cast<uint32_t>(rImVec2.y),
    };
}

auto createRenderOutputSampler(const IDevice& rDevice)
{
    const auto vkDevice = rDevice.getVkDevice();
    const auto& rVkFcts = rDevice.getFcts();

    VkSamplerCreateInfo vkSamplerInfo{
        .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
        .magFilter = VK_FILTER_LINEAR,
        .minFilter = VK_FILTER_LINEAR,
    };

    VkSampler vkSampler{};
    throwIfVkFailed(rVkFcts.vkCreateSampler(vkDevice, &vkSamplerInfo, nullptr, &vkSampler),
                    "Failed to create sampler for render panel output");

    return makeVkUniquePtr<VkSampler>(vkDevice, vkSampler, rVkFcts.vkDestroySampler);
}

void dispatchEvents(ImGuiIO& rIo, IGuiEventListener& rEventListener)
{
    if (ImGui::IsItemHovered() && rIo.MouseWheel)
    {
        rEventListener.onMouseWheel(rIo.MouseWheel);
    }

    if (ImGui::IsItemActive() && (rIo.MouseDelta.x || rIo.MouseDelta.y))
    {
        const auto imViewportSize = getWindowContentRegionSize();
        const glm::vec2 normalizedMouseOffset{
            rIo.MouseDelta.x * 2.0F / static_cast<float>(imViewportSize.x),
            rIo.MouseDelta.y * 2.0F / static_cast<float>(imViewportSize.y),
        };

        const std::array<bool, 3U> mouseButtonsDown{rIo.MouseDown[0], rIo.MouseDown[1], rIo.MouseDown[2]};

        rEventListener.onMouseMove(normalizedMouseOffset, mouseButtonsDown);
    }
}

}  // namespace

ImguiRenderPanel::ImguiRenderPanel(std::string_view name, std::unique_ptr<IFramePipeline> pFramePipeline,
                                   std::shared_ptr<IGuiEventListener> pEventListener)
  : m_name(name)
  , m_pFramePipeline(throwIfArgNull(std::move(pFramePipeline), "ImGui render panel requires a frame pipeline"))
  , m_pDevice(m_pFramePipeline->getDevice())
  , m_pRenderOutputSampler(createRenderOutputSampler(*m_pDevice))
  , m_pEventListener(std::move(pEventListener))
{
}

void ImguiRenderPanel::draw(const ICommandBuffer& rCommandBuffer)
{
    if (!m_pRenderOutput)
    {
        return;
    }

    const auto imViewportSize = getWindowContentRegionSize();
    if (imViewportSize.x == 0 || imViewportSize.y == 0)
    {
        return;
    }

    m_pFramePipeline->prepareExecution(rCommandBuffer, toVkExtent2D(imViewportSize), m_pRenderOutput);
    {
        auto pBarrierRecorder = rCommandBuffer.startScopedBarrier(fmt::format("barrierToDrawIn{}", m_name));
        pBarrierRecorder->addImageBarrier(*m_pRenderOutput,
                                          ImageBarrierConfig{
                                              .vkDstStageMask = VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT,
                                              .vkDstAccessMask = VK_ACCESS_2_SHADER_READ_BIT,
                                              .vkLayout = RenderOutputFinalLayout,
                                          });
    }

    const auto imagePos = ImGui::GetCursorScreenPos();
    const auto renderOutputSize = m_pRenderOutput->getVkExtent();
    const ImVec2 maxUV{
        imViewportSize.x / static_cast<float>(renderOutputSize.width),
        imViewportSize.y / static_cast<float>(renderOutputSize.height),
    };

    ImGui::SetNextItemAllowOverlap();
    ImGui::Image(reinterpret_cast<ImTextureID>(m_vkRenderOutputSet), imViewportSize, ImVec2{}, maxUV);
    ImGui::SetCursorScreenPos(imagePos);
    ImGui::InvisibleButton(fmt::format("{}_mouseArea", m_name).c_str(), imViewportSize,
                           ImGuiButtonFlags_MouseButtonLeft | ImGuiButtonFlags_MouseButtonMiddle);
    ImGui::SetItemKeyOwner(ImGuiKey_MouseWheelY);

    if (m_pEventListener)
    {
        dispatchEvents(ImGui::GetIO(), *m_pEventListener);
    }
}

void ImguiRenderPanel::onWindowResized(const VkExtent2D& rVkWindowSize, VkFormat vkFormat, uint32_t frameInFlightCount)
{
    m_pFramePipeline->resize(rVkWindowSize, frameInFlightCount);

    m_pRenderOutput = m_pDevice->getImageFactory()->createImage(ImageConfig{
        .name = fmt::format("{}.RenderOutput", m_name),
        .vkExtent = rVkWindowSize,
        .vkFormat = vkFormat,
        .vkUsage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
    });
    m_pRenderOutputView = m_pRenderOutput->createView();
    m_vkRenderOutputSet = ImGui_ImplVulkan_AddTexture(m_pRenderOutputSampler.get(),
                                                      m_pRenderOutputView->getVkImageView(), RenderOutputFinalLayout);
}

auto im3e::createImguiRenderPanel(std::string_view name, std::unique_ptr<IFramePipeline> pFramePipeline,
                                  std::shared_ptr<IGuiEventListener> pEventListener) -> std::shared_ptr<IGuiPanel>
{
    return std::make_shared<ImguiRenderPanel>(name, std::move(pFramePipeline), std::move(pEventListener));
}