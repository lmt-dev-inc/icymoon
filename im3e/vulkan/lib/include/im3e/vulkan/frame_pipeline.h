#pragma once

#include <im3e/vulkan/command_buffer.h>
#include <im3e/vulkan/device.h>
#include <im3e/vulkan/image.h>
#include <im3e/vulkan/utils.h>

namespace im3e {

/// @brief Pipeline of commands to execute to generate a frame.
class IFramePipeline
{
public:
    virtual ~IFramePipeline() = default;

    /// @brief Schedule pipeline execution
    ///
    /// @param commandBuffer Object used to record commands that will be executed.
    /// @param vkViewportSize Size of the content to be rendered in the output image starting from the top-left corner.
    /// @param outputImage Image that the pipeline should write the final results to.
    virtual void prepareExecution(const ICommandBuffer& commandBuffer, const VkExtent2D& vkViewportSize,
                                  std::shared_ptr<IImage> outputImage) = 0;

    /// @brief Resize the frame
    virtual void resize(const VkExtent2D& vkExtent, uint32_t frameInFlightCount) = 0;

    virtual auto getDevice() const -> std::shared_ptr<const IDevice> = 0;
};

}  // namespace im3e