#pragma once

#include <im3e/api/device.h>
#include <im3e/utils/loggers.h>

#include <functional>
#include <memory>

namespace im3e {

using IsPresentationSupportedFct = std::function<bool(VkInstance, VkPhysicalDevice, uint32_t queueFamilyIndex)>;

struct DeviceConfig
{
    bool isDebugEnabled = false;
    IsPresentationSupportedFct isPresentationSupported{};
    std::vector<const char*> requiredInstanceExtensions{};
};
auto createDevice(const ILogger& rLogger, DeviceConfig config = {}) -> std::shared_ptr<IDevice>;

}  // namespace im3e