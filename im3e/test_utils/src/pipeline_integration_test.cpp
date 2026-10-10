#include "pipeline_integration_test.h"

#include <fmt/format.h>
#include <fmt/std.h>

#include <filesystem>

using namespace im3e;

void PipelineIntegrationTest::TearDown()
{
    if (Test::HasFailure())
    {
        const auto outputFilePath = generateFilePath(getName(), "png");
        if (m_pHostVisibleOutputImage)
        {
            auto pImageMapping = m_pHostVisibleOutputImage->mapReadOnly();
            pImageMapping->save(outputFilePath);
        }
        ADD_FAILURE() << fmt::format(R"(Saved pipeline output to : "{}")", outputFilePath);
    }

    DeviceIntegrationTest::TearDown();
}

auto PipelineIntegrationTest::mapOutputImage() const -> std::unique_ptr<const IHostVisibleImage::IMapping>
{
    return m_pHostVisibleOutputImage->mapReadOnly();
}

void PipelineIntegrationTest::expectRgbaPixel(const IHostVisibleImage::IMapping& rMapping,
                                              const std::array<uint32_t, 2U>& rPos,
                                              const std::array<uint8_t, 4U>& rExpected)
{
    const auto& rRgbaPixel = *reinterpret_cast<const std::array<uint8_t, 4U>*>(rMapping.getPixel(rPos[0], rPos[1]));
    EXPECT_THAT(rRgbaPixel, ContainerEq(rExpected)) << fmt::format("Pixel at [{}; {}]", rPos[0], rPos[1]);
}

void PipelineIntegrationTest::expectRgbaPixelRegion(const IHostVisibleImage::IMapping& rMapping,
                                                    const std::array<uint32_t, 2U>& rMinPos,
                                                    const std::array<uint32_t, 2U>& rMaxPos,
                                                    const std::array<uint8_t, 4U>& rExpected)
{
    for (uint32_t y = rMinPos[1]; y < rMaxPos[1]; y++)
    {
        auto* pIt = reinterpret_cast<const std::array<uint8_t, 4U>*>(rMapping.getPixel(rMinPos[0], y));
        for (uint32_t x = rMinPos[0]; x < rMaxPos[0]; x++)
        {
            EXPECT_THAT(*pIt, ContainerEq(rExpected)) << fmt::format("Pixel at [{}; {}]", x, y);
            pIt++;
        }
    }
}

void PipelineIntegrationTest::initialize(Config config, std::unique_ptr<IFramePipeline> pFramePipeline)
{
    throwIfArgNull(pFramePipeline.get(), "Cannot initialize pipeline integration test without a pipeline");

    std::shared_ptr<IFramePipeline> pSharedPipeline(std::move(pFramePipeline));
    pSharedPipeline->resize(config.vkOutputExtent, config.frameInFlightCount);
    initialize(std::move(config),
               [pSharedPipeline, vkViewportSize = config.vkOutputExtent](auto& rCommandBuffer, auto pImage) {
                   pSharedPipeline->prepareExecution(rCommandBuffer, vkViewportSize, std::move(pImage));
               });
}

void PipelineIntegrationTest::initialize(Config config, PipelineFct pipelineFct)
{
    m_config = std::move(config);
    m_pipelineFct = throwIfArgNull(std::move(pipelineFct),
                                   "Cannot initialize pipeline integration test without a pipeline function");

    auto pImageFactory = getDevice()->getImageFactory();
    m_pOutputImage = pImageFactory->createImage(ImageConfig{
        .name = "PipelineTestOutputImage",
        .vkExtent = m_config.vkOutputExtent,
        .vkFormat = m_config.vkOutputFormat,
        .vkUsage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
    });
    m_pHostVisibleOutputImage = pImageFactory->createHostVisibleImage(ImageConfig{
        .name = "PipelineTestOutputImage",
        .vkExtent = m_config.vkOutputExtent,
        .vkFormat = m_config.vkOutputFormat,
        .vkUsage = VK_IMAGE_USAGE_TRANSFER_DST_BIT,
    });
}

void PipelineIntegrationTest::runTest(uint32_t iterationCount)
{
    auto pCommandQueue = getDevice()->getCommandQueue();

    for (uint32_t iteration = 0U; iteration < iterationCount; iteration++)
    {
        auto pCommandBuffer = pCommandQueue->startScopedCommand(fmt::format("PipelineTestCommand{}", iteration),
                                                                CommandExecutionType::Async);
        m_pipelineFct(*pCommandBuffer, m_pOutputImage);
    }

    {
        auto pCommandBuffer = pCommandQueue->startScopedCommand("PipelineTestBlitToHostVisible",
                                                                CommandExecutionType::Sync);
        {
            auto pBarrierRecorder = pCommandBuffer->startScopedBarrier("prepareHostVisibleOutput");
            pBarrierRecorder->addImageBarrier(*m_pOutputImage, ImageBarrierConfig{
                                                                   .vkDstStageMask = VK_PIPELINE_STAGE_2_COPY_BIT,
                                                                   .vkDstAccessMask = VK_ACCESS_2_TRANSFER_READ_BIT,
                                                                   .vkLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                                                               });
            pBarrierRecorder->addImageBarrier(*m_pHostVisibleOutputImage,
                                              ImageBarrierConfig{
                                                  .vkDstStageMask = VK_PIPELINE_STAGE_2_COPY_BIT,
                                                  .vkDstAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT,
                                                  .vkLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                                              });
        }
        VkImageCopy vkRegion{
            .srcSubresource = m_pOutputImage->getVkSubresourceLayers(),
            .dstSubresource = m_pHostVisibleOutputImage->getVkSubresourceLayers(),
            .extent = toVkExtent3D(m_pOutputImage->getVkExtent()),
        };
        getDevice()->getFcts().vkCmdCopyImage(
            pCommandBuffer->getVkCommandBuffer(), m_pOutputImage->getVkImage(), VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
            m_pHostVisibleOutputImage->getVkImage(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1U, &vkRegion);
    }
}
