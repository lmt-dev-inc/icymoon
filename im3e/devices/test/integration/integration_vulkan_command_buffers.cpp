#include <im3e/devices/devices.h>
#include <im3e/test_utils/device_integration_test.h>
#include <im3e/test_utils/test_utils.h>
#include <im3e/test_utils/vk.h>

using namespace im3e;

struct VulkanCommandBuffersIntegration : public DeviceIntegrationTest
{
    std::shared_ptr<const IImageFactory> m_pImageFactory = getDevice()->getImageFactory();
    std::shared_ptr<ICommandQueue> m_pCommandQueue = getDevice()->getCommandQueue();
};

TEST_F(VulkanCommandBuffersIntegration, clearColorImage)
{
    auto pImage = m_pImageFactory->createHostVisibleImage(ImageConfig{
        .vkExtent{.width = 64U, .height = 64U},
        .vkFormat = VK_FORMAT_R8G8B8A8_UNORM,
        .vkUsage = VK_IMAGE_USAGE_TRANSFER_DST_BIT,
    });
    {
        auto pCommandBuffer = m_pCommandQueue->startScopedCommand("initializeLayout", CommandExecutionType::Sync);
        {
            auto pBarrierRecorder = pCommandBuffer->startScopedBarrier("initImageLayout");
            pBarrierRecorder->addImageBarrier(*pImage, ImageBarrierConfig{
                                                           .vkDstStageMask = VK_PIPELINE_STAGE_2_CLEAR_BIT,
                                                           .vkDstAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT,
                                                           .vkLayout = VK_IMAGE_LAYOUT_GENERAL,
                                                       });
        }
    }
    {
        auto pImageMapping = pImage->map();
        auto* const pImageBegin = reinterpret_cast<uint32_t*>(pImageMapping->getData());
        auto* const pImageEnd = pImageBegin + pImageMapping->getSizeInBytes();
        std::fill(pImageBegin, pImageEnd, 0U);
    }
    {
        auto pCommandBuffer = m_pCommandQueue->startScopedCommand("clearColorImage", CommandExecutionType::Sync);

        const VkClearColorValue vkClearColor{1.0F, 1.0F, 1.0F, 1.0F};
        const VkImageSubresourceRange vkRange{
            .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
            .levelCount = 1U,
            .layerCount = 1U,
        };
        getDevice()->getFcts().vkCmdClearColorImage(pCommandBuffer->getVkCommandBuffer(), pImage->getVkImage(),
                                                    VK_IMAGE_LAYOUT_GENERAL, &vkClearColor, 1U, &vkRange);
    }
    {
        auto pImageMapping = pImage->map();
        auto* const pImageBegin = pImageMapping->getData();
        const auto vkExtent = pImage->getVkExtent();
        const auto rowPitch = pImageMapping->getRowPitch();
        for (uint32_t y = 0U; y < vkExtent.height; y++)
        {
            for (uint32_t x = 0U; x < vkExtent.width; x++)
            {
                const auto pPixel = pImageBegin + y * rowPitch + x * sizeof(uint32_t);
                ASSERT_THAT(*reinterpret_cast<const uint32_t*>(pPixel), Eq(0xFFFFFFFF))
                    << fmt::format("pixel ({};{}) = ({}; {}; {}; {})", x, y, *(pPixel + 0U), *(pPixel + 1U),
                                   *(pPixel + 2U), *(pPixel + 3U));
            }
        }
    }
}
