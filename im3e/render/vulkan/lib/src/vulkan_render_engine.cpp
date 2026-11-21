#include <im3e/render/vulkan/vulkan_render_engine.h>

#include <im3e/utils/core/throw_utils.h>

using namespace im3e;

namespace {

class VulkanRenderEngine : public IVulkanRenderEngine
{
public:
    VulkanRenderEngine(const ILogger& logger, std::shared_ptr<IDevice> device)
      : m_logger(logger.createChild("VulkanRenderEngine"))
      , m_device(throwIfArgNull(std::move(device), "Vulkan render engine requires a device"))
    {
    }

    auto createFramePipeline() -> std::unique_ptr<IFramePipeline> override { return nullptr; }

private:
    std::unique_ptr<ILogger> m_logger;
    std::shared_ptr<IDevice> m_device;
};

}  // namespace

auto im3e::createVulkanRenderEngine(const ILogger& logger, std::shared_ptr<IDevice> device)
    -> std::unique_ptr<IVulkanRenderEngine>
{
    return std::make_unique<VulkanRenderEngine>(logger, std::move(device));
}