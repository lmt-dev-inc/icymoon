#pragma once

#include <im3e/utils/core/throw_utils.h>
#include <im3e/utils/core/types.h>
#include <im3e/utils/loggers.h>

#include <fmt/format.h>
#include <vk_mem_alloc.h>

#include <string_view>
#include <vector>

constexpr bool operator==(const VkExtent2D& rVkExtent1, const VkExtent2D& rVkExtent2)
{
    return rVkExtent1.width == rVkExtent2.width && rVkExtent1.height == rVkExtent2.height;
}

constexpr bool operator!=(const VkExtent2D& rVkExtent1, const VkExtent2D& rVkExtent2)
{
    return rVkExtent1.width != rVkExtent2.width || rVkExtent1.height != rVkExtent2.height;
}

namespace im3e {

template <typename T>
using VkUniquePtr = std::unique_ptr<typename std::remove_pointer<T>::type, std::function<void(T)>>;

template <typename T>
using VkSharedPtr = std::shared_ptr<typename std::remove_pointer<T>::type>;

template <typename T>
using VkWeakPtr = std::weak_ptr<typename std::remove_pointer<T>::type>;

template <typename T>
auto makeVkUniquePtr(VkDevice vkDevice, T pValue,
                     std::function<void(VkDevice, T, const VkAllocationCallbacks*)> destructor)
{
    return VkUniquePtr<T>(pValue, [vkDevice, destructor](auto* pV) {
        if (destructor)
        {
            destructor(vkDevice, pV, nullptr);
        }
    });
}

inline void throwIfVkFailed(VkResult vkResult, std::string_view errorMessage)
{
    throwIfFalse<std::runtime_error>(vkResult == VK_SUCCESS,
                                     fmt::format("Vulkan Error {}: {}", static_cast<int>(vkResult), errorMessage));
}

inline void logIfVkFailed(VkResult vkResult, const ILogger& rLogger, std::string_view errorMessage)
{
    if (vkResult != VK_SUCCESS)
    {
        rLogger.error(fmt::format("Vulkan Error {}: {}", static_cast<int>(vkResult), errorMessage));
    }
}

template <typename T, typename F, typename... Args>
std::vector<T> getVkList(F fct, std::string_view name, Args... args)
{
    uint32_t count{};
    throwIfVkFailed(fct(args..., &count, nullptr), fmt::format("Could not get count for {}", name));

    std::vector<T> result(count);
    throwIfVkFailed(fct(args..., &count, result.data()), fmt::format("Could not get list for {}", name));
    return result;
}

template <class T>
constexpr bool vkFlagsContain(VkFlags vkFlags, T vkFlag)
{
    return (vkFlags & static_cast<VkFlags>(vkFlag)) != 0U;
}

struct FormatProperties
{
    VkDeviceSize sizeInBytes{};
    VkDeviceSize componentSizeInBytes{};
    uint32_t componentCount{};
};
auto getFormatProperties(VkFormat vkFormat) -> FormatProperties;

inline auto toVkExtent3D(const VkExtent2D& rVkExtent2d)
{
    return VkExtent3D{
        .width = rVkExtent2d.width,
        .height = rVkExtent2d.height,
        .depth = 1U,
    };
}

}  // namespace im3e