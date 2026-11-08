#pragma once

#include <im3e/api/device.h>

#include <memory>

namespace im3e {

class IVulkanMemoryAllocator
{
public:
    virtual ~IVulkanMemoryAllocator() = default;

    virtual auto createImage(const VkImageCreateInfo* pVkCreateInfo, const VmaAllocationCreateInfo* pVmaCreateInfo,
                             VkImage* pVkImage, VmaAllocation* pVmaAllocation, VmaAllocationInfo* pVmaAllocationInfo)
        -> VkResult = 0;
    virtual void destroyImage(VkImage vkImage, VmaAllocation vmaAllocation) = 0;

    virtual auto mapMemory(VmaAllocation vmaAllocation, void** ppData) -> VkResult = 0;
    virtual void unmapMemory(VmaAllocation vmaAllocation) = 0;
};

auto createVulkanMemoryAllocator(const IDevice& rDevice, VmaVulkanFunctions vmaFcts)
    -> std::unique_ptr<IVulkanMemoryAllocator>;

}  // namespace im3e