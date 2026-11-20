#include "vulkan_device.h"

#include <im3e/test_utils/test_utils.h>
#include <im3e/utils/mock/mock_logger.h>
#include <im3e/utils/mock/mock_stats.h>
#include <im3e/vulkan/test/mock_vulkan_instance.h>

#include <memory>

using namespace im3e;

namespace {

struct VulkanDeviceTest : public Test
{
    auto createDevice()
    {
        return std::make_shared<VulkanDevice>(m_mockLogger, DeviceConfig{}, m_mockStats,
                                              m_mockInstance.createMockProxy());
    }

    NiceMock<MockLogger> m_mockLogger;
    std::shared_ptr<MockStatsProvider> m_mockStats = std::make_shared<NiceMock<MockStatsProvider>>();
    NiceMock<MockVulkanInstance> m_mockInstance;
};

}  // namespace

TEST_F(VulkanDeviceTest, constructor)
{
    auto device = createDevice();
}