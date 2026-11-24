#pragma once

#include <im3e/shader/compiled_shader.h>
#include <im3e/vulkan/device.h>
#include <im3e/vulkan/utils.h>

namespace im3e {

/// @brief Create a shader module from SPIR-V compiled at build time.
/// @details The module holds every entry point of the shader. It is only needed to create pipelines, and can be
/// destroyed as soon as they are created.
/// @throws std::runtime_error if Vulkan fails to create the module.
auto createVkShaderModule(const IDevice& device, const CompiledShader& shader) -> VkUniquePtr<VkShaderModule>;

/// @brief Create an empty pipeline layout, without descriptor set layouts or push constant ranges.
/// @details For shaders without resources. Unlike a shader module, keep the layout alive while the pipelines created
/// with it are used: destroying it earlier requires the maintenance4 feature, which the device doesn't enable.
/// @throws std::runtime_error if Vulkan fails to create the layout.
auto createVkPipelineLayout(const IDevice& device) -> VkUniquePtr<VkPipelineLayout>;

}  // namespace im3e