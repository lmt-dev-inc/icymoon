#pragma once

#include <im3e/utils/loggers.h>
#include <im3e/utils/stats.h>
#include <im3e/vulkan/command_buffer.h>
#include <im3e/vulkan/image.h>
#include <im3e/vulkan/vulkan_functions.h>

#include <functional>
#include <memory>
#include <string_view>
#include <vector>

namespace im3e {

class IDevice
{
public:
    virtual ~IDevice() = default;

    virtual auto createLogger(std::string_view name) const -> std::unique_ptr<ILogger> = 0;

    virtual auto getStatsProvider() -> std::shared_ptr<IStatsProvider> = 0;

    virtual auto getVkInstance() const -> VkInstance = 0;
    virtual auto getVkPhysicalDevice() const -> VkPhysicalDevice = 0;
    virtual auto getVkDevice() const -> VkDevice = 0;
    virtual auto getFcts() const -> const VulkanDeviceFcts& = 0;
    virtual auto getInstanceFcts() const -> const VulkanInstanceFcts& = 0;
    virtual auto getImageFactory() const -> std::shared_ptr<const IImageFactory> = 0;
    virtual auto getCommandQueue() const -> std::shared_ptr<const ICommandQueue> = 0;
    virtual auto getCommandQueue() -> std::shared_ptr<ICommandQueue> = 0;
};

using IsPresentationSupportedFct = std::function<bool(VkInstance, VkPhysicalDevice, uint32_t queueFamilyIndex)>;

struct DeviceConfig
{
    bool isDebugEnabled = false;
    IsPresentationSupportedFct isPresentationSupported{};
    std::vector<const char*> requiredInstanceExtensions{};
};
auto createDevice(const ILogger& logger, DeviceConfig config = {}) -> std::shared_ptr<IDevice>;

}  // namespace im3e
