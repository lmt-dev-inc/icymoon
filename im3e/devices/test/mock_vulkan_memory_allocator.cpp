#include "mock_vulkan_memory_allocator.h"

using namespace im3e;

namespace {

class MockProxyMemoryAllocator : public IVulkanMemoryAllocator
{
public:
    MockProxyMemoryAllocator(MockVulkanMemoryAllocator& rMock)
      : m_rMock(rMock)
    {
    }

    auto createImage(const VkImageCreateInfo* pVkCreateInfo, const VmaAllocationCreateInfo* pVmaCreateInfo,
                     VkImage* pVkImage, VmaAllocation* pVmaAllocation, VmaAllocationInfo* pVmaAllocationInfo)
        -> VkResult override
    {
        return m_rMock.createImage(pVkCreateInfo, pVmaCreateInfo, pVkImage, pVmaAllocation, pVmaAllocationInfo);
    }

    void destroyImage(VkImage vkImage, VmaAllocation vmaAllocation) override
    {
        m_rMock.destroyImage(vkImage, vmaAllocation);
    }

    auto mapMemory(VmaAllocation vmaAllocation, void** ppData) -> VkResult override
    {
        return m_rMock.mapMemory(vmaAllocation, ppData);
    }
    void unmapMemory(VmaAllocation vmaAllocation) override { m_rMock.unmapMemory(vmaAllocation); }

private:
    MockVulkanMemoryAllocator& m_rMock;
};

}  // namespace

MockVulkanMemoryAllocator::MockVulkanMemoryAllocator() = default;
MockVulkanMemoryAllocator::~MockVulkanMemoryAllocator() = default;

auto MockVulkanMemoryAllocator::createMockProxy() -> std::unique_ptr<IVulkanMemoryAllocator>
{
    return std::make_unique<MockProxyMemoryAllocator>(*this);
}