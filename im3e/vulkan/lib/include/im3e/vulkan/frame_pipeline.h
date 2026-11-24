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
    /// @pre resize() was called at least once.
    /// @param commandBuffer Object used to record commands that will be executed.
    /// @param vkViewportSize Size of the content to be rendered in the output image starting from the top-left corner.
    /// Must not be larger than the extent given to the last call to resize(), nor than the output image.
    /// @param outputImage Image that the pipeline should write the final results to.
    virtual void prepareExecution(const ICommandBuffer& commandBuffer, const VkExtent2D& vkViewportSize,
                                  std::shared_ptr<IImage> outputImage) = 0;

    /// @brief Resize the frame
    ///
    /// @pre No command recorded by prepareExecution() is still executing, as the pipeline may destroy the resources
    /// they use.
    /// @param vkExtent Largest viewport size that prepareExecution() can be given until the next call to resize().
    /// @param frameInFlightCount Number of frames that can be executing at the same time.
    virtual void resize(const VkExtent2D& vkExtent, uint32_t frameInFlightCount) = 0;

    virtual auto getDevice() const -> std::shared_ptr<const IDevice> = 0;
};

}  // namespace im3e