#include <im3e/vulkan/test/mock_vulkan_instance.h>

#include <im3e/test_utils/test_utils.h>

using namespace im3e;

namespace {

class MockProxyVulkanInstance : public IVulkanInstance
{
public:
    MockProxyVulkanInstance(MockVulkanInstance& mock)
      : m_mock(mock)
    {
    }

    auto loadDeviceFcts(VkDevice vkDevice) const -> VulkanDeviceFcts { return m_mock.loadDeviceFcts(vkDevice); }
    auto loadVmaFcts(VkDevice vkDevice) const -> VmaVulkanFunctions { return m_mock.loadVmaFcts(vkDevice); }
    auto choosePhysicalDevice(const IsPresentationSupportedFct& isPresentationSupportedFct) const
        -> VulkanPhysicalDevice
    {
        return m_mock.choosePhysicalDevice(isPresentationSupportedFct);
    }

    auto getFcts() const -> const VulkanInstanceFcts& { return m_mock.getFcts(); }
    auto getExtensions() const -> const IVulkanExtensions& { return m_mock.getExtensions(); }
    auto getVkInstance() const -> VkInstance { return m_mock.getVkInstance(); }

private:
    MockVulkanInstance& m_mock;
};

}  // namespace

MockVulkanInstance::MockVulkanInstance()
{
    ON_CALL(*this, loadDeviceFcts(_)).WillByDefault(Return(m_mockFcts.getDeviceFcts()));
    ON_CALL(*this, loadVmaFcts(_)).WillByDefault(Return(m_mockFcts.getVmaFcts()));
    ON_CALL(*this, choosePhysicalDevice(_)).WillByDefault(InvokeWithoutArgs([this] { return m_physicalDevice; }));
    ON_CALL(*this, getFcts()).WillByDefault(ReturnRef(m_mockFcts.getInstanceFcts()));
    ON_CALL(*this, getExtensions()).WillByDefault(ReturnRef(m_mockExtensions));
    ON_CALL(*this, getVkInstance()).WillByDefault(Return(m_fakeVkInstance));
}

MockVulkanInstance::~MockVulkanInstance() = default;

auto MockVulkanInstance::createMockProxy() -> std::unique_ptr<IVulkanInstance>
{
    return std::make_unique<MockProxyVulkanInstance>(*this);
}