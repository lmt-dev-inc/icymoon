#include "vulkan_render_context.h"

#include <im3e/test_utils/test_utils.h>
#include <im3e/vulkan/mock_device.h>

#include <memory>
#include <stdexcept>
#include <utility>

using namespace im3e;

namespace {

struct VulkanRenderContextTest : public Test
{
    auto createContext() { return createVulkanRenderContext(m_device); }

    NiceMock<MockDevice> m_mockDevice;
    std::shared_ptr<IDevice> m_device{m_mockDevice.createMockProxy()};
};

}  // namespace

TEST_F(VulkanRenderContextTest, constructor_withoutDevice)
{
    EXPECT_THROW(createVulkanRenderContext(nullptr), std::invalid_argument);
}

TEST_F(VulkanRenderContextTest, constructor)
{
    EXPECT_CALL(m_mockDevice, createLogger(StrEq("VulkanRenderContext")));

    auto context = createContext();
    ASSERT_THAT(context, NotNull());
    EXPECT_THAT(context->getDevice(), Eq(m_device));
    EXPECT_THAT(std::as_const(*context).getDevice(), Eq(m_device));
}
