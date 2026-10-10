#include "mock_vulkan_loader.h"

using namespace im3e;

namespace {

class MockProxyVulkanLoader : public IVulkanLoader
{
public:
    MockProxyVulkanLoader(MockVulkanLoader& rMock)
      : m_rMock(rMock)
    {
    }

    auto loadGlobalFcts() const -> VulkanGlobalFcts override { return m_rMock.loadGlobalFcts(); }
    auto loadInstanceFcts(VkInstance vkInstance) const -> VulkanInstanceFcts override
    {
        return m_rMock.loadInstanceFcts(vkInstance);
    }
    auto loadDeviceFcts(VkDevice vkDevice) const -> VulkanDeviceFcts override
    {
        return m_rMock.loadDeviceFcts(vkDevice);
    }
    auto loadVmaFcts(VkInstance vkInstance, VkDevice vkDevice) const -> VmaVulkanFunctions override
    {
        return m_rMock.loadVmaFcts(vkInstance, vkDevice);
    }

private:
    MockVulkanLoader& m_rMock;
};

}  // namespace

MockVulkanLoader::MockVulkanLoader()
{
    ON_CALL(*this, loadGlobalFcts()).WillByDefault(Invoke([this] { return this->getGlobalFcts(); }));
    ON_CALL(*this, loadInstanceFcts(_)).WillByDefault(InvokeWithoutArgs([this] { return this->getInstanceFcts(); }));
    ON_CALL(*this, loadDeviceFcts(_)).WillByDefault(InvokeWithoutArgs([this] { return this->getDeviceFcts(); }));
    ON_CALL(*this, loadVmaFcts(_, _)).WillByDefault(InvokeWithoutArgs([this] { return this->getVmaFcts(); }));
}

MockVulkanLoader::~MockVulkanLoader() = default;

auto MockVulkanLoader::createMockProxy() -> std::unique_ptr<IVulkanLoader>
{
    return std::make_unique<MockProxyVulkanLoader>(*this);
}