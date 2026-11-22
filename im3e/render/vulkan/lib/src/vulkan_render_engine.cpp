#include <im3e/render/vulkan/vulkan_render_engine.h>

#include "vulkan_frame_pipeline.h"
#include "vulkan_render_context.h"

#include <im3e/utils/core/throw_utils.h>

#include <utility>

using namespace im3e;

namespace {

class VulkanRenderEngine : public IVulkanRenderEngine
{
public:
    explicit VulkanRenderEngine(std::shared_ptr<IDevice> device)
      : m_device(throwIfArgNull(std::move(device), "Vulkan render engine requires a device"))
      , m_logger(m_device->createLogger("VulkanRenderEngine"))
      , m_context(createVulkanRenderContext(m_device))
    {
    }

    auto createFramePipeline() -> std::unique_ptr<IFramePipeline> override
    {
        return createVulkanFramePipeline(m_context);
    }

private:
    std::shared_ptr<IDevice> m_device;
    std::unique_ptr<ILogger> m_logger;
    std::shared_ptr<IVulkanRenderContext> m_context;
};

}  // namespace

auto im3e::createVulkanRenderEngine(std::shared_ptr<IDevice> device) -> std::unique_ptr<IVulkanRenderEngine>
{
    return std::make_unique<VulkanRenderEngine>(std::move(device));
}