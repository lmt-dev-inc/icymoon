#include "vulkan_extensions.h"

#include <im3e/test_utils/test_utils.h>
#include <im3e/utils/mock/mock_logger.h>
#include <im3e/vulkan/test/mock_vulkan_helper.h>
#include <im3e/vulkan/test/mock_vulkan_loader.h>

using namespace im3e;

struct VulkanExtensionsTest : public Test
{
    NiceMock<MockLogger> m_mockLogger;
    NiceMock<MockVulkanLoader> m_mockVk;
    VulkanGlobalFcts m_globalFcts = m_mockVk.loadGlobalFcts();
};

TEST_F(VulkanExtensionsTest, constructor)
{
    constexpr bool DebubDisabled = false;
    expectInstanceExtensionsEnumerated(m_mockVk.getMockGlobalFcts(), DebubDisabled);

    VulkanExtensions extensions(m_mockLogger, m_globalFcts, DebubDisabled);
    EXPECT_THAT(extensions.getInstanceExtensions(), IsSupersetOf(std::vector<std::string>{
                                                        VK_KHR_SURFACE_EXTENSION_NAME,
                                                    }));
    EXPECT_THAT(extensions.getDeviceExtensions(),
                ContainerEq(std::vector<const char*>{VK_KHR_SWAPCHAIN_EXTENSION_NAME}));
    EXPECT_THAT(extensions.getLayers(), ContainerEq(std::vector<const char*>{}));
}