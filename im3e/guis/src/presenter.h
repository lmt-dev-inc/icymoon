#pragma once

#include <im3e/vulkan/device.h>
#include <im3e/vulkan/frame_pipeline.h>

namespace im3e {

class Presenter
{
public:
    Presenter(std::shared_ptr<IDevice> pDevice, VkSurfaceKHR vkSurface, std::unique_ptr<IFramePipeline> pFramePipeline);
    ~Presenter();

    void present();
    void reset();

private:
    std::shared_ptr<IDevice> m_pDevice;
    VkSurfaceKHR m_vkSurface;
    std::shared_ptr<IFramePipeline> m_pFramePipeline;

    std::unique_ptr<ILogger> m_pLogger;

    VkUniquePtr<VkSwapchainKHR> m_pVkSwapchain;
    std::vector<std::shared_ptr<IImage>> m_pImages;
    VkExtent2D m_vkExtent{};
    bool m_isOutOfDate = true;

    std::vector<VkSharedPtr<VkSemaphore>> m_pReadyToWriteSemaphores;
    size_t m_readyToWriteSemaphoreIndex{};

    std::vector<VkSharedPtr<VkSemaphore>> m_pReadyToPresentSemaphores;
    std::vector<std::shared_ptr<ICommandBufferFuture>> m_pCommandFutures;
    size_t m_commandIndex{};
};

}  // namespace im3e