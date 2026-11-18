#pragma once

#include <im3e/test_utils/test_utils.h>
#include <im3e/vulkan/mock_vulkan_functions.h>

#include <algorithm>
#include <cstdint>
#include <vector>

namespace im3e {

/// @brief Implement the two-call pattern of Vulkan enumeration functions in mocks: return the count when items is
/// null, and copy up to count items otherwise.
/// @return VK_INCOMPLETE when count is smaller than the list, as Vulkan does, VK_SUCCESS otherwise.
template <class T>
auto fillVkList(const std::vector<T>& list, uint32_t* count, T* items) -> VkResult
{
    const auto size = static_cast<uint32_t>(list.size());
    if (items == nullptr)
    {
        *count = size;
        return VK_SUCCESS;
    }
    const auto copiedCount = std::min(*count, size);
    std::copy_n(list.begin(), copiedCount, items);
    *count = copiedCount;
    return copiedCount < size ? VK_INCOMPLETE : VK_SUCCESS;
}

inline void expectInstanceExtensionsEnumerated(MockVulkanGlobalFcts& rMockFcts, bool isDebugEnabled)
{
    std::vector<VkExtensionProperties> extensions{VkExtensionProperties{
        .extensionName = VK_KHR_SURFACE_EXTENSION_NAME,
        .specVersion = 1U,
    }};
    if (isDebugEnabled)
    {
        extensions.emplace_back(VkExtensionProperties{
            .extensionName = VK_EXT_DEBUG_UTILS_EXTENSION_NAME,
            .specVersion = 1U,
        });
    }

    InSequence s;
    EXPECT_CALL(rMockFcts, vkEnumerateInstanceExtensionProperties(IsNull(), NotNull(), IsNull()))
        .WillOnce(DoAll(SetArgPointee<1>(extensions.size()), Return(VK_SUCCESS)));
    EXPECT_CALL(rMockFcts, vkEnumerateInstanceExtensionProperties(IsNull(), NotNull(), NotNull()))
        .WillOnce(Invoke([extensions](Unused, auto* pPropertyCount, auto* pProperties) {
            *pPropertyCount = static_cast<uint32_t>(extensions.size());
            std::ranges::copy(extensions, pProperties);
            return VK_SUCCESS;
        }));
}

}  // namespace im3e