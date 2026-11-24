#include <im3e/vulkan/shader_utils.h>

using namespace im3e;

auto im3e::createVkShaderModule(const IDevice& device, const CompiledShader& shader) -> VkUniquePtr<VkShaderModule>
{
    auto vkDevice = device.getVkDevice();
    auto& fcts = device.getFcts();

    const auto& spirv = shader.getSpirv();

    const VkShaderModuleCreateInfo vkCreateInfo{
        .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
        .codeSize = spirv.size_bytes(),
        .pCode = spirv.data(),
    };

    VkShaderModule vkShaderModule{};
    throwIfVkFailed(fcts.vkCreateShaderModule(vkDevice, &vkCreateInfo, nullptr, &vkShaderModule),
                    "Failed to create shader module");
    return makeVkUniquePtr(vkDevice, vkShaderModule, fcts.vkDestroyShaderModule);
}

auto im3e::createVkPipelineLayout(const IDevice& device) -> VkUniquePtr<VkPipelineLayout>
{
    auto vkDevice = device.getVkDevice();
    auto& fcts = device.getFcts();

    const VkPipelineLayoutCreateInfo vkCreateInfo{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
    };

    VkPipelineLayout vkLayout{};
    throwIfVkFailed(fcts.vkCreatePipelineLayout(vkDevice, &vkCreateInfo, nullptr, &vkLayout),
                    "Failed to create pipeline layout");
    return makeVkUniquePtr(vkDevice, vkLayout, fcts.vkDestroyPipelineLayout);
}
