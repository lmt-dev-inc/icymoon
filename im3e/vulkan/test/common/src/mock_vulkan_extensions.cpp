#include <im3e/vulkan/test/mock_vulkan_extensions.h>

#include <im3e/test_utils/test_utils.h>

using namespace im3e;

MockVulkanExtensions::MockVulkanExtensions()
{
    ON_CALL(*this, getInstanceExtensions()).WillByDefault(ReturnRef(m_instanceExtensions));
    ON_CALL(*this, getDeviceExtensions()).WillByDefault(ReturnRef(m_deviceExtensions));
    ON_CALL(*this, getLayers()).WillByDefault(ReturnRef(m_layers));
}

MockVulkanExtensions::~MockVulkanExtensions() = default;
