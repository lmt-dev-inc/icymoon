#pragma once

#include "imgui_context.h"
#include "imgui_vulkan_backend.h"
#include "imgui_workspace.h"

#include <im3e/api/gui.h>
#include <im3e/vulkan/device.h>
#include <im3e/vulkan/frame_pipeline.h>
#include <im3e/vulkan/image.h>

#include <GLFW/glfw3.h>

#include <memory>

namespace im3e {

class ImguiPipeline : public IFramePipeline
{
public:
    ImguiPipeline(std::shared_ptr<const IDevice> pDevice, GLFWwindow* pGlfwWindow,
                  std::shared_ptr<ImguiWorkspace> pWorkspace, std::optional<std::string> iniFilename = {});
    ~ImguiPipeline() override;

    void prepareExecution(const ICommandBuffer& rCommandBuffer, const VkExtent2D& rVkViewportSize,
                          std::shared_ptr<IImage> pOutputImage) override;

    void resize(const VkExtent2D& rVkExtent, uint32_t frameInFlightCount) override;

    auto getDevice() const -> std::shared_ptr<const IDevice> override { return m_pDevice; }

private:
    std::shared_ptr<const IDevice> m_pDevice;
    std::unique_ptr<ILogger> m_pLogger;
    GLFWwindow* m_pGlfwWindow;
    std::shared_ptr<ImguiWorkspace> m_pWorkspace;
    std::optional<std::string> m_iniFilename;

    std::unique_ptr<ImguiContext> m_pContext;
    std::shared_ptr<IImage> m_pFrame;
    std::unique_ptr<ImguiVulkanBackend> m_pBackend;
};

}  // namespace im3e