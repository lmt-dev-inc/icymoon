#include "vulkan_memory_allocator.h"

#include "vulkan_instance.h"

using namespace im3e;

namespace {

auto createVmaAllocator(VkInstance vkInstance, VkPhysicalDevice vkPhysicalDevice, VkDevice vkDevice,
                        const VmaVulkanFunctions& rVmaVkFcts)
{
    VmaAllocatorCreateInfo vmaCreateInfo{
        .flags = VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT | VMA_ALLOCATOR_CREATE_KHR_BIND_MEMORY2_BIT,
        .physicalDevice = vkPhysicalDevice,
        .device = vkDevice,
        .pVulkanFunctions = &rVmaVkFcts,
        .instance = vkInstance,
        .vulkanApiVersion = VulkanInstance::getVulkanApiVersion(),
    };

    VmaAllocator vmaAllocator{};
    throwIfVkFailed(vmaCreateAllocator(&vmaCreateInfo, &vmaAllocator), "Could not create VMA allocator");

    return VkUniquePtr<VmaAllocator>(vmaAllocator, [](auto* pVmaAllocator) { vmaDestroyAllocator(pVmaAllocator); });
}

class VulkanMemoryAllocator : public IVulkanMemoryAllocator
{
public:
    VulkanMemoryAllocator(const IDevice& rDevice, VmaVulkanFunctions vmaFcts)
      : m_rDevice(rDevice)
      , m_vmaFcts(std::move(vmaFcts))
      , m_pVmaAllocator(createVmaAllocator(m_rDevice.getVkInstance(), m_rDevice.getVkPhysicalDevice(),
                                           m_rDevice.getVkDevice(), vmaFcts))
    {
    }

    auto createImage(const VkImageCreateInfo* pVkCreateInfo, const VmaAllocationCreateInfo* pVmaCreateInfo,
                     VkImage* pVkImage, VmaAllocation* pVmaAllocation, VmaAllocationInfo* pVmaAllocationInfo)
        -> VkResult override
    {
        return vmaCreateImage(m_pVmaAllocator.get(), pVkCreateInfo, pVmaCreateInfo, pVkImage, pVmaAllocation,
                              pVmaAllocationInfo);
    }

    void destroyImage(VkImage vkImage, VmaAllocation vmaAllocation) override
    {
        vmaDestroyImage(m_pVmaAllocator.get(), vkImage, vmaAllocation);
    }

    auto mapMemory(VmaAllocation vmaAllocation, void** ppData) -> VkResult override
    {
        const auto vkResult = vmaMapMemory(m_pVmaAllocator.get(), vmaAllocation, ppData);
        vmaInvalidateAllocation(m_pVmaAllocator.get(), vmaAllocation, 0U, VK_WHOLE_SIZE);
        return vkResult;
    }

    void unmapMemory(VmaAllocation vmaAllocation) override
    {
        vmaFlushAllocation(m_pVmaAllocator.get(), vmaAllocation, 0U, VK_WHOLE_SIZE);
        vmaUnmapMemory(m_pVmaAllocator.get(), vmaAllocation);
    }

private:
    const IDevice& m_rDevice;
    VmaVulkanFunctions m_vmaFcts;
    VkUniquePtr<VmaAllocator> m_pVmaAllocator;
};

}  // namespace

auto im3e::createVulkanMemoryAllocator(const IDevice& rDevice, VmaVulkanFunctions vmaFcts)
    -> std::unique_ptr<IVulkanMemoryAllocator>
{
    return std::make_unique<VulkanMemoryAllocator>(rDevice, std::move(vmaFcts));
}