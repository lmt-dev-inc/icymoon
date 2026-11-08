#pragma once

#include <im3e/api/command_buffer.h>
#include <im3e/api/device.h>

namespace im3e {

struct VulkanCommandQueueInfo
{
    VkQueue vkQueue{};
    uint32_t queueFamilyIndex = ~0U;
};
auto createVulkanCommandQueue(const IDevice& rDevice, VulkanCommandQueueInfo queueInfo, std::string_view name)
    -> std::shared_ptr<ICommandQueue>;

}  // namespace im3e