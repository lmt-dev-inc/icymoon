#pragma once

#include "vulkan_render_context.h"

#include <im3e/vulkan/mock_device.h>

#include <gmock/gmock.h>

#include <memory>

namespace im3e {

class MockVulkanRenderContext : public IVulkanRenderContext
{
public:
    MockVulkanRenderContext();
    ~MockVulkanRenderContext() override;

    MOCK_METHOD(std::shared_ptr<IDevice>, getDevice, (), (override));
    MOCK_METHOD(std::shared_ptr<const IDevice>, getDevice, (), (const, override));

    auto createMockProxy() -> std::unique_ptr<IVulkanRenderContext>;

    auto getMockDevice() -> MockDevice& { return m_mockDevice; }
    auto getFakeDevice() const -> const std::shared_ptr<IDevice>& { return m_device; }

private:
    ::testing::NiceMock<MockDevice> m_mockDevice;
    // Created once, so that every call to getDevice() returns the same pointer.
    std::shared_ptr<IDevice> m_device{m_mockDevice.createMockProxy()};
};

}  // namespace im3e
