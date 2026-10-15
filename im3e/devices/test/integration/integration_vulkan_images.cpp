#include <im3e/devices/devices.h>
#include <im3e/test_utils/device_integration_test.h>
#include <im3e/test_utils/test_utils.h>
#include <im3e/test_utils/vk.h>
#include <im3e/utils/loggers.h>

using namespace im3e;

struct VulkanImageIntegration : public DeviceIntegrationTest
{
    std::shared_ptr<const IImageFactory> m_pImageFactory = getDevice()->getImageFactory();
};

TEST_F(VulkanImageIntegration, createImageRgbaSnorm)
{
    constexpr VkExtent2D ExpectedExtent{.width = 256U, .height = 120U};
    constexpr VkFormat ExpectedFormat = VK_FORMAT_R8G8B8A8_SNORM;

    auto pImage = m_pImageFactory->createImage(ImageConfig{
        .name = "testImage",
        .vkExtent = ExpectedExtent,
        .vkFormat = ExpectedFormat,
        .vkUsage = VK_IMAGE_USAGE_STORAGE_BIT,
    });
    ASSERT_THAT(pImage, NotNull());
    EXPECT_THAT(pImage->getVkImage(), Ne(VK_NULL_HANDLE));
    EXPECT_THAT(pImage->getVkExtent(), Eq(ExpectedExtent));
    EXPECT_THAT(pImage->getVkFormat(), Eq(ExpectedFormat));
}

TEST_F(VulkanImageIntegration, createImageRgbaSrgb)
{
    constexpr VkExtent2D ExpectedExtent{.width = 120U, .height = 64U};
    constexpr VkFormat ExpectedFormat = VK_FORMAT_R8G8B8A8_SRGB;

    auto pImage = m_pImageFactory->createImage(ImageConfig{
        .name = "testImage",
        .vkExtent = ExpectedExtent,
        .vkFormat = ExpectedFormat,
        .vkUsage = VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
    });
    ASSERT_THAT(pImage, NotNull());
    EXPECT_THAT(pImage->getVkImage(), Ne(VK_NULL_HANDLE));
    EXPECT_THAT(pImage->getVkExtent(), Eq(ExpectedExtent));
    EXPECT_THAT(pImage->getVkFormat(), Eq(ExpectedFormat));
}

TEST_F(VulkanImageIntegration, createHostVisibleImageRgbaUnorm)
{
    constexpr VkExtent2D ExpectedExtent{.width = 24U, .height = 10U};
    constexpr VkFormat ExpectedFormat = VK_FORMAT_R8G8B8A8_UNORM;

    auto pImage = m_pImageFactory->createHostVisibleImage(ImageConfig{
        .name = "testHostVisibleImage",
        .vkExtent = ExpectedExtent,
        .vkFormat = ExpectedFormat,
        .vkUsage = VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
    });
    ASSERT_THAT(pImage, NotNull());
    EXPECT_THAT(pImage->getVkImage(), Ne(VK_NULL_HANDLE));
    EXPECT_THAT(pImage->getVkExtent(), Eq(ExpectedExtent));
    EXPECT_THAT(pImage->getVkFormat(), Eq(ExpectedFormat));

    auto pImageMapping = pImage->map();
    EXPECT_THAT(pImageMapping->getData(), NotNull());
    EXPECT_THAT(pImageMapping->getConstData(), NotNull());
    EXPECT_THAT(pImageMapping->getRowPitch(), Eq(ExpectedExtent.width * sizeof(uint32_t)));
    EXPECT_THAT(pImageMapping->getSizeInBytes(), Eq(ExpectedExtent.width * ExpectedExtent.height * sizeof(uint32_t)));
}