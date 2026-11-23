#include "vulkan_frame_pipeline.h"

#include "triangle.slang.h"

#include <im3e/utils/core/throw_utils.h>
#include <im3e/utils/loggers.h>
#include <im3e/vulkan/utils.h>

#include <array>
#include <stdexcept>
#include <utility>

using namespace im3e;

namespace {

constexpr auto OutputVkFormat = VK_FORMAT_R8G8B8A8_UNORM;

auto createShaderModule(const IDevice& device)
{
    auto vkDevice = device.getVkDevice();
    auto& fcts = device.getFcts();

    const auto& shader = shaders::getTriangleShader();
    const auto& spirv = shader.getSpirv();

    const VkShaderModuleCreateInfo vkCreateInfo{
        .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
        .codeSize = spirv.size() * sizeof(uint32_t),
        .pCode = spirv.data(),
    };

    VkShaderModule vkShaderModule{};
    throwIfVkFailed(fcts.vkCreateShaderModule(vkDevice, &vkCreateInfo, nullptr, &vkShaderModule),
                    "Failed to create shader module");
    return makeVkUniquePtr<VkShaderModule>(vkDevice, vkShaderModule, fcts.vkDestroyShaderModule);
}

auto createPipelineLayout(const IDevice& device)
{
    auto vkDevice = device.getVkDevice();
    auto& fcts = device.getFcts();

    const VkPipelineLayoutCreateInfo vkCreateInfo{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
    };

    VkPipelineLayout vkLayout;
    throwIfVkFailed(fcts.vkCreatePipelineLayout(vkDevice, &vkCreateInfo, nullptr, &vkLayout),
                    "Failed to create pipeline layout");
    return makeVkUniquePtr<VkPipelineLayout>(vkDevice, vkLayout, fcts.vkDestroyPipelineLayout);
}

auto createGraphicsPipeline(const IDevice& device, VkShaderModule vkShaderModule, VkPipelineLayout vkPipelineLayout)
{
    auto vkDevice = device.getVkDevice();
    auto& fcts = device.getFcts();

    const VkPipelineRenderingCreateInfo vkPipelineRenderingCreateInfo{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO,
        .colorAttachmentCount = 1U,
        .pColorAttachmentFormats = &OutputVkFormat,
    };

    const auto vkStageCreateInfos = std::to_array({
        VkPipelineShaderStageCreateInfo{
            .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
            .stage = VK_SHADER_STAGE_VERTEX_BIT,
            .module = vkShaderModule,
            .pName = "vertexMain",
        },
        VkPipelineShaderStageCreateInfo{
            .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
            .stage = VK_SHADER_STAGE_FRAGMENT_BIT,
            .module = vkShaderModule,
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
    return makeVkUniquePtr<VkPipeline>(vkDevice, vkPipeline, fcts.vkDestroyPipeline);
}

class VulkanFramePipeline : public IFramePipeline
{
public:
    explicit VulkanFramePipeline(std::shared_ptr<IVulkanRenderContext> context)
      : m_context(throwIfArgNull(std::move(context), "Vulkan frame pipeline requires a context"))
      , m_device(m_context->getDevice())
      , m_logger(m_device->createLogger("VulkanFramePipeline"))
    {
        auto vkShaderModule = createShaderModule(*m_device);
        auto vkPipelineLayout = createPipelineLayout(*m_device);
        auto vkPipeline = createGraphicsPipeline(*m_device, vkShaderModule.get(), vkPipelineLayout.get());
    }

    void prepareExecution([[maybe_unused]] const ICommandBuffer& commandBuffer,
                          [[maybe_unused]] const VkExtent2D& vkViewportSize,
                          [[maybe_unused]] std::shared_ptr<IImage> outputImage) override
    {
        throw std::runtime_error("VulkanFramePipeline::prepareExecution() not implemented");
    }

    void resize(const VkExtent2D& vkExtent, [[maybe_unused]] uint32_t frameInFlightCount) override
    {
        m_image = m_device->getImageFactory()->createImage(ImageConfig{
            .name = "VulkanPipelineImage",
            .vkExtent = vkExtent,
            .vkFormat = OutputVkFormat,
            .vkUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
        });
        m_imageView = m_image->createView();
    }

    auto getDevice() const -> std::shared_ptr<const IDevice> override { return m_device; }

private:
    std::shared_ptr<IVulkanRenderContext> m_context;
    std::shared_ptr<IDevice> m_device;
    std::unique_ptr<ILogger> m_logger;

    VkUniquePtr<VkShaderModule> m_vkShaderModule;
    std::unique_ptr<IImage> m_image;
    std::unique_ptr<IImageView> m_imageView;
};

}  // namespace

auto im3e::createVulkanFramePipeline(std::shared_ptr<IVulkanRenderContext> context) -> std::unique_ptr<IFramePipeline>
{
    return std::make_unique<VulkanFramePipeline>(std::move(context));
}