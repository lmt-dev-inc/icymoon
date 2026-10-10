#include "mock_command_buffer.h"

using namespace im3e;

namespace {

class MockProxyCommandBufferFuture : public ICommandBufferFuture
{
public:
    MockProxyCommandBufferFuture(MockCommandBufferFuture& rMock)
      : m_rMock(rMock)
    {
    }

    void waitForCompletion() override { m_rMock.waitForCompletion(); }

private:
    MockCommandBufferFuture& m_rMock;
};

}  // namespace

MockCommandBufferFuture::MockCommandBufferFuture() = default;
MockCommandBufferFuture::~MockCommandBufferFuture() = default;

auto MockCommandBufferFuture::createMockProxy() -> std::unique_ptr<ICommandBufferFuture>
{
    return std::make_unique<MockProxyCommandBufferFuture>(*this);
}

namespace {

class MockProxyCommandBarrierRecorder : public ICommandBarrierRecorder
{
public:
    MockProxyCommandBarrierRecorder(MockCommandBarrierRecorder& rMock)
      : m_rMock(rMock)
    {
    }

    void addImageBarrier(IImage& rImage, ImageBarrierConfig config) override
    {
        return m_rMock.addImageBarrier(rImage, std::move(config));
    }

private:
    MockCommandBarrierRecorder& m_rMock;
};

}  // namespace

MockCommandBarrierRecorder::MockCommandBarrierRecorder() = default;
MockCommandBarrierRecorder::~MockCommandBarrierRecorder() = default;

auto MockCommandBarrierRecorder::createMockProxy() -> std::unique_ptr<ICommandBarrierRecorder>
{
    return std::make_unique<MockProxyCommandBarrierRecorder>(*this);
}

namespace {

class MockProxyCommandBuffer : public ICommandBuffer
{
public:
    MockProxyCommandBuffer(MockCommandBuffer& rMock)
      : m_rMock(rMock)
    {
    }

    auto startScopedBarrier(std::string_view name) const -> std::unique_ptr<ICommandBarrierRecorder> override
    {
        return m_rMock.startScopedBarrier(name);
    }
    auto createFuture() -> std::shared_ptr<ICommandBufferFuture> override { return m_rMock.createFuture(); }

    void setVkSignalSemaphore(VkSharedPtr<VkSemaphore> vkSemaphore) override
    {
        m_rMock.setVkSignalSemaphore(vkSemaphore);
    }
    void setVkWaitSemaphore(VkSharedPtr<VkSemaphore> vkSemaphore) override { m_rMock.setVkWaitSemaphore(vkSemaphore); }

    auto getVkCommandBuffer() const -> VkCommandBuffer override { return m_rMock.getVkCommandBuffer(); }

private:
    MockCommandBuffer& m_rMock;
};

}  // namespace

MockCommandBuffer::MockCommandBuffer()
{
    ON_CALL(*this, startScopedBarrier(_)).WillByDefault(Invoke([this](Unused) {
        return m_mockBarrierRecorder.createMockProxy();
    }));
    ON_CALL(*this, createFuture()).WillByDefault(Invoke([this] { return m_mockFuture.createMockProxy(); }));
    ON_CALL(*this, getVkCommandBuffer()).WillByDefault(Return(m_vkCommandBuffer));
}

MockCommandBuffer::~MockCommandBuffer() = default;

auto MockCommandBuffer::createMockProxy() -> std::unique_ptr<ICommandBuffer>
{
    return std::make_unique<MockProxyCommandBuffer>(*this);
}

namespace {

class MockProxyCommandQueue : public ICommandQueue
{
public:
    MockProxyCommandQueue(MockCommandQueue& rMock)
      : m_rMock(rMock)
    {
    }

    auto startScopedCommand(std::string_view name, CommandExecutionType executionType)
        -> UniquePtrWithDeleter<ICommandBuffer> override
    {
        return m_rMock.startScopedCommand(name, executionType);
    }

    void waitIdle() override { m_rMock.waitIdle(); }

    auto getQueueFamilyIndex() const -> uint32_t override { return m_rMock.getQueueFamilyIndex(); }
    auto getVkQueue() const -> VkQueue override { return m_rMock.getVkQueue(); }

private:
    MockCommandQueue& m_rMock;
};

}  // namespace

MockCommandQueue::MockCommandQueue()
{
    ON_CALL(*this, startScopedCommand(_, _)).WillByDefault(Invoke([this](Unused, Unused) {
        return m_mockCommandBuffer.createMockProxy();
    }));
    ON_CALL(*this, getVkQueue()).WillByDefault(Return(m_vkQueue));
}

MockCommandQueue::~MockCommandQueue() = default;

auto MockCommandQueue::createMockProxy() -> std::unique_ptr<ICommandQueue>
{
    return std::make_unique<MockProxyCommandQueue>(*this);
}