#pragma once

#include "vulkan_extensions.h"

#include <gmock/gmock.h>

namespace im3e {

class MockVulkanExtensions : public IVulkanExtensions
{
public:
    MockVulkanExtensions();
    ~MockVulkanExtensions() override;

    MOCK_METHOD((const std::vector<const char*>&), getInstanceExtensions, (), (const, override));
    MOCK_METHOD((const std::vector<const char*>&), getDeviceExtensions, (), (const, override));
    MOCK_METHOD((const std::vector<const char*>&), getLayers, (), (const, override));

    MOCK_METHOD(bool, areDebugUtilsEnabled, (), (const, override));

private:
    std::vector<const char*> m_instanceExtensions;
    std::vector<const char*> m_deviceExtensions;
    std::vector<const char*> m_layers;
};

}  // namespace im3e