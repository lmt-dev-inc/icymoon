#include "vulkan_instance.h"

#include <im3e/vulkan/mock_vulkan_functions.h>
#include <im3e/vulkan/test/mock_vulkan_extensions.h>

#include <gmock/gmock.h>

namespace im3e {

class MockVulkanInstance : public IVulkanInstance
{
public:
    MockVulkanInstance();
    ~MockVulkanInstance() override;

    MOCK_METHOD(VulkanDeviceFcts, loadDeviceFcts, (VkDevice vkDevice), (const, override));
    MOCK_METHOD(VmaVulkanFunctions, loadVmaFcts, (VkDevice vkDevice), (const, override));
    MOCK_METHOD(VulkanPhysicalDevice, choosePhysicalDevice,
                (const IsPresentationSupportedFct& isPresentationSupportedFct), (const, override));

    MOCK_METHOD(const VulkanInstanceFcts&, getFcts, (), (const, override));
    MOCK_METHOD(const IVulkanExtensions&, getExtensions, (), (const, override));
    MOCK_METHOD(VkInstance, getVkInstance, (), (const, override));

    auto createMockProxy() -> std::unique_ptr<IVulkanInstance>;
    auto getMockFunctions() -> MockVulkanFunctions& { return m_mockFcts; }
    auto getMockExtensions() -> MockVulkanExtensions& { return m_mockExtensions; }
    auto getFakeVkInstance() -> VkInstance { return m_fakeVkInstance; }
    auto getFakePhysicalDevice() -> VulkanPhysicalDevice& { return m_physicalDevice; }

private:
    ::testing::NiceMock<MockVulkanFunctions> m_mockFcts;
    ::testing::NiceMock<MockVulkanExtensions> m_mockExtensions;

    VkInstance m_fakeVkInstance = reinterpret_cast<VkInstance>(0xdef425a);
    VulkanPhysicalDevice m_physicalDevice{
        .vkPhysicalDevice = reinterpret_cast<VkPhysicalDevice>(0xa4b3c7f),
        .vkDeviceProperties{
            .apiVersion = VK_API_VERSION_1_3,
            .deviceType = VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU,
            .deviceName = "Mock Device",
        },
        .vkDeviceMemoryProperties{},
        .vkDeviceFeatures{},
        .queueFamilies{
            .vkQueueFamilyProperties{
                VkQueueFamilyProperties{
                    .queueFlags = VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT | VK_QUEUE_TRANSFER_BIT,
                    .queueCount = 1U,
                },
            },
            .computeFamilyIndices{0U},
            .graphicsFamilyIndices{0U},
            .transferFamilyIndices{0U},
            .presentationFamilyIndices{0U},
        },
    };
};

}  // namespace im3e