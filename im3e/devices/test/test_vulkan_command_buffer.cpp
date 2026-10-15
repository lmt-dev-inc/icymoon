#include "src/vulkan_command_buffer.h"

#include <im3e/mock/mock_command_buffer.h>
#include <im3e/mock/mock_device.h>
#include <im3e/test_utils/test_utils.h>

using namespace im3e;

struct VulkanCommandBufferTest : public Test
{
    auto createCommandBuffer()
    {
        ON_CALL(m_rMockFcts, vkAllocateCommandBuffers(m_mockVkDevice, NotNull(), NotNull()))
            .WillByDefault(Invoke([this](Unused, Unused, auto* pVkCommandBuffer) {
                *pVkCommandBuffer = m_mockVkCommandBuffer;
                return VK_SUCCESS;
            }));
        ON_CALL(m_mockDevice, createVkFence(_)).WillByDefault(InvokeWithoutArgs([this] {
            return VkUniquePtr<VkFence>(m_mockVkFence, [](auto*) {});
        }));
        return std::make_shared<VulkanCommandBuffer>(m_mockQueue, m_mockDevice, m_mockVkPool, "test_buffer");
    }

    NiceMock<MockDevice> m_mockDevice;
    MockVulkanDeviceFcts& m_rMockFcts = m_mockDevice.getMockDeviceFcts();
    NiceMock<MockCommandQueue> m_mockQueue;
    VkDevice m_mockVkDevice = m_mockDevice.getMockVkDevice();
    VkCommandPool m_mockVkPool = reinterpret_cast<VkCommandPool>(0xf7ea9eb);
    VkCommandBuffer m_mockVkCommandBuffer = reinterpret_cast<VkCommandBuffer>(0x637e2a4b);
    VkFence m_mockVkFence = reinterpret_cast<VkFence>(0xbe46ec2);
};

TEST_F(VulkanCommandBufferTest, constructor)
{
    const auto mockVkBuffer = reinterpret_cast<VkCommandBuffer>(0xef4ca23);

    EXPECT_CALL(m_rMockFcts, vkAllocateCommandBuffers(m_mockVkDevice, NotNull(), NotNull()))
        .WillOnce(Invoke([&](Unused, auto* pVkCreateInfo, auto* pVkCommandBuffer) {
            EXPECT_THAT(pVkCreateInfo->sType, Eq(VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO));
            EXPECT_THAT(pVkCreateInfo->pNext, IsNull());
            EXPECT_THAT(pVkCreateInfo->commandPool, Eq(m_mockVkPool));
            EXPECT_THAT(pVkCreateInfo->level, Eq(VK_COMMAND_BUFFER_LEVEL_PRIMARY));
            EXPECT_THAT(pVkCreateInfo->commandBufferCount, Eq(1U));
            *pVkCommandBuffer = mockVkBuffer;
            return VK_SUCCESS;
        }));
    EXPECT_CALL(m_rMockFcts, vkSetDebugUtilsObjectNameEXT(m_mockVkDevice, NotNull()))
        .WillOnce(Invoke([&](Unused, const auto* pDebugInfo) {
            EXPECT_THAT(pDebugInfo->sType, Eq(VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT));
            EXPECT_THAT(pDebugInfo->objectType, Eq(VK_OBJECT_TYPE_COMMAND_BUFFER));
            EXPECT_THAT(pDebugInfo->objectHandle, Eq(reinterpret_cast<uint64_t>(mockVkBuffer)));
            EXPECT_THAT(pDebugInfo->pObjectName, StrEq("Im3eCommandBuffer.buffer"));
            return VK_SUCCESS;
        }));
    EXPECT_CALL(m_mockDevice, createVkFence(VK_FENCE_CREATE_SIGNALED_BIT));
    VulkanCommandBuffer buffer(m_mockQueue, m_mockDevice, m_mockVkPool, "buffer");

    EXPECT_THAT(buffer.getVkCommandBuffer(), Eq(mockVkBuffer));
}

TEST_F(VulkanCommandBufferTest, setVkSignalSemaphore)
{
    VkSharedPtr<VkSemaphore> mockVkSemaphore(reinterpret_cast<VkSemaphore>(0x421b8ca9d), [](auto*) {});

    auto pCommandBuffer = createCommandBuffer();
    pCommandBuffer->setVkSignalSemaphore(mockVkSemaphore);

    EXPECT_CALL(m_rMockFcts, vkQueueSubmit(m_mockQueue.getMockVkQueue(), 1U, NotNull(), m_mockVkFence))
        .WillOnce(Invoke([&](Unused, Unused, auto* pVkSubmitInfo, Unused) {
            EXPECT_THAT(pVkSubmitInfo->signalSemaphoreCount, Eq(1U));
            EXPECT_THAT(*pVkSubmitInfo->pSignalSemaphores, Eq(mockVkSemaphore.get()));
            return VK_SUCCESS;
        }));
    pCommandBuffer->submitToQueue(CommandExecutionType::Async);

    pCommandBuffer->reset();  // a call to reset should remove the signal semaphore

    EXPECT_CALL(m_rMockFcts, vkQueueSubmit(m_mockQueue.getMockVkQueue(), 1U, NotNull(), m_mockVkFence))
        .WillOnce(Invoke([&](Unused, Unused, auto* pVkSubmitInfo, Unused) {
            EXPECT_THAT(pVkSubmitInfo->signalSemaphoreCount, Eq(0U));
            EXPECT_THAT(pVkSubmitInfo->pSignalSemaphores, IsNull());  // semaphore no longer passed
            return VK_SUCCESS;
        }));
    pCommandBuffer->submitToQueue(CommandExecutionType::Async);
}

TEST_F(VulkanCommandBufferTest, setVkWaitSemaphore)
{
    VkSharedPtr<VkSemaphore> mockVkSemaphore(reinterpret_cast<VkSemaphore>(0x421b8ca9d), [](auto*) {});

    auto pCommandBuffer = createCommandBuffer();
    pCommandBuffer->setVkWaitSemaphore(mockVkSemaphore);

    EXPECT_CALL(m_rMockFcts, vkQueueSubmit(m_mockQueue.getMockVkQueue(), 1U, NotNull(), m_mockVkFence))
        .WillOnce(Invoke([&](Unused, Unused, auto* pVkSubmitInfo, Unused) {
            EXPECT_THAT(pVkSubmitInfo->waitSemaphoreCount, Eq(1U));
            EXPECT_THAT(*pVkSubmitInfo->pWaitSemaphores, Eq(mockVkSemaphore.get()));
            EXPECT_THAT(*pVkSubmitInfo->pWaitDstStageMask, Eq(VK_PIPELINE_STAGE_ALL_COMMANDS_BIT));
            return VK_SUCCESS;
        }));
    pCommandBuffer->submitToQueue(CommandExecutionType::Async);

    pCommandBuffer->reset();  // a call to reset should remove the wait semaphore

    EXPECT_CALL(m_rMockFcts, vkQueueSubmit(m_mockQueue.getMockVkQueue(), 1U, NotNull(), m_mockVkFence))
        .WillOnce(Invoke([&](Unused, Unused, auto* pVkSubmitInfo, Unused) {
            EXPECT_THAT(pVkSubmitInfo->waitSemaphoreCount, Eq(0U));
            EXPECT_THAT(pVkSubmitInfo->pWaitSemaphores, IsNull());  // semaphore no longer passed
            return VK_SUCCESS;
        }));
    pCommandBuffer->submitToQueue(CommandExecutionType::Async);
}

TEST_F(VulkanCommandBufferTest, beginRecording)
{
    auto pCommandBuffer = createCommandBuffer();

    EXPECT_CALL(m_rMockFcts, vkBeginCommandBuffer(m_mockVkCommandBuffer, NotNull()))
        .WillOnce(Invoke([&](Unused, auto* pVkBeginInfo) {
            EXPECT_THAT(pVkBeginInfo->sType, Eq(VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO));
            EXPECT_THAT(pVkBeginInfo->flags, Eq(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT));
            return VK_SUCCESS;
        }));
    pCommandBuffer->beginRecording("testCommand");
}

TEST_F(VulkanCommandBufferTest, endRecording)
{
    auto pCommandBuffer = createCommandBuffer();

    EXPECT_CALL(m_rMockFcts, vkEndCommandBuffer(m_mockVkCommandBuffer)).WillOnce(Return(VK_SUCCESS));
    pCommandBuffer->endRecording();
}

TEST_F(VulkanCommandBufferTest, submitToQueueAsync)
{
    auto pCommandBuffer = createCommandBuffer();

    EXPECT_CALL(m_rMockFcts, vkQueueSubmit(m_mockQueue.getMockVkQueue(), 1U, NotNull(), m_mockVkFence))
        .WillOnce(Invoke([&](Unused, Unused, auto* pVkSubmitInfo, Unused) {
            EXPECT_THAT(pVkSubmitInfo->sType, Eq(VK_STRUCTURE_TYPE_SUBMIT_INFO));
            EXPECT_THAT(pVkSubmitInfo->commandBufferCount, Eq(1U));
            EXPECT_THAT(*pVkSubmitInfo->pCommandBuffers, Eq(m_mockVkCommandBuffer));
            EXPECT_THAT(pVkSubmitInfo->signalSemaphoreCount, Eq(0U));
            EXPECT_THAT(pVkSubmitInfo->waitSemaphoreCount, Eq(0U));
            return VK_SUCCESS;
        }));
    pCommandBuffer->submitToQueue(CommandExecutionType::Async);
}

TEST_F(VulkanCommandBufferTest, submitToQueueSync)
{
    auto pCommandBuffer = createCommandBuffer();
    {
        InSequence s;
        EXPECT_CALL(m_rMockFcts, vkQueueSubmit(m_mockQueue.getMockVkQueue(), 1U, NotNull(), m_mockVkFence))
            .WillOnce(Invoke([&](Unused, Unused, auto* pVkSubmitInfo, Unused) {
                EXPECT_THAT(pVkSubmitInfo->sType, Eq(VK_STRUCTURE_TYPE_SUBMIT_INFO));
                EXPECT_THAT(pVkSubmitInfo->commandBufferCount, Eq(1U));
                EXPECT_THAT(*pVkSubmitInfo->pCommandBuffers, Eq(m_mockVkCommandBuffer));
                EXPECT_THAT(pVkSubmitInfo->signalSemaphoreCount, Eq(0U));
                EXPECT_THAT(pVkSubmitInfo->waitSemaphoreCount, Eq(0U));
                return VK_SUCCESS;
            }));
        EXPECT_CALL(m_rMockFcts, vkWaitForFences(m_mockVkDevice, 1U, Pointee(m_mockVkFence), VK_TRUE,
                                                 std::numeric_limits<uint64_t>::max()));
    }
    pCommandBuffer->submitToQueue(CommandExecutionType::Sync);
}

TEST_F(VulkanCommandBufferTest, futureWaitsForCompletion)
{
    auto pCommandBuffer = createCommandBuffer();
    pCommandBuffer->submitToQueue(CommandExecutionType::Async);

    auto pFuture = pCommandBuffer->createFuture();

    EXPECT_CALL(m_rMockFcts, vkWaitForFences(m_mockVkDevice, 1U, Pointee(m_mockVkFence), VK_TRUE,
                                             std::numeric_limits<uint64_t>::max()));
    pFuture->waitForCompletion();

    // vkWaitForFences will be called when the command buffer is destroyed.
    // We verify before the command buffer is destroyed to ensure that vkWaitForFences is indeed called during
    // waitForCompletion().
    Mock::VerifyAndClearExpectations(&m_rMockFcts);
}

TEST_F(VulkanCommandBufferTest, futureDoesNotWaitIfCommandComplete)
{
    auto pCommandBuffer = createCommandBuffer();
    pCommandBuffer->submitToQueue(CommandExecutionType::Sync);

    auto pFuture = pCommandBuffer->createFuture();

    EXPECT_CALL(m_rMockFcts, vkWaitForFences(_, _, _, _, _)).Times(0);
    pFuture->waitForCompletion();
}

TEST_F(VulkanCommandBufferTest, futureDoesNotCrashIfCommandAlreadyDestroyed)
{
    auto pCommandBuffer = createCommandBuffer();
    auto pFuture = pCommandBuffer->createFuture();

    pCommandBuffer.reset();

    EXPECT_NO_THROW(pFuture->waitForCompletion());
}