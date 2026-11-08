#pragma once

#include <im3e/utils/mock/mock_logger.h>
#include <im3e/utils/mock/mock_stats.h>
#include <im3e/vulkan/device.h>
#include <im3e/vulkan/mock_command_buffer.h>
#include <im3e/vulkan/mock_image.h>
#include <im3e/vulkan/mock_vulkan_functions.h>

#include <gmock/gmock.h>

#include <memory>

namespace im3e {

class MockDevice : public IDevice
{
public:
    MockDevice();
    ~MockDevice() override;

    MOCK_METHOD(VkUniquePtr<VkSemaphore>, createVkSemaphore, (), (const, override));
    MOCK_METHOD(VkUniquePtr<VkFence>, createVkFence, (VkFenceCreateFlags vkFlags), (const, override));
    MOCK_METHOD(void, waitForVkFence, (VkFence vkFence), (const, override));

    MOCK_METHOD(std::unique_ptr<ILogger>, createLogger, (std::string_view name), (const, override));

    MOCK_METHOD(std::shared_ptr<IStatsProvider>, getStatsProvider, (), (override));

    MOCK_METHOD(VkInstance, getVkInstance, (), (const, override));
    MOCK_METHOD(VkPhysicalDevice, getVkPhysicalDevice, (), (const, override));
    MOCK_METHOD(VkDevice, getVkDevice, (), (const, override));
    MOCK_METHOD(const VulkanDeviceFcts&, getFcts, (), (const, override));
    MOCK_METHOD(const VulkanInstanceFcts&, getInstanceFcts, (), (const, override));
    MOCK_METHOD(std::shared_ptr<const IImageFactory>, getImageFactory, (), (const, override));
    MOCK_METHOD(std::shared_ptr<const ICommandQueue>, getCommandQueue, (), (const, override));
    MOCK_METHOD(std::shared_ptr<ICommandQueue>, getCommandQueue, (), (override));

    auto createMockProxy() -> std::unique_ptr<IDevice>;

    auto getMockLogger() -> MockLogger& { return m_mockLogger; }
    auto getMockStatsProvider() -> MockStatsProvider& { return m_mockStatsProvider; }
    auto getMockVkInstance() const -> VkInstance { return m_vkInstance; }
    auto getMockVkPhysicalDevice() const -> VkPhysicalDevice { return m_vkPhysicalDevice; }
    auto getMockVkDevice() const -> VkDevice { return m_vkDevice; }
    auto getMockDeviceFcts() -> MockVulkanDeviceFcts& { return m_mockFcts.getMockDeviceFcts(); }
    auto getMockImageFactory() -> MockImageFactory& { return m_mockImageFactory; }
    auto getMockCommandQueue() -> MockCommandQueue& { return m_mockCommandQueue; }

private:
    const VkInstance m_vkInstance = reinterpret_cast<VkInstance>(0x35e2ca18b3e);
    const VkPhysicalDevice m_vkPhysicalDevice = reinterpret_cast<VkPhysicalDevice>(0xef45a3c4);
    const VkDevice m_vkDevice = reinterpret_cast<VkDevice>(0xbaef532f3e4a);
    const VkSemaphore m_vkSemaphore = reinterpret_cast<VkSemaphore>(0x51e8fe9a);
    const VkFence m_vkFence = reinterpret_cast<VkFence>(0xf52e6a);

    ::testing::NiceMock<MockLogger> m_mockLogger;
    ::testing::NiceMock<MockStatsProvider> m_mockStatsProvider;
    ::testing::NiceMock<MockVulkanFunctions> m_mockFcts;
    ::testing::NiceMock<MockImageFactory> m_mockImageFactory;
    ::testing::NiceMock<MockCommandQueue> m_mockCommandQueue;
};

}  // namespace im3e