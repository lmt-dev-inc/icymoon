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

    auto extensions = createVulkanExtensions(m_mockLogger, m_globalFcts, DebubDisabled);
    EXPECT_THAT(extensions->getInstanceExtensions(), IsSupersetOf(std::vector<std::string>{
                                                         VK_KHR_SURFACE_EXTENSION_NAME,
                                                     }));
    EXPECT_THAT(extensions->getDeviceExtensions(), ElementsAre(StrEq(VK_KHR_SWAPCHAIN_EXTENSION_NAME)));
    EXPECT_THAT(extensions->getLayers(), ContainerEq(std::vector<const char*>{}));
    EXPECT_THAT(extensions->isValidationEnabled(), IsFalse());
}

TEST_F(VulkanExtensionsTest, isValidationEnabled_whenLayerSupported)
{
    constexpr bool DebugEnabled = true;
    expectInstanceExtensionsEnumerated(m_mockVk.getMockGlobalFcts(), DebugEnabled);
    setSupportedInstanceLayers(m_mockVk.getMockGlobalFcts(), {VkLayerProperties{
                                                                 .layerName = "VK_LAYER_KHRONOS_validation",
                                                                 .specVersion = 1U,
                                                             }});

    auto extensions = createVulkanExtensions(m_mockLogger, m_globalFcts, DebugEnabled);
    EXPECT_THAT(extensions->getLayers(), ElementsAre(StrEq("VK_LAYER_KHRONOS_validation")));
    EXPECT_THAT(extensions->isValidationEnabled(), IsTrue());
}

TEST_F(VulkanExtensionsTest, isValidationEnabled_whenLayerNotSupported)
{
    constexpr bool DebugEnabled = true;
    expectInstanceExtensionsEnumerated(m_mockVk.getMockGlobalFcts(), DebugEnabled);
    setSupportedInstanceLayers(m_mockVk.getMockGlobalFcts(), {});
    EXPECT_CALL(m_mockLogger, warning(_)).Times(AnyNumber());
    EXPECT_CALL(m_mockLogger,
                warning(StrEq("Vulkan validation is disabled: VK_LAYER_KHRONOS_validation was not found")));

    auto extensions = createVulkanExtensions(m_mockLogger, m_globalFcts, DebugEnabled);
    EXPECT_THAT(extensions->getLayers(), IsEmpty());
    EXPECT_THAT(extensions->isValidationEnabled(), IsFalse());
}