#include "vulkan_render_context.h"

#include <im3e/utils/core/throw_utils.h>

#include <utility>

using namespace im3e;

namespace {

class VulkanRenderContext : public IVulkanRenderContext
{
public:
    explicit VulkanRenderContext(std::shared_ptr<IDevice> device)
      : m_device(throwIfArgNull(std::move(device), "Vulkan render context requires a device"))
      , m_logger(m_device->createLogger("VulkanRenderContext"))
    {
    }

    auto getDevice() -> std::shared_ptr<IDevice> override { return m_device; }
    auto getDevice() const -> std::shared_ptr<const IDevice> override { return m_device; }

private:
    std::shared_ptr<IDevice> m_device;
    std::unique_ptr<ILogger> m_logger;
};

}  // namespace

auto im3e::createVulkanRenderContext(std::shared_ptr<IDevice> device) -> std::shared_ptr<IVulkanRenderContext>
{
    return std::make_shared<VulkanRenderContext>(std::move(device));
}