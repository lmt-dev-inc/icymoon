#include "vulkan_command_buffer.h"

#include <algorithm>

using namespace im3e;

namespace im3e {

class VulkanCommandBufferFuture : public ICommandBufferFuture
{
public:
    VulkanCommandBufferFuture(std::weak_ptr<VulkanCommandBuffer> pCommandBuffer)
      : m_pCommandBuffer(pCommandBuffer)
    {
    }

    void waitForCompletion() override
    {
        auto pCommandBuffer = m_pCommandBuffer.lock();
        if (m_isComplete || !pCommandBuffer)
        {
            m_isComplete = true;
            return;
        }
        pCommandBuffer->waitForCompletion();
        m_isComplete = true;
    }

    void markAsComplete() { m_isComplete = true; }

private:
    std::weak_ptr<VulkanCommandBuffer> m_pCommandBuffer;
    bool m_isComplete = false;
};

}  // namespace im3e

namespace {

class VulkanCommandBarrierRecorder : public ICommandBarrierRecorder
{
public:
    VulkanCommandBarrierRecorder(std::string_view name, std::shared_ptr<const ICommandBuffer> pCommandBuffer,
                                 const VulkanDeviceFcts& rFcts)
      : m_name(name)
      , m_pCommandBuffer(throwIfArgNull(std::move(pCommandBuffer),
                                        "Cannot create Vulkan command barrier recorder without a command buffer"))
      , m_rFcts(rFcts)
    {
    }

    ~VulkanCommandBarrierRecorder() override
    {
        if (m_vkImageBarriers.empty())
        {
            return;
        }

        VkDependencyInfo vkInfo{
            .sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
            .dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT,
            .imageMemoryBarrierCount = static_cast<uint32_t>(m_vkImageBarriers.size()),
            .pImageMemoryBarriers = m_vkImageBarriers.data(),
        };
        m_rFcts.vkCmdPipelineBarrier2(m_pCommandBuffer->getVkCommandBuffer(), &vkInfo);
    }

    void addImageBarrier(IImage& rImage, ImageBarrierConfig config) override
    {
        auto pMetadata = rImage.getMetadata();

        m_vkImageBarriers.emplace_back(VkImageMemoryBarrier2{
            .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
            .srcStageMask = pMetadata->getLastStageMask(),
            .srcAccessMask = pMetadata->getLastAccessMask(),
            .dstStageMask = config.vkDstStageMask,
            .dstAccessMask = config.vkDstAccessMask,
            .oldLayout = pMetadata->getLayout(),
            .newLayout = config.vkLayout.has_value() ? config.vkLayout.value() : pMetadata->getLayout(),
            .srcQueueFamilyIndex = pMetadata->getQueueFamilyIndex(),
            .dstQueueFamilyIndex = pMetadata->getQueueFamilyIndex(),
            .image = rImage.getVkImage(),
            .subresourceRange{
                .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                .levelCount = 1U,
                .layerCount = 1U,
            },
        });

        if (config.vkLayout.has_value())
        {
            pMetadata->setLayout(config.vkLayout.value());
        }
        pMetadata->setLastStageMask(config.vkDstStageMask);
        pMetadata->setLastAccessMask(config.vkDstAccessMask);
    }

private:
    const std::string m_name;
    std::shared_ptr<const ICommandBuffer> m_pCommandBuffer;
    const VulkanDeviceFcts& m_rFcts;
    std::vector<VkImageMemoryBarrier2> m_vkImageBarriers;
};

void setVkObjectDebugName(VkDevice vkDevice, const VulkanDeviceFcts& rFcts, VkObjectType vkObjectType, void* pVkObject,
                          std::string_view name)
{
    if (!rFcts.vkSetDebugUtilsObjectNameEXT)
    {
        return;
    }

    VkDebugUtilsObjectNameInfoEXT vkDebugInfo{
        .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT,
        .objectType = vkObjectType,
        .objectHandle = reinterpret_cast<uint64_t>(pVkObject),
        .pObjectName = name.data(),
    };
    throwIfVkFailed(rFcts.vkSetDebugUtilsObjectNameEXT(vkDevice, &vkDebugInfo),
                    fmt::format("Failed to set debug name to object \"{}\"", name));
}

auto createVkCommandBuffer(VkDevice vkDevice, const VulkanDeviceFcts& rFcts, VkCommandPool vkCommandPool,
                           std::string_view name)
{
    VkCommandBufferAllocateInfo vkAllocateInfo{
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .commandPool = vkCommandPool,
        .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandBufferCount = 1U,
    };

    VkCommandBuffer vkCommandBuffer{};
    throwIfVkFailed(rFcts.vkAllocateCommandBuffers(vkDevice, &vkAllocateInfo, &vkCommandBuffer),
                    "Failed to allocate command for command queue pool");

    VkUniquePtr<VkCommandBuffer> pCommandBuffer(
        vkCommandBuffer, [vkDevice, vkCommandPool, pFcts = &rFcts](auto* vkCmdBuffer) {
            pFcts->vkFreeCommandBuffers(vkDevice, vkCommandPool, 1U, &vkCmdBuffer);
        });
    setVkObjectDebugName(vkDevice, rFcts, VK_OBJECT_TYPE_COMMAND_BUFFER, vkCommandBuffer, name);
    return pCommandBuffer;
}

}  // namespace

VulkanCommandBuffer::VulkanCommandBuffer(const ICommandQueue& rQueue, const IDevice& rDevice,
                                         VkCommandPool vkCommandPool, std::string_view name)
  : m_rQueue(rQueue)
  , m_rDevice(rDevice)
  , m_pLogger(m_rDevice.createLogger(name))
  , m_name(name)
  , m_pVkCommandBuffer(createVkCommandBuffer(m_rDevice.getVkDevice(), m_rDevice.getFcts(), vkCommandPool,
                                             fmt::format("Im3eCommandBuffer.{}", name)))
  // Create the fence signalled so that we consider the command as complete until we actually submit work to the queue
  , m_pVkFence(m_rDevice.createVkFence(VK_FENCE_CREATE_SIGNALED_BIT))
{
}

VulkanCommandBuffer::~VulkanCommandBuffer()
{
    if (m_inFlight)
    {
        waitForCompletion();
    }
}

auto VulkanCommandBuffer::startScopedBarrier(std::string_view name) const -> std::unique_ptr<ICommandBarrierRecorder>
{
    return make_unique<VulkanCommandBarrierRecorder>(name, this->shared_from_this(), m_rDevice.getFcts());
}

auto VulkanCommandBuffer::createFuture() -> std::shared_ptr<ICommandBufferFuture>
{
    auto pFuture = make_shared<VulkanCommandBufferFuture>(this->shared_from_this());
    m_pFutures.emplace_back(pFuture);
    return pFuture;
}

void VulkanCommandBuffer::reset()
{
    throwIfVkFailed(m_rDevice.getFcts().vkResetCommandBuffer(m_pVkCommandBuffer.get(), 0U),
                    "Failed to reset command buffer");

    m_inFlight = false;
    std::ranges::for_each(m_pFutures, [](auto& pFuture) { pFuture->markAsComplete(); });

    // Note: we do not reset the fence here is that it remains marked as complete until we actually submit something
    // new to the queue. Otherwise, we risk waiting for the fence while nothing is executing.

    m_pVkSignalSemaphore.reset();
    m_pVkWaitSemaphore.reset();
    m_pFutures.clear();
}

void VulkanCommandBuffer::beginRecording(std::string_view)
{
    VkCommandBufferBeginInfo vkBeginInfo{
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
    };
    throwIfVkFailed(m_rDevice.getFcts().vkBeginCommandBuffer(m_pVkCommandBuffer.get(), &vkBeginInfo),
                    "Could not begin command buffer recording");
}

void VulkanCommandBuffer::endRecording()
{
    throwIfVkFailed(m_rDevice.getFcts().vkEndCommandBuffer(m_pVkCommandBuffer.get()),
                    "Failed to end command buffer recording");
}

void VulkanCommandBuffer::submitToQueue(CommandExecutionType executionType)
{
    // Reset the fence right before submitting to queue so that the fence is always set between the moment an execution
    // is complete and the moment a new command is submitted. Otherwise, we risk waiting for a fence that will never
    // be set.
    const auto vkFence = m_pVkFence.get();
    throwIfVkFailed(m_rDevice.getFcts().vkResetFences(m_rDevice.getVkDevice(), 1U, &vkFence),
                    "Failed to reset command buffer fence");

    const auto vkCommandBuffer = m_pVkCommandBuffer.get();
    const auto vkSignalSemaphore = m_pVkSignalSemaphore.get();
    VkSubmitInfo vkSubmitInfo{
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .commandBufferCount = 1U,
        .pCommandBuffers = &vkCommandBuffer,
        .signalSemaphoreCount = vkSignalSemaphore ? 1U : 0U,
        .pSignalSemaphores = vkSignalSemaphore ? &vkSignalSemaphore : nullptr,
    };
    const VkPipelineStageFlags vkWaitDstMask = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
    const auto vkWaitSemaphore = m_pVkWaitSemaphore.get();
    if (m_pVkWaitSemaphore)
    {
        vkSubmitInfo.waitSemaphoreCount = 1U;
        vkSubmitInfo.pWaitSemaphores = &vkWaitSemaphore;
        vkSubmitInfo.pWaitDstStageMask = &vkWaitDstMask;
    }
    throwIfVkFailed(m_rDevice.getFcts().vkQueueSubmit(m_rQueue.getVkQueue(), 1U, &vkSubmitInfo, m_pVkFence.get()),
                    "Failed to execute command buffer");
    m_inFlight = true;

    if (executionType == CommandExecutionType::Sync)
    {
        this->waitForCompletion();
    }
}

void VulkanCommandBuffer::waitForCompletion()
{
    if (!m_inFlight)
    {
        return;
    }

    const auto vkFence = m_pVkFence.get();
    logIfVkFailed(m_rDevice.getFcts().vkWaitForFences(m_rDevice.getVkDevice(), 1U, &vkFence, VK_TRUE,
                                                      std::numeric_limits<uint64_t>::max()),
                  *m_pLogger, "Failed to wait for fence while destroying command buffer");

    m_inFlight = false;
    std::ranges::for_each(m_pFutures, [](auto& pFuture) { pFuture->markAsComplete(); });

    this->reset();
}

auto VulkanCommandBuffer::isExecutionComplete() const -> bool
{
    if (!m_inFlight)
    {
        return false;
    }

    const auto vkFence = m_pVkFence.get();
    const auto vkResult = m_rDevice.getFcts().vkWaitForFences(m_rDevice.getVkDevice(), 1U, &vkFence, VK_TRUE, 0U);
    if (vkResult == VK_SUCCESS)
    {
        return true;
    }
    else if (vkResult == VK_TIMEOUT)
    {
        return false;
    }
    throwIfVkFailed(vkResult, "Failed to query state of command buffer fence");
    return false;
}
