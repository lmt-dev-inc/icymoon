#pragma once

#include <im3e/utils/loggers.h>
#include <im3e/vulkan/device.h>

#include <memory>

namespace im3e {

class IVulkanRenderContext
{
public:
    virtual ~IVulkanRenderContext() = default;

    virtual auto getDevice() -> std::shared_ptr<IDevice> = 0;
    virtual auto getDevice() const -> std::shared_ptr<const IDevice> = 0;
};

auto createVulkanRenderContext(std::shared_ptr<IDevice> device) -> std::shared_ptr<IVulkanRenderContext>;

}  // namespace im3e