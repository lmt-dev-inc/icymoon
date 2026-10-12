#pragma once

#include <im3e/api/frame_pipeline.h>

#include <memory>

namespace im3e {

class ClearColorTestPipeline : public IFramePipeline
{
public:
    static constexpr VkClearColorValue DefaultClearColor{.float32 = {0.0177445058F, 0.0656561702F, 0.198943928F, 1.0F}};

    ClearColorTestPipeline(std::shared_ptr<const IDevice> pDevice,
                           const VkClearColorValue& rVkClearColor = DefaultClearColor)
      : m_pDevice(std::move(pDevice))
      , m_vkClearColor(rVkClearColor)
    {
    }

    void prepareExecution(const ICommandBuffer& rCommandBuffer, const VkExtent2D&,
                          std::shared_ptr<IImage> pOutputImage) override
    {
        {
            auto pBarrierRecorder = rCommandBuffer.startScopedBarrier("beforeClearColor");
            pBarrierRecorder->addImageBarrier(*pOutputImage, ImageBarrierConfig{
                                                                 .vkDstStageMask = VK_PIPELINE_STAGE_2_CLEAR_BIT,
                                                                 .vkDstAccessMask = VK_ACCESS_2_MEMORY_WRITE_BIT,
                                                                 .vkLayout = VK_IMAGE_LAYOUT_GENERAL,
                                                             });
        }

        const VkImageSubresourceRange vkRange{
            .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
            .levelCount = 1U,
            .layerCount = 1U,
        };

        const auto& rFcts = m_pDevice->getFcts();
        rFcts.vkCmdClearColorImage(rCommandBuffer.getVkCommandBuffer(), pOutputImage->getVkImage(),
                                   VK_IMAGE_LAYOUT_GENERAL, &m_vkClearColor, 1U, &vkRange);
    }

    void resize(const VkExtent2D&, uint32_t) override {}

    auto getDevice() const -> std::shared_ptr<const IDevice> override { return m_pDevice; }

private:
    std::shared_ptr<const IDevice> m_pDevice;
    const VkClearColorValue m_vkClearColor{};
};

}  // namespace im3e