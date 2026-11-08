#pragma once

#include <im3e/test_utils/test_utils.h>
#include <im3e/vulkan/image.h>

namespace im3e {

class MockImageView : public IImageView
{
public:
    MockImageView();
    ~MockImageView() override;

    MOCK_METHOD(VkImageView, getVkImageView, (), (const, override));
    MOCK_METHOD(VkImage, getVkImage, (), (const, override));

    auto createMockProxy() -> std::unique_ptr<IImageView>;
};

class MockImageMetadata : public IImageMetadata
{
public:
    MockImageMetadata();
    ~MockImageMetadata() override;

    MOCK_METHOD(void, setLayout, (VkImageLayout vkLayout), (override));
    MOCK_METHOD(void, setLastStageMask, (VkPipelineStageFlags2 vkStageMask), (override));
    MOCK_METHOD(void, setLastAccessMask, (VkAccessFlags2 vkAccessMask), (override));

    MOCK_METHOD(VkImageLayout, getLayout, (), (const, override));
    MOCK_METHOD(uint32_t, getQueueFamilyIndex, (), (const, override));
    MOCK_METHOD(VkPipelineStageFlags2, getLastStageMask, (), (const, override));
    MOCK_METHOD(VkAccessFlags2, getLastAccessMask, (), (const, override));

    auto createMockProxy() -> std::unique_ptr<IImageMetadata>;
};

class MockImage : public IImage
{
public:
    MockImage();
    ~MockImage() override;

    MOCK_METHOD(std::unique_ptr<IImageView>, createView, (), (const, override));

    MOCK_METHOD(VkImage, getVkImage, (), (const, override));
    MOCK_METHOD(VkExtent2D, getVkExtent, (), (const, override));
    MOCK_METHOD(VkFormat, getVkFormat, (), (const, override));
    MOCK_METHOD(VkImageSubresourceLayers, getVkSubresourceLayers, (), (const, override));
    MOCK_METHOD(std::shared_ptr<IImageMetadata>, getMetadata, (), (override));
    MOCK_METHOD(std::shared_ptr<const IImageMetadata>, getMetadata, (), (const, override));

    auto createMockProxy() -> std::unique_ptr<IImage>;

    auto getMockImageView() -> MockImageView& { return m_mockView; }
    auto getMockMetadata() -> MockImageMetadata& { return m_mockMetadata; }

private:
    NiceMock<MockImageView> m_mockView;
    NiceMock<MockImageMetadata> m_mockMetadata;
};

class MockImageFactory : public IImageFactory
{
public:
    MockImageFactory();
    ~MockImageFactory() override;

    MOCK_METHOD(std::unique_ptr<IImage>, createImage, (ImageConfig config), (const, override));
    MOCK_METHOD(std::unique_ptr<IHostVisibleImage>, createHostVisibleImage, (ImageConfig config), (const, override));
    MOCK_METHOD(std::unique_ptr<IImage>, createProxyImage, (VkImage vkImage, ImageConfig config), (const, override));

    auto createMockProxy() -> std::unique_ptr<IImageFactory>;
};

}  // namespace im3e