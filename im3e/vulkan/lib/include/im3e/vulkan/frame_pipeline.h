#pragma once

#include <im3e/utils/vk_utils.h>
#include <im3e/vulkan/command_buffer.h>
#include <im3e/vulkan/device.h>
#include <im3e/vulkan/image.h>

namespace im3e {

/// @brief Pipeline of commands to execute to generate a frame.
class IFramePipeline
{
public:
    virtual ~IFramePipeline() = default;

    /// @brief Schedule pipeline execution
    ///
    /// @param rCommandBuffer Object used to record commands that will be executed.
    /// @param rVkViewportSize Size of the content to be rendered in the output image starting from the top-left corner.
    /// @param pOutputImage Image that the pipeline should write the final results to.
    virtual void prepareExecution(const ICommandBuffer& rCommandBuffer, const VkExtent2D& rVkViewportSize,
                                  std::shared_ptr<IImage> pOutputImage) = 0;

    /// @brief Resize the frame
    virtual void resize(const VkExtent2D& rVkExtent, uint32_t frameInFlightCount) = 0;

    virtual auto getDevice() const -> std::shared_ptr<const IDevice> = 0;
};

}  // namespace im3e