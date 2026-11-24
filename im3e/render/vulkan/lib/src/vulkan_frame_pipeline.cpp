#include "vulkan_frame_pipeline.h"

#include "triangle.slang.h"

#include <im3e/utils/core/throw_utils.h>
#include <im3e/utils/loggers.h>
#include <im3e/vulkan/shader_utils.h>
#include <im3e/vulkan/utils.h>

#include <array>
#include <stdexcept>
#include <utility>

using namespace im3e;

namespace {

constexpr auto ColorVkFormat = VK_FORMAT_R8G8B8A8_UNORM;

auto createGraphicsPipeline(const IDevice& device, VkPipelineLayout vkPipelineLayout)
{
    auto vkDevice = device.getVkDevice();
    auto& fcts = device.getFcts();

    auto vkShaderModule = createVkShaderModule(device, shaders::getTriangleShader());

    const VkPipelineRenderingCreateInfo vkPipelineRenderingCreateInfo{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO,
        .colorAttachmentCount = 1U,
        .pColorAttachmentFormats = &ColorVkFormat,
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
    return makeVkUniquePtr<VkPipeline>(vkDevice, vkPipeline, fcts.vkDestroyPipeline);
}

class VulkanFramePipeline : public IFramePipeline
{
public:
    explicit VulkanFramePipeline(std::shared_ptr<IVulkanRenderContext> context)
      : m_context(throwIfArgNull(std::move(context), "Vulkan frame pipeline requires a context"))
      , m_device(m_context->getDevice())
      , m_logger(m_device->createLogger("VulkanFramePipeline"))
      , m_vkPipelineLayout(createVkPipelineLayout(*m_device))
      , m_vkPipeline(createGraphicsPipeline(*m_device, m_vkPipelineLayout.get()))
    {
    }

    void prepareExecution(const ICommandBuffer& commandBuffer, const VkExtent2D& vkViewportSize,
                          std::shared_ptr<IImage> outputImage) override
    {
        {
            auto barrier = commandBuffer.startScopedBarrier("barrierBeforeRendering");
            barrier->addImageBarrier(*m_image, ImageBarrierConfig{
                                                   .vkDstStageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
                                                   .vkDstAccessMask = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT,
                                                   .vkLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                                               });
        }

        const auto vkCommandBuffer = commandBuffer.getVkCommandBuffer();
        auto& fcts = m_device->getFcts();
        {
            const VkRenderingAttachmentInfo vkColorAttachmentInfo{
                .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
                .imageView = m_imageView->getVkImageView(),
                .imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
                .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
                .clearValue = VkClearValue{},
            };
            const VkRenderingInfo vkRenderingInfo{
                .sType = VK_STRUCTURE_TYPE_RENDERING_INFO,
                .renderArea = VkRect2D{.extent = vkViewportSize},
                .layerCount = 1U,
                .colorAttachmentCount = 1U,
                .pColorAttachments = &vkColorAttachmentInfo,
            };
            fcts.vkCmdBeginRendering(vkCommandBuffer, &vkRenderingInfo);
            fcts.vkCmdBindPipeline(vkCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_vkPipeline.get());

            const VkViewport vkViewport{
                .width = static_cast<float>(vkViewportSize.width),
                .height = static_cast<float>(vkViewportSize.height),
                .minDepth = 0.0F,
                .maxDepth = 1.0F,
            };
            fcts.vkCmdSetViewport(vkCommandBuffer, 0U, 1U, &vkViewport);

            const VkRect2D vkScissor{.extent = vkViewportSize};
            fcts.vkCmdSetScissor(vkCommandBuffer, 0U, 1U, &vkScissor);

            fcts.vkCmdDraw(vkCommandBuffer, 3U, 1U, 0U, 0U);

            fcts.vkCmdEndRendering(vkCommandBuffer);
        }

        {
            auto barrier = commandBuffer.startScopedBarrier("barrierAfterRendering");
            barrier->addImageBarrier(*m_image, ImageBarrierConfig{
                                                   .vkDstStageMask = VK_PIPELINE_STAGE_2_BLIT_BIT,
                                                   .vkDstAccessMask = VK_ACCESS_2_TRANSFER_READ_BIT,
                                                   .vkLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                                               });
            barrier->addImageBarrier(*outputImage, ImageBarrierConfig{
                                                       .vkDstStageMask = VK_PIPELINE_STAGE_2_BLIT_BIT,
                                                       .vkDstAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT,
                                                       .vkLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                                                   });
        }

        const VkOffset3D vkEndOffset{
            .x = static_cast<int32_t>(vkViewportSize.width),
            .y = static_cast<int32_t>(vkViewportSize.height),
            .z = 1,
        };
        const VkImageBlit vkRegion{
            .srcSubresource = m_image->getVkSubresourceLayers(),
            .srcOffsets = {VkOffset3D{}, vkEndOffset},
            .dstSubresource = outputImage->getVkSubresourceLayers(),
            .dstOffsets = {VkOffset3D{}, vkEndOffset},
        };
        fcts.vkCmdBlitImage(vkCommandBuffer, m_image->getVkImage(), VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                            outputImage->getVkImage(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1U, &vkRegion,
                            VK_FILTER_NEAREST);
    }

    void resize(const VkExtent2D& vkExtent, [[maybe_unused]] uint32_t frameInFlightCount) override
    {
        m_image = m_device->getImageFactory()->createImage(ImageConfig{
            .name = "VulkanPipelineImage",
            .vkExtent = vkExtent,
            .vkFormat = ColorVkFormat,
            .vkUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
        });
        m_imageView = m_image->createView();
    }

    auto getDevice() const -> std::shared_ptr<const IDevice> override { return m_device; }

private:
    std::shared_ptr<IVulkanRenderContext> m_context;
    std::shared_ptr<IDevice> m_device;
    std::unique_ptr<ILogger> m_logger;

    VkUniquePtr<VkPipelineLayout> m_vkPipelineLayout;
    VkUniquePtr<VkPipeline> m_vkPipeline;
    std::unique_ptr<IImage> m_image;
    std::unique_ptr<IImageView> m_imageView;
};

}  // namespace

auto im3e::createVulkanFramePipeline(std::shared_ptr<IVulkanRenderContext> context) -> std::unique_ptr<IFramePipeline>
{
    return std::make_unique<VulkanFramePipeline>(std::move(context));
}