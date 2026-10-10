#include "imgui_vulkan_backend.h"

#include <im3e/utils/core/throw_utils.h>

#include <imgui_impl_glfw.h>
#include <imgui_impl_vulkan.h>

using namespace im3e;

namespace {

auto makeRenderPass(const VulkanDeviceFcts& rFcts, VkDevice vkDevice, VkFormat vkDstImageFormat)
{
    VkAttachmentDescription vkAttachment{
        .format = vkDstImageFormat,
        .samples = VK_SAMPLE_COUNT_1_BIT,
        .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
        .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
        .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
        .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
        .initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        .finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
    };

    VkAttachmentReference vkAttachmentRef{
        .attachment = 0U,
        .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
    };

    VkSubpassDescription vkSubpass{
        .pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
        .colorAttachmentCount = 1U,
        .pColorAttachments = &vkAttachmentRef,
    };

    VkSubpassDependency vkSubpassDependency{
        .srcSubpass = VK_SUBPASS_EXTERNAL,
        .dstSubpass = 0U,
        .srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
        .dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
        .srcAccessMask = 0U,
        .dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
    };

    VkRenderPassCreateInfo vkCreateInfo{
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
        .attachmentCount = 1U,
        .pAttachments = &vkAttachment,
        .subpassCount = 1U,
        .pSubpasses = &vkSubpass,
        .dependencyCount = 1U,
        .pDependencies = &vkSubpassDependency,
    };

    VkRenderPass vkRenderPass{};
    throwIfVkFailed(rFcts.vkCreateRenderPass(vkDevice, &vkCreateInfo, nullptr, &vkRenderPass),
                    "Could not create render pass for ImGui backend");

    return makeVkUniquePtr<VkRenderPass>(vkDevice, vkRenderPass, rFcts.vkDestroyRenderPass);
}

auto makeDescriptorPool(const VulkanDeviceFcts& rFcts, VkDevice vkDevice)
{
    const std::vector<VkDescriptorPoolSize> poolSizes{
        {VK_DESCRIPTOR_TYPE_SAMPLER, 1000U},
        {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000U},
        {VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000U},
        {VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000U},
        {VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000U},
        {VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000U},
        {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000U},
        {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000U},
        {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000U},
        {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000U},
        {VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000U},
    };
    VkDescriptorPoolCreateInfo createInfo{
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
        .flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT,
        .maxSets = 1000U * static_cast<uint32_t>(poolSizes.size()),
        .poolSizeCount = static_cast<uint32_t>(poolSizes.size()),
        .pPoolSizes = poolSizes.data(),
    };

    VkDescriptorPool pVkDescriptorPool{};
    throwIfVkFailed(rFcts.vkCreateDescriptorPool(vkDevice, &createInfo, nullptr, &pVkDescriptorPool),
                    "Failed to create descriptor pool for ImGui backend");

    return makeVkUniquePtr<VkDescriptorPool>(vkDevice, pVkDescriptorPool, rFcts.vkDestroyDescriptorPool);
}

auto makeFramebuffer(const VulkanDeviceFcts& rFcts, VkDevice vkDevice, VkRenderPass vkRenderPass,
                     VkImageView vkImageView, const VkExtent2D& rExtent)
{
    VkFramebufferCreateInfo vkCreateInfo{
        .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
        .renderPass = vkRenderPass,
        .attachmentCount = 1U,
        .pAttachments = &vkImageView,
        .width = rExtent.width,
        .height = rExtent.height,
        .layers = 1U,
    };

    VkFramebuffer vkFramebuffer{};
    throwIfVkFailed(rFcts.vkCreateFramebuffer(vkDevice, &vkCreateInfo, nullptr, &vkFramebuffer),
                    "Could not create framebuffer for ImGui backend");

    return makeVkUniquePtr<VkFramebuffer>(vkDevice, vkFramebuffer, rFcts.vkDestroyFramebuffer);
}

void initializeImguiVulkan(const ILogger& rLogger, const IDevice& rDevice, VkDescriptorPool vkDescriptorPool,
                           uint32_t frameInFlightCount, VkRenderPass vkRenderPass)
{
    ImGui_ImplVulkan_LoadFunctions(
        [](const char* pFctName, void* pUserData) {
            auto* pDevice = reinterpret_cast<const IDevice*>(pUserData);
            return throwIfNull<std::runtime_error>(
                pDevice->getInstanceFcts().vkGetInstanceProcAddr(pDevice->getVkInstance(), pFctName),
                fmt::format("Failed to load function for ImGui: \"{}\" not found", pFctName));
        },
        const_cast<IDevice*>(&rDevice));

    const auto pCommandQueue = rDevice.getCommandQueue();

    ImGui_ImplVulkan_InitInfo initInfo{
        .Instance = rDevice.getVkInstance(),
        .PhysicalDevice = rDevice.getVkPhysicalDevice(),
        .Device = rDevice.getVkDevice(),
        .QueueFamily = pCommandQueue->getQueueFamilyIndex(),
        .Queue = pCommandQueue->getVkQueue(),
        .DescriptorPool = vkDescriptorPool,
        .RenderPass = vkRenderPass,
        .MinImageCount = frameInFlightCount,
        .ImageCount = frameInFlightCount,
        .CheckVkResultFn = [](auto vkResult) { throwIfVkFailed(vkResult, "Vulkan error in ImGui backend"); },
    };
    ImGui_ImplVulkan_Init(&initInfo);
    rLogger.debug("Successfully initialized ImGui Vulkan backend");
}

auto beginRenderPass(const VulkanDeviceFcts& rFcts, VkCommandBuffer vkCommandBuffer, VkRenderPass vkRenderPass,
                     VkFramebuffer vkFramebuffer, const VkExtent2D& rExtent)
{
    VkClearValue vkClearValue{};

    VkRenderPassBeginInfo vkBeginInfo{
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
        .renderPass = vkRenderPass,
        .framebuffer = vkFramebuffer,
        .renderArea = {.extent = rExtent},
        .clearValueCount = 1U,
        .pClearValues = &vkClearValue,
    };
    rFcts.vkCmdBeginRenderPass(vkCommandBuffer, &vkBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

    return VkUniquePtr<VkCommandBuffer>(
        vkCommandBuffer, [pFcts = &rFcts](auto* pCommandBuffer) { pFcts->vkCmdEndRenderPass(pCommandBuffer); });
}

}  // namespace

ImguiVulkanBackend::ImguiVulkanBackend(std::shared_ptr<const IDevice> pDevice, std::shared_ptr<IImage> pOutputImage,
                                       uint32_t frameInFlightCount, GLFWwindow* pGlfwWindow)
  : m_pDevice(throwIfArgNull(std::move(pDevice), "ImGui Vulkan backend requires a device"))
  , m_pOutputImage(throwIfArgNull(std::move(pOutputImage), "ImGui Vulkan backend requires an output image"))
  , m_pOutputImageView(m_pOutputImage->createView())
  , m_pGlfwWindow(pGlfwWindow)
  , m_pLogger(m_pDevice->createLogger("ImGui Vulkan Backend"))
  , m_pVkRenderPass(makeRenderPass(m_pDevice->getFcts(), m_pDevice->getVkDevice(), m_pOutputImage->getVkFormat()))
  , m_pVkDescriptorPool(makeDescriptorPool(m_pDevice->getFcts(), m_pDevice->getVkDevice()))
  , m_pVkFramebuffer(makeFramebuffer(m_pDevice->getFcts(), m_pDevice->getVkDevice(), m_pVkRenderPass.get(),
                                     m_pOutputImageView->getVkImageView(), m_pOutputImage->getVkExtent()))
{
    if (m_pGlfwWindow)
    {
        constexpr auto InstallCallbacks = true;
        ImGui_ImplGlfw_InitForVulkan(m_pGlfwWindow, InstallCallbacks);
    }

    // ImGui requires that the frame in flight count must be at least two:
    constexpr uint32_t DefaultFrameInFlightCount = 2U;
    frameInFlightCount = std::max(DefaultFrameInFlightCount, frameInFlightCount);

    initializeImguiVulkan(*m_pLogger, *m_pDevice, m_pVkDescriptorPool.get(), frameInFlightCount, m_pVkRenderPass.get());
}

ImguiVulkanBackend::~ImguiVulkanBackend()
{
    ImGui_ImplVulkan_Shutdown();
    if (m_pGlfwWindow)
    {
        ImGui_ImplGlfw_Shutdown();
    }
}

void ImguiVulkanBackend::prepareExecution(const ICommandBuffer& rCommandBuffer)
{
    {
        auto pBarrierRecorder = rCommandBuffer.startScopedBarrier("BeforeImGuiVulkanBackendRender");
        pBarrierRecorder->addImageBarrier(*m_pOutputImage,
                                          ImageBarrierConfig{
                                              .vkDstStageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
                                              .vkDstAccessMask = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT,
                                              .vkLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                                          });
    }

    const auto vkCommandBuffer = rCommandBuffer.getVkCommandBuffer();
    auto pRenderPassGuard = beginRenderPass(m_pDevice->getFcts(), vkCommandBuffer, m_pVkRenderPass.get(),
                                            m_pVkFramebuffer.get(), m_pOutputImage->getVkExtent());
    ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), vkCommandBuffer);
}
