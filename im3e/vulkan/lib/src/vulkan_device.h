#pragma once

#include "vulkan_command_queue.h"
#include "vulkan_instance.h"
#include "vulkan_loader.h"
#include "vulkan_memory_allocator.h"

#include <im3e/utils/loggers.h>
#include <im3e/vulkan/device.h>
#include <im3e/vulkan/image.h>

namespace im3e {

class VulkanDevice : public IDevice, public std::enable_shared_from_this<VulkanDevice>
{
public:
    VulkanDevice(const ILogger& rLogger, DeviceConfig config);
    ~VulkanDevice() override;

    auto createVkSemaphore() const -> VkUniquePtr<VkSemaphore> override;
    auto createVkFence(VkFenceCreateFlags vkFlags) const -> VkUniquePtr<VkFence> override;
    void waitForVkFence(VkFence vkFence) const override;

    auto createLogger(std::string_view name) const -> std::unique_ptr<ILogger> override;

    auto getStatsProvider() -> std::shared_ptr<IStatsProvider> override { return m_pStatsProvider; }

    auto getVkInstance() const -> VkInstance override { return m_instance.getVkInstance(); }
    auto getVkPhysicalDevice() const -> VkPhysicalDevice override { return m_physicalDevice.vkPhysicalDevice; }
    auto getVkDevice() const -> VkDevice override { return m_pVkDevice.get(); }
    auto getFcts() const -> const VulkanDeviceFcts& override { return m_fcts; }
    auto getInstanceFcts() const -> const VulkanInstanceFcts& override { return m_instance.getFcts(); }
    auto getImageFactory() const -> std::shared_ptr<const IImageFactory> override;
    auto getCommandQueue() const -> std::shared_ptr<const ICommandQueue> override { return m_pCommandQueue; }
    auto getCommandQueue() -> std::shared_ptr<ICommandQueue> override { return m_pCommandQueue; }

private:
    std::unique_ptr<ILogger> m_pLogger;
    std::shared_ptr<IStatsProvider> m_pStatsProvider;
    const DeviceConfig m_config;

    const VulkanInstance m_instance;
    const VulkanPhysicalDevice m_physicalDevice;
    VulkanDeviceFcts m_fcts;
    VkUniquePtr<VkDevice> m_pVkDevice;
    const VulkanCommandQueueInfo m_commandQueueInfo;

    std::shared_ptr<IVulkanMemoryAllocator> m_pMemoryAllocator;
    mutable std::shared_ptr<IImageFactory> m_pImageFactory;
    std::shared_ptr<ICommandQueue> m_pCommandQueue;
};

}  // namespace im3e