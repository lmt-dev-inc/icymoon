#pragma once

#include <im3e/utils/loggers.h>
#include <im3e/vulkan/device.h>
#include <im3e/vulkan/frame_pipeline.h>

#include <memory>

namespace im3e {

class IVulkanRenderEngine
{
public:
    IVulkanRenderEngine() = default;
    IVulkanRenderEngine(const IVulkanRenderEngine&) = delete;
    IVulkanRenderEngine(IVulkanRenderEngine&&) = delete;
    auto operator=(const IVulkanRenderEngine&) -> IVulkanRenderEngine& = delete;
    auto operator=(IVulkanRenderEngine&&) -> IVulkanRenderEngine& = delete;
    virtual ~IVulkanRenderEngine() = default;

    virtual auto createFramePipeline() -> std::unique_ptr<IFramePipeline> = 0;
};

auto createVulkanRenderEngine(std::shared_ptr<IDevice> device) -> std::unique_ptr<IVulkanRenderEngine>;

}  // namespace im3e