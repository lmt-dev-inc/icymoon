#pragma once

#include <im3e/utils/loggers.h>
#include <im3e/vulkan/command_buffer.h>
#include <im3e/vulkan/device.h>
#include <im3e/vulkan/frame_pipeline.h>
#include <im3e/vulkan/image.h>
#include <im3e/vulkan/utils.h>

#include <memory>
#include <vector>

namespace im3e {

class Presenter
{
public:
    Presenter(std::shared_ptr<IDevice> device, VkSurfaceKHR vkSurface, std::unique_ptr<IFramePipeline> framePipeline);
    ~Presenter();

    void present();
    void reset();

private:
    std::shared_ptr<IDevice> m_device;
    VkSurfaceKHR m_vkSurface{};
    std::shared_ptr<IFramePipeline> m_framePipeline;

    std::unique_ptr<ILogger> m_logger;

    VkUniquePtr<VkSwapchainKHR> m_vkSwapchain;
    std::vector<std::shared_ptr<IImage>> m_images;
    VkExtent2D m_vkExtent{};
    bool m_isOutOfDate{true};

    std::vector<VkSharedPtr<VkSemaphore>> m_vkReadyToWriteSemaphores;
    size_t m_readyToWriteSemaphoreIndex{};

    std::vector<VkSharedPtr<VkSemaphore>> m_vkReadyToPresentSemaphores;
    std::vector<std::shared_ptr<ICommandBufferFuture>> m_commandFutures;
    size_t m_commandIndex{};
};

}  // namespace im3e
