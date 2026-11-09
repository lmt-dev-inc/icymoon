#pragma once

#include <im3e/test_utils/test_utils.h>
#include <im3e/vulkan/command_buffer.h>

namespace im3e {

class MockCommandBufferFuture : public ICommandBufferFuture
{
public:
    MockCommandBufferFuture();
    ~MockCommandBufferFuture() override;

    MOCK_METHOD(void, waitForCompletion, (), (override));

    auto createMockProxy() -> std::unique_ptr<ICommandBufferFuture>;
};

class MockCommandBarrierRecorder : public ICommandBarrierRecorder
{
public:
    MockCommandBarrierRecorder();
    ~MockCommandBarrierRecorder() override;

    MOCK_METHOD(void, addImageBarrier, (IImage & rImage, ImageBarrierConfig config), (override));

    auto createMockProxy() -> std::unique_ptr<ICommandBarrierRecorder>;
};

class MockCommandBuffer : public ICommandBuffer
{
public:
    MockCommandBuffer();
    ~MockCommandBuffer() override;

    MOCK_METHOD(std::unique_ptr<ICommandBarrierRecorder>, startScopedBarrier, (std::string_view name),
                (const, override));
    MOCK_METHOD(std::shared_ptr<ICommandBufferFuture>, createFuture, (), (override));

    MOCK_METHOD(void, setVkSignalSemaphore, (VkSharedPtr<VkSemaphore> vkSemaphore), (override));
    MOCK_METHOD(void, setVkWaitSemaphore, (VkSharedPtr<VkSemaphore> vkSemaphore), (override));

    MOCK_METHOD(VkCommandBuffer, getVkCommandBuffer, (), (const, override));

    auto createMockProxy() -> std::unique_ptr<ICommandBuffer>;

    auto getMockVkCommandBuffer() const -> VkCommandBuffer { return m_vkCommandBuffer; }
    auto getMockFuture() -> MockCommandBufferFuture& { return m_mockFuture; }
    auto getMockBarrierRecorder() -> MockCommandBarrierRecorder& { return m_mockBarrierRecorder; }

private:
    VkCommandBuffer m_vkCommandBuffer = reinterpret_cast<VkCommandBuffer>(0x4f3eda3eb29);
    NiceMock<MockCommandBufferFuture> m_mockFuture;
    NiceMock<MockCommandBarrierRecorder> m_mockBarrierRecorder;
};

class MockCommandQueue : public ICommandQueue
{
public:
    MockCommandQueue();
    ~MockCommandQueue() override;

    MOCK_METHOD(UniquePtrWithDeleter<ICommandBuffer>, startScopedCommand,
                (std::string_view name, CommandExecutionType executionType), (override));

    MOCK_METHOD(void, waitIdle, (), (override));

    MOCK_METHOD(uint32_t, getQueueFamilyIndex, (), (const, override));
    MOCK_METHOD(VkQueue, getVkQueue, (), (const, override));

    auto createMockProxy() -> std::unique_ptr<ICommandQueue>;

    auto getMockVkQueue() const -> VkQueue { return m_vkQueue; }
    auto getMockCommandBuffer() -> MockCommandBuffer& { return m_mockCommandBuffer; }

private:
    const VkQueue m_vkQueue = reinterpret_cast<VkQueue>(0xfe34a2d3e);

    NiceMock<MockCommandBuffer> m_mockCommandBuffer;
};

}  // namespace im3e