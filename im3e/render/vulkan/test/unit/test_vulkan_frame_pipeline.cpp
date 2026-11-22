#include "vulkan_frame_pipeline.h"

#include <im3e/render/vulkan/test/mock_vulkan_render_context.h>
#include <im3e/test_utils/test_utils.h>

#include <memory>
#include <stdexcept>

using namespace im3e;

namespace {

struct VulkanFramePipelineTest : public Test
{
    auto createFramePipeline() { return createVulkanFramePipeline(m_mockContext.createMockProxy()); }

    NiceMock<MockVulkanRenderContext> m_mockContext;
};

}  // namespace

TEST_F(VulkanFramePipelineTest, constructor_withoutContext)
{
    EXPECT_THROW(createVulkanFramePipeline(nullptr), std::invalid_argument);
}

TEST_F(VulkanFramePipelineTest, constructor)
{
    EXPECT_CALL(m_mockContext, getDevice());
    EXPECT_CALL(m_mockContext.getMockDevice(), createLogger(StrEq("VulkanFramePipeline")));

    auto framePipeline = createFramePipeline();
    ASSERT_THAT(framePipeline, NotNull());
    EXPECT_THAT(framePipeline->getDevice(), Eq(m_mockContext.getFakeDevice()));
}
