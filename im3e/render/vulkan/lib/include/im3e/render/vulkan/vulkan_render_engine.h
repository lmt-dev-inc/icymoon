#pragma once

#include <im3e/vulkan/device.h>
#include <im3e/vulkan/frame_pipeline.h>

#include <memory>

namespace im3e {

/// @brief Entry point of the Vulkan renderer: creates the frame pipelines that render with Vulkan.
///
/// The engine holds what its frame pipelines share, such as GPU resources that several views use. Each frame
/// pipeline renders one view, e.g. a render panel of a window. A frame pipeline keeps what it shares alive, so it
/// remains valid after the engine is destroyed.
class IVulkanRenderEngine
{
public:
    IVulkanRenderEngine() = default;
    IVulkanRenderEngine(const IVulkanRenderEngine&) = delete;
    IVulkanRenderEngine(IVulkanRenderEngine&&) = delete;
    auto operator=(const IVulkanRenderEngine&) -> IVulkanRenderEngine& = delete;
    auto operator=(IVulkanRenderEngine&&) -> IVulkanRenderEngine& = delete;
    virtual ~IVulkanRenderEngine() = default;

    /// @brief Create a frame pipeline that renders one view with the resources of this engine.
    /// @return A new frame pipeline, rendering with the device of the engine.
    virtual auto createFramePipeline() -> std::unique_ptr<IFramePipeline> = 0;
};

/// @brief Create a Vulkan render engine.
/// @param device Device that the engine and its frame pipelines render with. Must not be null.
/// @return A new render engine.
/// @throws std::invalid_argument if device is null.
auto createVulkanRenderEngine(std::shared_ptr<IDevice> device) -> std::unique_ptr<IVulkanRenderEngine>;

}  // namespace im3e
