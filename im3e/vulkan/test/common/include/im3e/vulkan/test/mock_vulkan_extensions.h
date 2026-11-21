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

    void setFakeInstanceExtensions(std::vector<const char*> extensions)
    {
        m_instanceExtensions = std::move(extensions);
    }
    void setFakeDeviceExtensions(std::vector<const char*> extensions) { m_deviceExtensions = std::move(extensions); }
    void setFakeLayers(std::vector<const char*> layers) { m_layers = std::move(layers); }

private:
    std::vector<const char*> m_instanceExtensions;
    std::vector<const char*> m_deviceExtensions;
    std::vector<const char*> m_layers;
};

}  // namespace im3e