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

    void verifyVkPhysicalDeviceFeatures(const VkPhysicalDeviceFeatures& vkFeatures)
    {
        EXPECT_THAT(vkFeatures.samplerAnisotropy, Eq(VK_TRUE));
        EXPECT_THAT(vkFeatures.shaderSampledImageArrayDynamicIndexing, Eq(VK_TRUE));
        EXPECT_THAT(vkFeatures.shaderInt64, Eq(VK_TRUE));
        EXPECT_THAT(vkFeatures.shaderInt16, Eq(VK_TRUE));
    }

    void verifyVkPhysicalDevice11Features(const VkPhysicalDeviceVulkan11Features& vk11Features)
    {
        EXPECT_THAT(vk11Features.sType, Eq(VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES));
        EXPECT_THAT(vk11Features.storageBuffer16BitAccess, Eq(VK_TRUE));
    }

    void verifyVkPhysicalDevice12Features(const VkPhysicalDeviceVulkan12Features& vk12Features)
    {
        EXPECT_THAT(vk12Features.sType, Eq(VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES));
        EXPECT_THAT(vk12Features.shaderFloat16, Eq(VK_TRUE));
        EXPECT_THAT(vk12Features.shaderInt8, Eq(VK_TRUE));
        EXPECT_THAT(vk12Features.descriptorBindingPartiallyBound, Eq(VK_TRUE));
        EXPECT_THAT(vk12Features.descriptorBindingVariableDescriptorCount, Eq(VK_TRUE));
        EXPECT_THAT(vk12Features.runtimeDescriptorArray, Eq(VK_TRUE));
        EXPECT_THAT(vk12Features.scalarBlockLayout, Eq(VK_TRUE));
        EXPECT_THAT(vk12Features.bufferDeviceAddress, Eq(VK_TRUE));
    }

    void verifyVkPhysicalDevice13Features(const VkPhysicalDeviceVulkan13Features& vk13Features)
    {
        EXPECT_THAT(vk13Features.sType, Eq(VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES));
        EXPECT_THAT(vk13Features.synchronization2, Eq(VK_TRUE));
        EXPECT_THAT(vk13Features.dynamicRendering, Eq(VK_TRUE));
        EXPECT_THAT(vk13Features.maintenance4, Eq(VK_TRUE));
    }

    void verifyVkDeviceCreateInfo(const VkDeviceCreateInfo* vkCreateInfo)
    {
        ASSERT_THAT(vkCreateInfo, NotNull());
        EXPECT_THAT(vkCreateInfo->sType, Eq(VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO));

        ASSERT_THAT(vkCreateInfo->pEnabledFeatures, NotNull());
        verifyVkPhysicalDeviceFeatures(*vkCreateInfo->pEnabledFeatures);

        ASSERT_THAT(vkCreateInfo->pNext, NotNull());
        auto& vk11Features = *reinterpret_cast<const VkPhysicalDeviceVulkan11Features*>(vkCreateInfo->pNext);
        verifyVkPhysicalDevice11Features(vk11Features);

        ASSERT_THAT(vk11Features.pNext, NotNull());
        auto& vk12Features = *reinterpret_cast<const VkPhysicalDeviceVulkan12Features*>(vk11Features.pNext);
        verifyVkPhysicalDevice12Features(vk12Features);

        ASSERT_THAT(vk12Features.pNext, NotNull());
        auto& vk13Features = *reinterpret_cast<const VkPhysicalDeviceVulkan13Features*>(vk12Features.pNext);
        verifyVkPhysicalDevice13Features(vk13Features);
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
    const auto vkPhysicalDevice = m_mockInstance.getFakePhysicalDevice().vkPhysicalDevice;
    EXPECT_CALL(m_mockFcts.getMockInstanceFcts(), vkCreateDevice(vkPhysicalDevice, NotNull(), IsNull(), NotNull()))
        .WillOnce(Invoke([this](Unused, auto* vkCreateInfo, Unused, auto* vkDevice) {
            verifyVkDeviceCreateInfo(vkCreateInfo);
            *vkDevice = m_fakeVkDevice;
            return VK_SUCCESS;
        }));
    auto device = createDevice();
    EXPECT_THAT(device->getStatsProvider(), Eq(m_mockStats));
    EXPECT_THAT(device->getVkInstance(), Eq(m_mockInstance.getFakeVkInstance()));
    EXPECT_THAT(device->getVkPhysicalDevice(), Eq(vkPhysicalDevice));
    EXPECT_THAT(device->getVkDevice(), Eq(m_fakeVkDevice));
    EXPECT_THAT(device->getImageFactory(), NotNull());
    EXPECT_THAT(device->getCommandQueue(), NotNull());
}

TEST_F(VulkanDeviceTest, isValidationEnabled)
{
    auto device = createDevice();

    EXPECT_CALL(m_mockInstance.getMockExtensions(), isValidationEnabled()).WillOnce(Return(true));
    EXPECT_THAT(device->isValidationEnabled(), IsTrue());

    EXPECT_CALL(m_mockInstance.getMockExtensions(), isValidationEnabled()).WillOnce(Return(false));
    EXPECT_THAT(device->isValidationEnabled(), IsFalse());
}