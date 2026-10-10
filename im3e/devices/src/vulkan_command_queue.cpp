#include "vulkan_command_queue.h"

#include "vulkan_command_buffer.h"

using namespace im3e;

namespace {

auto createVkCommandPool(VkDevice vkDevice, const VulkanDeviceFcts& rFcts, uint32_t queueFamilyIndex)
{
    VkCommandPoolCreateInfo vkCreateInfo{
        .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
        .queueFamilyIndex = queueFamilyIndex,
    };

    VkCommandPool vkCommandPool{};
    throwIfVkFailed(rFcts.vkCreateCommandPool(vkDevice, &vkCreateInfo, nullptr, &vkCommandPool),
                    "Failed to create pool for command queue");

    return makeVkUniquePtr<VkCommandPool>(vkDevice, vkCommandPool, rFcts.vkDestroyCommandPool);
}

class VulkanCommandQueue : public ICommandQueue, public std::enable_shared_from_this<VulkanCommandQueue>
{
public:
    VulkanCommandQueue(const IDevice& rDevice, VulkanCommandQueueInfo queueInfo, std::string_view name)
      : m_rDevice(rDevice)
      , m_queueInfo(std::move(queueInfo))
      , m_name(name)
      , m_pVkCommandPool(createVkCommandPool(m_rDevice.getVkDevice(), m_rDevice.getFcts(), queueInfo.queueFamilyIndex))
    {
    }

    ~VulkanCommandQueue()
    {
        for (auto* pInFlight : m_pInFlight)
        {
            pInFlight->waitForCompletion();
        }
    }

    auto startScopedCommand(std::string_view name, CommandExecutionType executionType)
        -> UniquePtrWithDeleter<ICommandBuffer> override
    {
        this->_releaseCompletedCommands();

        // If no buffer is available, create a new one:
        VulkanCommandBuffer* pCommandBuffer{};
        if (m_pAvailable.empty())
        {
            m_pVkCommandBuffers.emplace_back(std::make_shared<VulkanCommandBuffer>(
                *this, m_rDevice, m_pVkCommandPool.get(), fmt::format("{}_{}", m_name, m_pVkCommandBuffers.size())));
            pCommandBuffer = m_pVkCommandBuffers.back().get();
        }
        else
        {
            pCommandBuffer = m_pAvailable.back();
            m_pAvailable.pop_back();
        }

        pCommandBuffer->beginRecording(name);
        return UniquePtrWithDeleter<ICommandBuffer>(
            pCommandBuffer, [pThis = this->shared_from_this(), executionType, pVulkanCommand = pCommandBuffer](auto*) {
                pVulkanCommand->endRecording();
                pVulkanCommand->submitToQueue(executionType);
                if (executionType == CommandExecutionType::Sync)
                {
                    pThis->m_pAvailable.emplace_back(pVulkanCommand);
                }
                else
                {
                    pThis->m_pInFlight.emplace_back(pVulkanCommand);
                }
            });
    }

    void waitIdle() override
    {
        m_rDevice.getFcts().vkQueueWaitIdle(m_queueInfo.vkQueue);
        this->_releaseCompletedCommands();
    }

    auto getQueueFamilyIndex() const -> uint32_t override { return m_queueInfo.queueFamilyIndex; }
    auto getVkQueue() const -> VkQueue override { return m_queueInfo.vkQueue; }

private:
    void _releaseCompletedCommands()
    {
        // Release any in-flight buffer whose execution is complete:
        auto itInFlight = m_pInFlight.begin();
        while (itInFlight != m_pInFlight.end())
        {
            if (!(*itInFlight)->isExecutionComplete())
            {
                itInFlight++;
                continue;
            }
            (*itInFlight)->reset();
            m_pAvailable.emplace_back((*itInFlight));
            itInFlight = m_pInFlight.erase(itInFlight);
        }
    }

    const IDevice& m_rDevice;
    const VulkanCommandQueueInfo m_queueInfo;
    const std::string m_name;

    VkUniquePtr<VkCommandPool> m_pVkCommandPool;
    std::vector<std::shared_ptr<VulkanCommandBuffer>> m_pVkCommandBuffers;
    std::vector<VulkanCommandBuffer*> m_pInFlight;
    std::vector<VulkanCommandBuffer*> m_pAvailable;
};

}  // namespace

auto im3e::createVulkanCommandQueue(const IDevice& rDevice, VulkanCommandQueueInfo queueInfo, std::string_view name)
    -> std::shared_ptr<ICommandQueue>
{
    return std::make_shared<VulkanCommandQueue>(rDevice, std::move(queueInfo), name);
}
