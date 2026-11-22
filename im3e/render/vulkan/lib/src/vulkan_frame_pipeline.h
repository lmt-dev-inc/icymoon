#pragma once

#include "vulkan_render_context.h"

#include <im3e/vulkan/frame_pipeline.h>

#include <memory>

namespace im3e {

auto createVulkanFramePipeline(std::shared_ptr<IVulkanRenderContext> context) -> std::unique_ptr<IFramePipeline>;

}  // namespace im3e