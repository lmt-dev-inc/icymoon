#pragma once

#include <im3e/vulkan/device.h>
#include <im3e/vulkan/utils.h>

namespace im3e {

/// @brief Create a binary semaphore.
/// @throws std::runtime_error if Vulkan fails to create the semaphore.
auto createVkSemaphore(const IDevice& device) -> VkUniquePtr<VkSemaphore>;

/// @brief Create a fence.
/// @param vkFlags Creation flags, e.g. VK_FENCE_CREATE_SIGNALED_BIT to create the fence signaled.
/// @throws std::runtime_error if Vulkan fails to create the fence.
auto createVkFence(const IDevice& device, VkFenceCreateFlags vkFlags = 0U) -> VkUniquePtr<VkFence>;

/// @brief Wait, without timeout, until a fence is signaled. Returns immediately if the fence is null.
/// @throws std::runtime_error if Vulkan fails to wait for the fence.
void waitForVkFence(const IDevice& device, VkFence vkFence);

}  // namespace im3e
