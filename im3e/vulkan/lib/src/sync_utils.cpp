#include <im3e/vulkan/sync_utils.h>

#include <cstdint>
#include <limits>

using namespace im3e;

auto im3e::createVkSemaphore(const IDevice& device) -> VkUniquePtr<VkSemaphore>
{
    const auto vkDevice = device.getVkDevice();
    const auto& fcts = device.getFcts();

    const VkSemaphoreCreateInfo vkCreateInfo{
        .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
    };

    VkSemaphore vkSemaphore{};
    throwIfVkFailed(fcts.vkCreateSemaphore(vkDevice, &vkCreateInfo, nullptr, &vkSemaphore),
                    "Failed to create semaphore");
    return makeVkUniquePtr(vkDevice, vkSemaphore, fcts.vkDestroySemaphore);
}

auto im3e::createVkFence(const IDevice& device, VkFenceCreateFlags vkFlags) -> VkUniquePtr<VkFence>
{
    const auto vkDevice = device.getVkDevice();
    const auto& fcts = device.getFcts();

    const VkFenceCreateInfo vkCreateInfo{
        .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
        .flags = vkFlags,
    };

    VkFence vkFence{};
    throwIfVkFailed(fcts.vkCreateFence(vkDevice, &vkCreateInfo, nullptr, &vkFence), "Failed to create fence");
    return makeVkUniquePtr(vkDevice, vkFence, fcts.vkDestroyFence);
}

void im3e::waitForVkFence(const IDevice& device, VkFence vkFence)
{
    if (!vkFence)
    {
        return;
    }
    throwIfVkFailed(device.getFcts().vkWaitForFences(device.getVkDevice(), 1U, &vkFence, VK_TRUE,
                                                     std::numeric_limits<uint64_t>::max()),
                    "Failed to wait for fence");
}
