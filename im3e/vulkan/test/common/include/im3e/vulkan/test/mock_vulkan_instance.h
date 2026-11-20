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

private:
    ::testing::NiceMock<MockVulkanFunctions> m_mockFcts;
    ::testing::NiceMock<MockVulkanExtensions> m_mockExtensions;
};

}  // namespace im3e