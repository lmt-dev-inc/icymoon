#include "vulkan_frame_pipeline.h"

#include "triangle.slang.h"

#include <im3e/utils/core/throw_utils.h>
#include <im3e/utils/loggers.h>
#include <im3e/vulkan/utils.h>

#include <stdexcept>
#include <utility>

using namespace im3e;

namespace {

auto createShaderModule(const IDevice& device)
{
    auto vkDevice = device.getVkDevice();
    auto& fcts = device.getFcts();

    const auto& shader = shaders::getTriangleShader();
    const auto& spirv = shader.getSpirv();

    VkShaderModuleCreateInfo vkCreateInfo{
        .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
        .codeSize = spirv.size() * sizeof(uint32_t),
        .pCode = spirv.data(),
    };

    VkShaderModule vkShaderModule{};
    throwIfVkFailed(fcts.vkCreateShaderModule(vkDevice, &vkCreateInfo, nullptr, &vkShaderModule),
                    "Failed to create shader module");
    return makeVkUniquePtr<VkShaderModule>(vkDevice, vkShaderModule, fcts.vkDestroyShaderModule);
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
    }

    void prepareExecution([[maybe_unused]] const ICommandBuffer& commandBuffer,
                          [[maybe_unused]] const VkExtent2D& vkViewportSize,
                          [[maybe_unused]] std::shared_ptr<IImage> outputImage) override
    {
        throw std::runtime_error("VulkanFramePipeline::prepareExecution() not implemented");
    }

    void resize([[maybe_unused]] const VkExtent2D& vkExtent, [[maybe_unused]] uint32_t frameInFlightCount) override
    {
        throw std::runtime_error("VulkanFramePipeline::resize() not implemented");
    }

    auto getDevice() const -> std::shared_ptr<const IDevice> override { return m_device; }

private:
    std::shared_ptr<IVulkanRenderContext> m_context;
    std::shared_ptr<IDevice> m_device;
    std::unique_ptr<ILogger> m_logger;

    VkUniquePtr<VkShaderModule> m_vkShaderModule;
};

}  // namespace

auto im3e::createVulkanFramePipeline(std::shared_ptr<IVulkanRenderContext> context) -> std::unique_ptr<IFramePipeline>
{
    return std::make_unique<VulkanFramePipeline>(std::move(context));
}