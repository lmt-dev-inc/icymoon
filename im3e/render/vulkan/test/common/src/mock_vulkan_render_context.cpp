#include <im3e/render/vulkan/test/mock_vulkan_render_context.h>

#include <im3e/test_utils/test_utils.h>

#include <memory>
#include <utility>

using namespace im3e;

namespace {

class MockProxyVulkanRenderContext : public IVulkanRenderContext
{
public:
    explicit MockProxyVulkanRenderContext(MockVulkanRenderContext& mock)
      : m_mock(mock)
    {
    }

    auto getDevice() -> std::shared_ptr<IDevice> override { return m_mock.getDevice(); }
    auto getDevice() const -> std::shared_ptr<const IDevice> override { return m_mock.getDevice(); }

private:
    MockVulkanRenderContext& m_mock;
};

}  // namespace

MockVulkanRenderContext::MockVulkanRenderContext()
{
    ON_CALL(*this, getDevice()).WillByDefault(Invoke([this] { return m_device; }));
    ON_CALL(Const(*this), getDevice()).WillByDefault(Invoke([this] {
        return std::shared_ptr<const IDevice>(m_device);
    }));
}

MockVulkanRenderContext::~MockVulkanRenderContext() = default;

auto MockVulkanRenderContext::createMockProxy() -> std::unique_ptr<IVulkanRenderContext>
{
    return std::make_unique<MockProxyVulkanRenderContext>(*this);
}
