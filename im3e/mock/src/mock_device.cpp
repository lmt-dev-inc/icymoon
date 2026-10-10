#include "mock_device.h"

#include <im3e/test_utils/test_utils.h>

using namespace im3e;

namespace {

class MockProxyDevice : public IDevice
{
public:
    MockProxyDevice(MockDevice& rMock)
      : m_rMock(rMock)
    {
    }

    auto createVkSemaphore() const -> VkUniquePtr<VkSemaphore> override { return m_rMock.createVkSemaphore(); }
    auto createVkFence(VkFenceCreateFlags vkFlags = 0U) const -> VkUniquePtr<VkFence> override
    {
        return m_rMock.createVkFence(vkFlags);
    }
    void waitForVkFence(VkFence vkFence) const override { m_rMock.waitForVkFence(vkFence); }

    auto createLogger(std::string_view name) const -> std::unique_ptr<ILogger> override
    {
        return m_rMock.createLogger(name);
    }

    auto getStatsProvider() -> std::shared_ptr<IStatsProvider> override { return m_rMock.getStatsProvider(); }

    auto getVkInstance() const -> VkInstance override { return m_rMock.getVkInstance(); }
    auto getVkPhysicalDevice() const -> VkPhysicalDevice override { return m_rMock.getVkPhysicalDevice(); }
    auto getVkDevice() const -> VkDevice override { return m_rMock.getVkDevice(); }
    auto getFcts() const -> const VulkanDeviceFcts& override { return m_rMock.getFcts(); }
    auto getInstanceFcts() const -> const VulkanInstanceFcts& override { return m_rMock.getInstanceFcts(); }
    auto getImageFactory() const -> std::shared_ptr<const IImageFactory> override { return m_rMock.getImageFactory(); }
    auto getCommandQueue() const -> std::shared_ptr<const ICommandQueue> override { return m_rMock.getCommandQueue(); }
    auto getCommandQueue() -> std::shared_ptr<ICommandQueue> override { return m_rMock.getCommandQueue(); }

private:
    MockDevice& m_rMock;
};

}  // namespace

MockDevice::MockDevice()
{
    ON_CALL(*this, createVkSemaphore()).WillByDefault(Invoke([this] {
        return VkUniquePtr<VkSemaphore>(m_vkSemaphore, [](auto*) {});
    }));
    ON_CALL(*this, createVkFence(_)).WillByDefault(InvokeWithoutArgs([this] {
        return VkUniquePtr<VkFence>(m_vkFence, [](auto*) {});
    }));

    ON_CALL(*this, createLogger(_)).WillByDefault(InvokeWithoutArgs([this] { return m_mockLogger.createMockProxy(); }));

    ON_CALL(*this, getStatsProvider()).WillByDefault(Invoke([this] { return m_mockStatsProvider.createMockProxy(); }));

    ON_CALL(*this, getVkInstance()).WillByDefault(Return(m_vkInstance));
    ON_CALL(*this, getVkPhysicalDevice()).WillByDefault(Return(m_vkPhysicalDevice));
    ON_CALL(*this, getVkDevice()).WillByDefault(Return(m_vkDevice));
    ON_CALL(*this, getFcts()).WillByDefault(ReturnRef(m_mockFcts.getDeviceFcts()));
    ON_CALL(*this, getImageFactory()).WillByDefault(Invoke([this] { return m_mockImageFactory.createMockProxy(); }));
    ON_CALL(*this, getCommandQueue()).WillByDefault(Invoke([this] { return m_mockCommandQueue.createMockProxy(); }));
}

MockDevice::~MockDevice() = default;

auto MockDevice::createMockProxy() -> std::unique_ptr<IDevice>
{
    return std::make_unique<MockProxyDevice>(*this);
}