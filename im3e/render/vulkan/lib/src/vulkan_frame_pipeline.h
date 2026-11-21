#pragma once

#include <im3e/vulkan/frame_pipeline.h>

namespace im3e {

auto createVulkanFramePipeline() -> std::unique_ptr<IFramePipeline>;

}  // namespace im3e