#include "vulkan_command_buffer.h"

#include <im3e/utils/core/throw_utils.h>
#include <im3e/vulkan/image.h>
#include <im3e/vulkan/sync_utils.h>
#include <im3e/vulkan/utils.h>
#include <im3e/vulkan/vulkan_functions.h>

#include <fmt/format.h>

#include <algorithm>
#include <cstdint>
#include <limits>
#include <memory>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

using namespace im3e;

namespace im3e {

class VulkanCommandBufferFuture : public ICommandBufferFuture
{
public:
    VulkanCommandBufferFuture(std::weak_ptr<VulkanCommandBuffer> commandBuffer)
      : m_commandBuffer(std::move(commandBuffer))
    {
    }

    void waitForCompletion() override
    {
        auto commandBuffer = m_commandBuffer.lock();
        if (m_isComplete || !commandBuffer)
        {
            m_isComplete = true;
            return;
        }
        commandBuffer->waitForCompletion();
        m_isComplete = true;
    }

    void markAsComplete() { m_isComplete = true; }

private:
    std::weak_ptr<VulkanCommandBuffer> m_commandBuffer;
    bool m_isComplete{};
};

}  // namespace im3e

namespace {

class VulkanCommandBarrierRecorder : public ICommandBarrierRecorder
{
public:
    VulkanCommandBarrierRecorder(std::string_view name, std::shared_ptr<const ICommandBuffer> commandBuffer,
                                 const VulkanDeviceFcts& fcts)
      : m_name(name)
      , m_commandBuffer(throwIfArgNull(std::move(commandBuffer),
                                       "Cannot create Vulkan command barrier recorder without a command buffer"))
      , m_fcts(fcts)
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
        m_fcts.vkCmdPipelineBarrier2(m_commandBuffer->getVkCommandBuffer(), &vkInfo);
    }

    void addImageBarrier(IImage& image, ImageBarrierConfig config) override
    {
        auto metadata = image.getMetadata();

        m_vkImageBarriers.emplace_back(VkImageMemoryBarrier2{
            .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
            .srcStageMask = metadata->getLastStageMask(),
            .srcAccessMask = metadata->getLastAccessMask(),
            .dstStageMask = config.vkDstStageMask,
            .dstAccessMask = config.vkDstAccessMask,
            .oldLayout = metadata->getLayout(),
            .newLayout = config.vkLayout.has_value() ? config.vkLayout.value() : metadata->getLayout(),
            .srcQueueFamilyIndex = metadata->getQueueFamilyIndex(),
            .dstQueueFamilyIndex = metadata->getQueueFamilyIndex(),
            .image = image.getVkImage(),
            .subresourceRange{
                .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                .levelCount = 1U,
                .layerCount = 1U,
            },
        });

        if (config.vkLayout.has_value())
        {
            metadata->setLayout(config.vkLayout.value());
        }
        metadata->setLastStageMask(config.vkDstStageMask);
        metadata->setLastAccessMask(config.vkDstAccessMask);
    }

private:
    const std::string m_name;
    std::shared_ptr<const ICommandBuffer> m_commandBuffer;
    const VulkanDeviceFcts& m_fcts;
    std::vector<VkImageMemoryBarrier2> m_vkImageBarriers;
};

void setVkObjectDebugName(VkDevice vkDevice, const VulkanDeviceFcts& fcts, VkObjectType vkObjectType, void* vkObject,
                          std::string_view name)
{
    if (!fcts.vkSetDebugUtilsObjectNameEXT)
    {
        return;
    }

    VkDebugUtilsObjectNameInfoEXT vkDebugInfo{
        .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT,
        .objectType = vkObjectType,
        .objectHandle = reinterpret_cast<uint64_t>(vkObject),
        .pObjectName = name.data(),
    };
    throwIfVkFailed(fcts.vkSetDebugUtilsObjectNameEXT(vkDevice, &vkDebugInfo),
                    fmt::format("Failed to set debug name to object \"{}\"", name));
}

auto createVkCommandBuffer(VkDevice vkDevice, const VulkanDeviceFcts& fcts, VkCommandPool vkCommandPool,
                           std::string_view name) -> VkUniquePtr<VkCommandBuffer>
{
    VkCommandBufferAllocateInfo vkAllocateInfo{
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .commandPool = vkCommandPool,
        .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandBufferCount = 1U,
    };

    VkCommandBuffer vkCommandBuffer{};
    throwIfVkFailed(fcts.vkAllocateCommandBuffers(vkDevice, &vkAllocateInfo, &vkCommandBuffer),
                    "Failed to allocate command for command queue pool");

    // Owned before naming it, so that it is freed if naming fails.
    VkUniquePtr<VkCommandBuffer> vkOwnedCommandBuffer(
        vkCommandBuffer, [vkDevice, vkCommandPool, fcts = &fcts](VkCommandBuffer vkCmdBuffer) {
            fcts->vkFreeCommandBuffers(vkDevice, vkCommandPool, 1U, &vkCmdBuffer);
        });
    setVkObjectDebugName(vkDevice, fcts, VK_OBJECT_TYPE_COMMAND_BUFFER, vkCommandBuffer, name);
    return vkOwnedCommandBuffer;
}

}  // namespace

VulkanCommandBuffer::VulkanCommandBuffer(const ICommandQueue& queue, const IDevice& device, VkCommandPool vkCommandPool,
                                         std::string_view name)
  : m_queue(queue)
  , m_device(device)
  , m_logger(m_device.createLogger(name))
  , m_name(name)
  , m_vkCommandBuffer(createVkCommandBuffer(m_device.getVkDevice(), m_device.getFcts(), vkCommandPool,
                                            fmt::format("Im3eCommandBuffer.{}", name)))
  // Create the fence signalled so that we consider the command as complete until we actually submit work to the queue
  , m_vkFence(createVkFence(m_device, VK_FENCE_CREATE_SIGNALED_BIT))
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
    return std::make_unique<VulkanCommandBarrierRecorder>(name, shared_from_this(), m_device.getFcts());
}

auto VulkanCommandBuffer::createFuture() -> std::shared_ptr<ICommandBufferFuture>
{
    auto future = std::make_shared<VulkanCommandBufferFuture>(shared_from_this());
    m_futures.emplace_back(future);
    return future;
}

void VulkanCommandBuffer::reset()
{
    throwIfVkFailed(m_device.getFcts().vkResetCommandBuffer(m_vkCommandBuffer.get(), 0U),
                    "Failed to reset command buffer");

    m_inFlight = false;
    std::ranges::for_each(m_futures, [](auto& future) { future->markAsComplete(); });

    // Note: we do not reset the fence here is that it remains marked as complete until we actually submit something
    // new to the queue. Otherwise, we risk waiting for the fence while nothing is executing.

    m_vkSignalSemaphore.reset();
    m_vkWaitSemaphore.reset();
    m_futures.clear();
}

void VulkanCommandBuffer::beginRecording(std::string_view)
{
    VkCommandBufferBeginInfo vkBeginInfo{
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
    };
    throwIfVkFailed(m_device.getFcts().vkBeginCommandBuffer(m_vkCommandBuffer.get(), &vkBeginInfo),
                    "Could not begin command buffer recording");
}

void VulkanCommandBuffer::endRecording()
{
    throwIfVkFailed(m_device.getFcts().vkEndCommandBuffer(m_vkCommandBuffer.get()),
                    "Failed to end command buffer recording");
}

void VulkanCommandBuffer::submitToQueue(CommandExecutionType executionType)
{
    // Reset the fence right before submitting to queue so that the fence is always set between the moment an execution
    // is complete and the moment a new command is submitted. Otherwise, we risk waiting for a fence that will never
    // be set.
    const auto vkFence = m_vkFence.get();
    throwIfVkFailed(m_device.getFcts().vkResetFences(m_device.getVkDevice(), 1U, &vkFence),
                    "Failed to reset command buffer fence");

    const auto vkCommandBuffer = m_vkCommandBuffer.get();
    const auto vkSignalSemaphore = m_vkSignalSemaphore.get();
    VkSubmitInfo vkSubmitInfo{
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .commandBufferCount = 1U,
        .pCommandBuffers = &vkCommandBuffer,
        .signalSemaphoreCount = vkSignalSemaphore ? 1U : 0U,
        .pSignalSemaphores = vkSignalSemaphore ? &vkSignalSemaphore : nullptr,
    };
    const VkPipelineStageFlags vkWaitDstMask = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
    const auto vkWaitSemaphore = m_vkWaitSemaphore.get();
    if (vkWaitSemaphore)
    {
        vkSubmitInfo.waitSemaphoreCount = 1U;
        vkSubmitInfo.pWaitSemaphores = &vkWaitSemaphore;
        vkSubmitInfo.pWaitDstStageMask = &vkWaitDstMask;
    }
    throwIfVkFailed(m_device.getFcts().vkQueueSubmit(m_queue.getVkQueue(), 1U, &vkSubmitInfo, vkFence),
                    "Failed to execute command buffer");
    m_inFlight = true;

    if (executionType == CommandExecutionType::Sync)
    {
        waitForCompletion();
    }
}

void VulkanCommandBuffer::waitForCompletion()
{
    if (!m_inFlight)
    {
        return;
    }

    const auto vkFence = m_vkFence.get();
    logIfVkFailed(m_device.getFcts().vkWaitForFences(m_device.getVkDevice(), 1U, &vkFence, VK_TRUE,
                                                     std::numeric_limits<uint64_t>::max()),
                  *m_logger, "Failed to wait for fence while destroying command buffer");

    m_inFlight = false;
    std::ranges::for_each(m_futures, [](auto& future) { future->markAsComplete(); });

    reset();
}

auto VulkanCommandBuffer::isExecutionComplete() const -> bool
{
    if (!m_inFlight)
    {
        return false;
    }

    const auto vkFence = m_vkFence.get();
    const auto vkResult = m_device.getFcts().vkWaitForFences(m_device.getVkDevice(), 1U, &vkFence, VK_TRUE, 0U);
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
