#include "vulkan_frame_pipeline.h"

#include <im3e/utils/core/throw_utils.h>
#include <im3e/utils/loggers.h>

#include <stdexcept>
#include <utility>

using namespace im3e;

namespace {

class VulkanFramePipeline : public IFramePipeline
{
public:
    VulkanFramePipeline(std::shared_ptr<IVulkanRenderContext> context)
      : m_context(throwIfArgNull(std::move(context), "Vulkan frame pipeline requires a context"))
      , m_device(m_context->getDevice())
      , m_logger(m_device->createLogger("VulkanFramePipeline"))
    {
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
};

}  // namespace

auto im3e::createVulkanFramePipeline(std::shared_ptr<IVulkanRenderContext> context) -> std::unique_ptr<IFramePipeline>
{
    return std::make_unique<VulkanFramePipeline>(std::move(context));
}