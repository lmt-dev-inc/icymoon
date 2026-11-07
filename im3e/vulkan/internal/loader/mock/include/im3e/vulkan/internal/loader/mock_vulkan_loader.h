#pragma once

#include <im3e/vulkan/internal/loader/vulkan_loaders.h>
#include <im3e/mock/mock_vulkan_functions.h>

namespace im3e {

class MockVulkanLoader : public IVulkanLoader
{
public:
    MockVulkanLoader();
    ~MockVulkanLoader() override;

    MOCK_METHOD(VulkanGlobalFcts, loadGlobalFcts, (), (const, override));
    MOCK_METHOD(VulkanInstanceFcts, loadInstanceFcts, (VkInstance vkInstance), (const, override));
    MOCK_METHOD(VulkanDeviceFcts, loadDeviceFcts, (VkDevice vkDevice), (const, override));
    MOCK_METHOD(VmaVulkanFunctions, loadVmaFcts, (VkInstance vkInstance, VkDevice vkDevice), (const, override));

    auto createMockProxy() -> std::unique_ptr<IVulkanLoader>;

    auto getGlobalFcts() const -> const VulkanGlobalFcts& { return m_mockFcts.getGlobalFcts(); }
    auto getInstanceFcts() const -> const VulkanInstanceFcts& { return m_mockFcts.getInstanceFcts(); }
    auto getDeviceFcts() const -> const VulkanDeviceFcts& { return m_mockFcts.getDeviceFcts(); }
    auto getVmaFcts() const -> const VmaVulkanFunctions& { return m_mockFcts.getVmaFcts(); }

    auto getMockGlobalFcts() -> MockVulkanGlobalFcts& { return m_mockFcts.getMockGlobalFcts(); }
    auto getMockInstanceFcts() -> MockVulkanInstanceFcts& { return m_mockFcts.getMockInstanceFcts(); }
    auto getMockDeviceFcts() -> MockVulkanDeviceFcts& { return m_mockFcts.getMockDeviceFcts(); }
    auto getMockVmaFcts() -> MockVmaVulkanFunctions& { return m_mockFcts.getMockVmaFcts(); }

private:
    ::testing::NiceMock<MockVulkanFunctions> m_mockFcts;
};

}  // namespace im3e