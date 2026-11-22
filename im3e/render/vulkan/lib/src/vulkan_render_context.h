#pragma once

#include <im3e/vulkan/device.h>

#include <memory>

namespace im3e {

class IVulkanRenderContext
{
public:
    IVulkanRenderContext() = default;
    IVulkanRenderContext(const IVulkanRenderContext&) = delete;
    IVulkanRenderContext(IVulkanRenderContext&&) = delete;
    IVulkanRenderContext& operator=(const IVulkanRenderContext&) = delete;
    IVulkanRenderContext& operator=(IVulkanRenderContext&&) = delete;
    virtual ~IVulkanRenderContext() = default;

    virtual auto getDevice() -> std::shared_ptr<IDevice> = 0;
    virtual auto getDevice() const -> std::shared_ptr<const IDevice> = 0;
};

auto createVulkanRenderContext(std::shared_ptr<IDevice> device) -> std::shared_ptr<IVulkanRenderContext>;

}  // namespace im3e