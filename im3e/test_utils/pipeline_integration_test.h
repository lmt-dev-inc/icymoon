#pragma once

#include "device_integration_test.h"

#include <im3e/api/device.h>
#include <im3e/api/frame_pipeline.h>
#include <im3e/api/image.h>
#include <im3e/utils/core/types.h>
#include <im3e/utils/loggers.h>

#include <array>
#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace im3e {

class PipelineIntegrationTest : public DeviceIntegrationTest
{
public:
    using PipelineFct = std::function<void(ICommandBuffer& rCommandBuffer, std::shared_ptr<IImage> pOutputImage)>;

    void TearDown() override;

    auto mapOutputImage() const -> std::unique_ptr<const IHostVisibleImage::IMapping>;
    void expectRgbaPixel(const IHostVisibleImage::IMapping& rMapping, const std::array<uint32_t, 2U>& rPos,
                         const std::array<uint8_t, 4U>& rExpected, uint8_t tolerance = 0U);
    void expectRgbaPixelRegion(const IHostVisibleImage::IMapping& rMapping, const std::array<uint32_t, 2U>& rMinPos,
                               const std::array<uint32_t, 2U>& rMaxPos, const std::array<uint8_t, 4U>& rExpected,
                               uint8_t tolerance = 0U);

protected:
    struct Config
    {
        VkExtent2D vkOutputExtent{};
        VkFormat vkOutputFormat{};
        uint32_t frameInFlightCount{};
    };
    void initialize(Config config, std::unique_ptr<IFramePipeline> pFramePipeline);
    void initialize(Config config, PipelineFct pipelineFct);

    /// @brief Run the integration test and save results into output image.
    ///
    /// @param iterationCount Number of times to execute the frame pipeline for this test.
    void runTest(uint32_t iterationCount = 1U);

private:
    Config m_config;
    PipelineFct m_pipelineFct;
    std::shared_ptr<IImage> m_pOutputImage;
    std::shared_ptr<IHostVisibleImage> m_pHostVisibleOutputImage;
};

}  // namespace im3e
