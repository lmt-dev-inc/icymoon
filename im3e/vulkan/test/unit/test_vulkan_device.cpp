#include "vulkan_device.h"

#include <im3e/test_utils/test_utils.h>
#include <im3e/utils/mock/mock_logger.h>
#include <im3e/utils/mock/mock_stats.h>
#include <im3e/vulkan/test/mock_vulkan_instance.h>
#include <im3e/vulkan/test/mock_vulkan_memory_allocator.h>

#include <memory>

using namespace im3e;

namespace {

struct VulkanDeviceTest : public Test
{
    VulkanDeviceTest()
    {
        ON_CALL(m_mockFcts.getMockDeviceFcts(), vkGetDeviceQueue(_, _, _, NotNull()))
            .WillByDefault(SetArgPointee<3>(m_fakeVkQueue));
        ON_CALL(m_mockFcts.getMockInstanceFcts(), vkCreateDevice(_, _, _, NotNull()))
            .WillByDefault(DoAll(SetArgPointee<3>(m_fakeVkDevice), Return(VK_SUCCESS)));
    }

    auto createDevice()
    {
        return std::make_shared<VulkanDevice>(
            m_mockLogger, DeviceConfig{}, m_mockStats, m_mockInstance.createMockProxy(),
            [this](const IDevice&, VmaVulkanFunctions) { return m_mockAllocator.createMockProxy(); });
    }

    NiceMock<MockLogger> m_mockLogger;
    std::shared_ptr<MockStatsProvider> m_mockStats = std::make_shared<NiceMock<MockStatsProvider>>();
    NiceMock<MockVulkanInstance> m_mockInstance;
    MockVulkanFunctions& m_mockFcts = m_mockInstance.getMockFunctions();
    NiceMock<MockVulkanMemoryAllocator> m_mockAllocator;

    VkQueue m_fakeVkQueue = reinterpret_cast<VkQueue>(0xc0ffee);
    VkDevice m_fakeVkDevice = reinterpret_cast<VkDevice>(0x3e4fd5);
};

}  // namespace

TEST_F(VulkanDeviceTest, constructor)
{
    auto device = createDevice();
}