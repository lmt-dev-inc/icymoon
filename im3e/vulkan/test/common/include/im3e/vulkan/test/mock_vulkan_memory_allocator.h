#pragma once

#include "vulkan_memory_allocator.h"

#include <im3e/test_utils/test_utils.h>

namespace im3e {

class MockVulkanMemoryAllocator : public IVulkanMemoryAllocator
{
public:
    MockVulkanMemoryAllocator();
    ~MockVulkanMemoryAllocator() override;

    MOCK_METHOD(VkResult, createImage,
                (const VkImageCreateInfo* pVkCreateInfo, const VmaAllocationCreateInfo* pVmaCreateInfo,
                 VkImage* pVkImage, VmaAllocation* pVmaAllocation, VmaAllocationInfo* pVmaAllocationInfo),
                (override));
    MOCK_METHOD(void, destroyImage, (VkImage vkImage, VmaAllocation vmaAllocation), (override));

    MOCK_METHOD(VkResult, mapMemory, (VmaAllocation vmaAllocation, void** ppData), (override));
    MOCK_METHOD(void, unmapMemory, (VmaAllocation vmaAllocation), (override));

    auto createMockProxy() -> std::unique_ptr<IVulkanMemoryAllocator>;
};

}  // namespace im3e