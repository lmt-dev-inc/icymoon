#pragma once

#include <im3e/vulkan/device.h>
#include <im3e/vulkan/frame_pipeline.h>
#include <im3e/vulkan/image.h>
#include <im3e/vulkan/vk_utils.h>

#include <GLFW/glfw3.h>

namespace im3e {

/// @brief Vulkan back-end for rendering ImGui.
/// @note An ImguiContext must be made active while using the back-end.
class ImguiVulkanBackend
{
public:
    ImguiVulkanBackend(std::shared_ptr<const IDevice> pDevice, std::shared_ptr<IImage> pOutputImage,
                       uint32_t frameInFlightCount, GLFWwindow* pGlfwWindow);
    ~ImguiVulkanBackend();

    void prepareExecution(const ICommandBuffer& rCommandBuffer);

private:
    std::shared_ptr<const IDevice> m_pDevice;
    std::shared_ptr<IImage> m_pOutputImage;
    std::unique_ptr<IImageView> m_pOutputImageView;
    GLFWwindow* m_pGlfwWindow{};

    std::unique_ptr<ILogger> m_pLogger;
    VkUniquePtr<VkRenderPass> m_pVkRenderPass;
    VkUniquePtr<VkDescriptorPool> m_pVkDescriptorPool;
    VkUniquePtr<VkFramebuffer> m_pVkFramebuffer;
};

}  // namespace im3e