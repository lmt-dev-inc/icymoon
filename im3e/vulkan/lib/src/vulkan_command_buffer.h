#pragma once

#include <im3e/utils/loggers.h>
#include <im3e/vulkan/command_buffer.h>
#include <im3e/vulkan/device.h>
#include <im3e/vulkan/utils.h>

#include <memory>
#include <string>
#include <string_view>
#include <vector>

namespace im3e {

class VulkanCommandBufferFuture;

class VulkanCommandBuffer : public ICommandBuffer, public std::enable_shared_from_this<VulkanCommandBuffer>
{
public:
    VulkanCommandBuffer(const ICommandQueue& queue, const IDevice& device, VkCommandPool vkCommandPool,
                        std::string_view name);
    ~VulkanCommandBuffer() override;

    auto startScopedBarrier(std::string_view name) const -> std::unique_ptr<ICommandBarrierRecorder> override;
    auto createFuture() -> std::shared_ptr<ICommandBufferFuture> override;

    void setVkSignalSemaphore(VkSharedPtr<VkSemaphore> vkSemaphore) override { m_vkSignalSemaphore = vkSemaphore; }
    void setVkWaitSemaphore(VkSharedPtr<VkSemaphore> vkSemaphore) override { m_vkWaitSemaphore = vkSemaphore; }

    void reset();
    void beginRecording(std::string_view);
    void endRecording();

    void submitToQueue(CommandExecutionType executionType);
    void waitForCompletion();
    auto isExecutionComplete() const -> bool;

    auto getVkCommandBuffer() const -> VkCommandBuffer override { return m_vkCommandBuffer.get(); }

private:
    const ICommandQueue& m_queue;
    const IDevice& m_device;
    std::unique_ptr<ILogger> m_logger;
    const std::string m_name;

    VkUniquePtr<VkCommandBuffer> m_vkCommandBuffer;
    VkSharedPtr<VkFence> m_vkFence;
    bool m_inFlight{};

    VkSharedPtr<VkSemaphore> m_vkSignalSemaphore;
    VkSharedPtr<VkSemaphore> m_vkWaitSemaphore;
    std::vector<std::shared_ptr<VulkanCommandBufferFuture>> m_futures;
};

}  // namespace im3e