#include "src/vulkan_instance.h"

#include "mock_vulkan_helper.h"

#include <im3e/devices/vulkan_loaders/mock/mock_vulkan_loader.h>
#include <im3e/test_utils/test_utils.h>
#include <im3e/utils/mock/mock_logger.h>

using namespace im3e;

namespace {

constexpr bool DebugIsEnabled = true;
constexpr bool DebugIsDisabled = false;

}  // namespace

struct VulkanInstanceTest : public Test
{
    auto createInstance(bool isDebugEnabled = false)
    {
        expectInstanceExtensionsEnumerated(m_mockVkLoader.getMockGlobalFcts(), isDebugEnabled);
        return VulkanInstance(m_mockLogger, isDebugEnabled, {}, m_mockVkLoader.createMockProxy());
    }

    NiceMock<MockLogger> m_mockLogger;
    NiceMock<MockVulkanLoader> m_mockVkLoader;
};

TEST_F(VulkanInstanceTest, constructor)
{
    auto mockVkInstance = reinterpret_cast<VkInstance>(0xa3ef4d23b85);

    EXPECT_CALL(m_mockVkLoader, loadGlobalFcts());
    EXPECT_CALL(m_mockVkLoader.getMockGlobalFcts(), vkCreateInstance(NotNull(), nullptr, NotNull()))
        .WillOnce(Invoke([&](auto* pCreateInfo, Unused, auto* pVkInstance) {
            EXPECT_THAT(pCreateInfo->sType, Eq(VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO));
            EXPECT_THAT(pCreateInfo->pApplicationInfo, NotNull());

            auto pAppInfo = pCreateInfo->pApplicationInfo;
            EXPECT_THAT(pAppInfo->sType, Eq(VK_STRUCTURE_TYPE_APPLICATION_INFO));
            EXPECT_THAT(pAppInfo->apiVersion, Eq(VK_API_VERSION_1_3));

            *pVkInstance = mockVkInstance;
            return VK_SUCCESS;
        }));
    EXPECT_CALL(m_mockVkLoader, loadInstanceFcts(mockVkInstance));
    auto instance = createInstance(DebugIsDisabled);
}

TEST_F(VulkanInstanceTest, constructorWithDebugEnabled)
{
    auto mockVkInstance = reinterpret_cast<VkInstance>(0xb45ef76a);

    EXPECT_CALL(m_mockVkLoader, loadGlobalFcts());
    EXPECT_CALL(m_mockVkLoader.getMockGlobalFcts(), vkCreateInstance(NotNull(), nullptr, NotNull()))
        .WillOnce(Invoke([&](auto* pCreateInfo, Unused, auto* pVkInstance) {
            EXPECT_THAT(pCreateInfo->sType, Eq(VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO));
            EXPECT_THAT(pCreateInfo->pApplicationInfo, NotNull());
            EXPECT_THAT(pCreateInfo->pNext, NotNull());

            auto* pDebugCreateInfo = reinterpret_cast<const VkDebugUtilsMessengerCreateInfoEXT*>(pCreateInfo->pNext);
            EXPECT_THAT(pDebugCreateInfo->sType, Eq(VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT));

            auto pAppInfo = pCreateInfo->pApplicationInfo;
            EXPECT_THAT(pAppInfo->sType, Eq(VK_STRUCTURE_TYPE_APPLICATION_INFO));
            EXPECT_THAT(pAppInfo->apiVersion, Eq(VK_API_VERSION_1_3));

            *pVkInstance = mockVkInstance;
            return VK_SUCCESS;
        }));
    EXPECT_CALL(m_mockVkLoader, loadInstanceFcts(mockVkInstance));
    auto instance = createInstance(DebugIsEnabled);
}

TEST_F(VulkanInstanceTest, constructorWithDebugEnabledButNotSupportedThrows)
{
    EXPECT_CALL(m_mockVkLoader.getMockGlobalFcts(), vkEnumerateInstanceExtensionProperties(IsNull(), NotNull(), _))
        .WillOnce(Invoke([&](Unused, auto* pCount, Unused) {
            *pCount = 0U;
            return VK_SUCCESS;
        }))
        .WillOnce(Invoke([&](Unused, Unused, Unused) { return VK_SUCCESS; }));
    EXPECT_THROW(VulkanInstance instance(m_mockLogger, DebugIsEnabled, {}, m_mockVkLoader.createMockProxy()),
                 std::runtime_error);
}

TEST_F(VulkanInstanceTest, loadDeviceFcts)
{
    const auto vkDevice = reinterpret_cast<VkDevice>(0xba54fe32);
    auto instance = createInstance();

    EXPECT_CALL(m_mockVkLoader, loadDeviceFcts(vkDevice));
    instance.loadDeviceFcts(vkDevice);
}

TEST_F(VulkanInstanceTest, loadVmaFcts)
{
    const auto vkInstance = reinterpret_cast<VkInstance>(0x456fe32);
    const auto vkDevice = reinterpret_cast<VkDevice>(0x42f5ea5fe);

    EXPECT_CALL(m_mockVkLoader.getMockGlobalFcts(), vkCreateInstance(NotNull(), nullptr, NotNull()))
        .WillOnce(Invoke([&](Unused, Unused, auto* pVkInstance) {
            *pVkInstance = vkInstance;
            return VK_SUCCESS;
        }));
    auto instance = createInstance();

    EXPECT_CALL(m_mockVkLoader, loadVmaFcts(vkInstance, vkDevice));
    instance.loadVmaFcts(vkDevice);
}