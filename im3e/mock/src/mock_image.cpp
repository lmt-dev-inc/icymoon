#include "mock_image.h"

using namespace im3e;

namespace {

class MockProxyImageView : public IImageView
{
public:
    MockProxyImageView(MockImageView& rMock)
      : m_rMock(rMock)
    {
    }

    auto getVkImageView() const -> VkImageView override { return m_rMock.getVkImageView(); }
    auto getVkImage() const -> VkImage override { return m_rMock.getVkImage(); }

private:
    MockImageView& m_rMock;
};

}  // namespace

MockImageView::MockImageView() = default;
MockImageView::~MockImageView() = default;

auto MockImageView::createMockProxy() -> std::unique_ptr<IImageView>
{
    return std::make_unique<MockProxyImageView>(*this);
}

namespace {

class MockProxyImageMetadata : public IImageMetadata
{
public:
    MockProxyImageMetadata(MockImageMetadata& rMock)
      : m_rMock(rMock)
    {
    }

    void setLayout(VkImageLayout vkLayout) override { m_rMock.setLayout(vkLayout); }
    void setLastStageMask(VkPipelineStageFlags2 vkStageMask) override { m_rMock.setLastStageMask(vkStageMask); }
    void setLastAccessMask(VkAccessFlags2 vkAccessMask) override { m_rMock.setLastAccessMask(vkAccessMask); }

    auto getLayout() const -> VkImageLayout override { return m_rMock.getLayout(); }
    auto getQueueFamilyIndex() const -> uint32_t override { return m_rMock.getQueueFamilyIndex(); }
    auto getLastStageMask() const -> VkPipelineStageFlags2 override { return m_rMock.getLastStageMask(); }
    auto getLastAccessMask() const -> VkAccessFlags2 override { return m_rMock.getLastAccessMask(); }

private:
    MockImageMetadata& m_rMock;
};

}  // namespace

MockImageMetadata::MockImageMetadata() = default;
MockImageMetadata::~MockImageMetadata() = default;

auto MockImageMetadata::createMockProxy() -> std::unique_ptr<IImageMetadata>
{
    return std::make_unique<MockProxyImageMetadata>(*this);
}

namespace {

class MockProxyImage : public IImage
{
public:
    MockProxyImage(MockImage& rMock)
      : m_rMock(rMock)
    {
    }

    auto createView() const -> std::unique_ptr<IImageView> override { return m_rMock.createView(); }

    auto getVkImage() const -> VkImage override { return m_rMock.getVkImage(); }
    auto getVkExtent() const -> VkExtent2D override { return m_rMock.getVkExtent(); }
    auto getVkFormat() const -> VkFormat override { return m_rMock.getVkFormat(); }
    auto getVkSubresourceLayers() const -> VkImageSubresourceLayers override
    {
        return m_rMock.getVkSubresourceLayers();
    }
    auto getMetadata() -> std::shared_ptr<IImageMetadata> override { return m_rMock.getMetadata(); }
    auto getMetadata() const -> std::shared_ptr<const IImageMetadata> override { return m_rMock.getMetadata(); }

private:
    MockImage& m_rMock;
};

}  // namespace

MockImage::MockImage()
{
    ON_CALL(*this, getVkSubresourceLayers())
        .WillByDefault(Return(VkImageSubresourceLayers{
            .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
            .layerCount = 1U,
        }));
    ON_CALL(*this, createView()).WillByDefault(Invoke([this] { return m_mockView.createMockProxy(); }));
    ON_CALL(*this, getMetadata()).WillByDefault(Invoke([this] { return m_mockMetadata.createMockProxy(); }));
    ON_CALL(Const(*this), getMetadata()).WillByDefault(Invoke([this] { return m_mockMetadata.createMockProxy(); }));
}

MockImage::~MockImage() = default;

auto MockImage::createMockProxy() -> std::unique_ptr<IImage>
{
    return std::make_unique<MockProxyImage>(*this);
}

namespace {

class MockProxyImageFactory : public IImageFactory
{
public:
    MockProxyImageFactory(MockImageFactory& rMock)
      : m_rMock(rMock)
    {
    }

    auto createImage(ImageConfig config) const -> std::unique_ptr<IImage> override
    {
        return m_rMock.createImage(std::move(config));
    }

    auto createHostVisibleImage(ImageConfig config) const -> std::unique_ptr<IHostVisibleImage> override
    {
        return m_rMock.createHostVisibleImage(std::move(config));
    }

    auto createProxyImage(VkImage vkImage, ImageConfig config) const -> std::unique_ptr<IImage> override
    {
        return m_rMock.createProxyImage(vkImage, std::move(config));
    }

private:
    MockImageFactory& m_rMock;
};

}  // namespace

MockImageFactory::MockImageFactory() = default;
MockImageFactory::~MockImageFactory() = default;

auto MockImageFactory::createMockProxy() -> std::unique_ptr<IImageFactory>
{
    return std::make_unique<MockProxyImageFactory>(*this);
}
