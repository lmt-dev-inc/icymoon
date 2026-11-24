#include <im3e/vulkan/shader_utils.h>

#include <array>

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

auto im3e::createVkGraphicsPipeline(const IDevice& device, VkPipelineLayout vkPipelineLayout,
                                    const CompiledShader& shader, const GraphicsPipelineConfig& config)
    -> VkUniquePtr<VkPipeline>
{
    auto vkDevice = device.getVkDevice();
    auto& fcts = device.getFcts();

    auto vkShaderModule = createVkShaderModule(device, shader);

    const VkPipelineRenderingCreateInfo vkPipelineRenderingCreateInfo{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO,
        .colorAttachmentCount = 1U,
        .pColorAttachmentFormats = &config.vkColorFormat,
    };

    const auto vkStageCreateInfos = std::to_array({
        VkPipelineShaderStageCreateInfo{
            .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
            .stage = VK_SHADER_STAGE_VERTEX_BIT,
            .module = vkShaderModule.get(),
            .pName = "vertexMain",
        },
        VkPipelineShaderStageCreateInfo{
            .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
            .stage = VK_SHADER_STAGE_FRAGMENT_BIT,
            .module = vkShaderModule.get(),
            .pName = "fragmentMain",
        },
    });

    const VkPipelineVertexInputStateCreateInfo vkVertexInputStateCreateInfo{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
    };

    const VkPipelineInputAssemblyStateCreateInfo vkInputAssembyStateCreateInfo{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
        .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
    };

    const VkPipelineViewportStateCreateInfo vkViewportStateCreateInfo{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
        .viewportCount = 1U,
        .scissorCount = 1U,
    };

    const VkPipelineRasterizationStateCreateInfo vkRasterizationStateCreateInfo{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
        .polygonMode = VK_POLYGON_MODE_FILL,
        .cullMode = VK_CULL_MODE_NONE,
        .lineWidth = 1.0F,
    };

    const VkPipelineMultisampleStateCreateInfo vkMultisampleStateCreateInfo{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
        .rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
    };

    const VkPipelineColorBlendAttachmentState vkColorBlendAttachment{
        .blendEnable = VK_FALSE,
        .colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT |
                          VK_COLOR_COMPONENT_A_BIT,
    };
    const VkPipelineColorBlendStateCreateInfo vkColorBlendStateCreateInfo{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
        .logicOpEnable = VK_FALSE,
        .attachmentCount = 1U,
        .pAttachments = &vkColorBlendAttachment,
    };

    const auto vkDynamicStates = std::to_array({
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_SCISSOR,
    });
    const VkPipelineDynamicStateCreateInfo vkDynamicStateCreateInfo{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
        .dynamicStateCount = static_cast<uint32_t>(vkDynamicStates.size()),
        .pDynamicStates = vkDynamicStates.data(),
    };

    const VkGraphicsPipelineCreateInfo vkCreateInfo{
        .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
        .pNext = &vkPipelineRenderingCreateInfo,
        .stageCount = static_cast<uint32_t>(vkStageCreateInfos.size()),
        .pStages = vkStageCreateInfos.data(),
        .pVertexInputState = &vkVertexInputStateCreateInfo,
        .pInputAssemblyState = &vkInputAssembyStateCreateInfo,
        .pViewportState = &vkViewportStateCreateInfo,
        .pRasterizationState = &vkRasterizationStateCreateInfo,
        .pMultisampleState = &vkMultisampleStateCreateInfo,
        .pColorBlendState = &vkColorBlendStateCreateInfo,
        .pDynamicState = &vkDynamicStateCreateInfo,
        .layout = vkPipelineLayout,
    };

    VkPipeline vkPipeline{};
    throwIfVkFailed(fcts.vkCreateGraphicsPipelines(vkDevice, nullptr, 1U, &vkCreateInfo, nullptr, &vkPipeline),
                    "Failed to create graphics pipeline");
    return makeVkUniquePtr(vkDevice, vkPipeline, fcts.vkDestroyPipeline);
}