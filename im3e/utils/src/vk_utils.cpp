#include "vk_utils.h"

#define VMA_IMPLEMENTATION
#include "vk_mem_alloc.h"

#include <fmt/format.h>

using namespace im3e;

auto im3e::getFormatProperties(VkFormat vkFormat) -> FormatProperties
{
    switch (vkFormat)
    {
        case VK_FORMAT_B8G8R8_SRGB:
        case VK_FORMAT_B8G8R8_UNORM:
        case VK_FORMAT_B8G8R8_SNORM:
        case VK_FORMAT_R8G8B8_SRGB:
        case VK_FORMAT_R8G8B8_UNORM:
        case VK_FORMAT_R8G8B8_SNORM:
            return FormatProperties{.sizeInBytes = 3U, .componentSizeInBytes = 1U, .componentCount = 3U};

        case VK_FORMAT_B8G8R8A8_SRGB:
        case VK_FORMAT_B8G8R8A8_UNORM:
        case VK_FORMAT_B8G8R8A8_SNORM:
        case VK_FORMAT_R8G8B8A8_SRGB:
        case VK_FORMAT_R8G8B8A8_UNORM:
        case VK_FORMAT_R8G8B8A8_SNORM:
            return FormatProperties{.sizeInBytes = 4U, .componentSizeInBytes = 1U, .componentCount = 4U};

        case VK_FORMAT_R32G32_SFLOAT:
            return FormatProperties{.sizeInBytes = 8U, .componentSizeInBytes = 4U, .componentCount = 2U};
        case VK_FORMAT_R32G32B32_SFLOAT:
            return FormatProperties{.sizeInBytes = 12U, .componentSizeInBytes = 4U, .componentCount = 3U};
        case VK_FORMAT_R32G32B32A32_SFLOAT:
            return FormatProperties{.sizeInBytes = 16U, .componentSizeInBytes = 4U, .componentCount = 4U};
        case VK_FORMAT_BC1_RGB_SRGB_BLOCK:
            return FormatProperties{.sizeInBytes = 0U, .componentSizeInBytes = 0U, .componentCount = 0U};
        default:
            throw std::invalid_argument(
                fmt::format("Could not get VkFormat properties for {}: not supported", static_cast<int>(vkFormat)));
    }
}