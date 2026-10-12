#pragma once

#include <im3e/utils/core/throw_utils.h>
#include <im3e/utils/core/types.h>

#include <vulkan/vulkan.h>
#include <windows.h>

namespace im3e::vulkan_loader {

struct VulkanLibraryLoader
{
    VulkanLibraryLoader()
    {
        constexpr auto VulkanLibraryName{L"vulkan-1.dll"};
        if (HMODULE hModule = LoadLibraryExW(VulkanLibraryName, nullptr, LOAD_LIBRARY_SEARCH_SYSTEM32))
        {
            pLibrary = UniquePtrWithDeleter<void>(hModule,
                                                  [](void* hModule) { FreeLibrary(static_cast<HMODULE>(hModule)); });
        }

        throwIfNull<std::runtime_error>(pLibrary.get(), "Could not load Vulkan shared library file");
        const auto hModule = static_cast<HMODULE>(pLibrary.get());

        vkGetInstanceProcAddr = throwIfNull<std::runtime_error>(
            reinterpret_cast<PFN_vkGetInstanceProcAddr>(_loadFct(hModule, "vkGetInstanceProcAddr")),
            "Failed to load vkGetInstanceProcAddr");

        vkGetDeviceProcAddr = throwIfNull<std::runtime_error>(
            reinterpret_cast<PFN_vkGetDeviceProcAddr>(_loadFct(hModule, "vkGetDeviceProcAddr")),
            "Failed to load vkGetDeviceProcAddr");
    }

    auto _loadFct(HMODULE hModule, LPCSTR fctName) -> void*
    {
        // GetProcAddress returns FARPROC which is already a function pointer. This causes a warning a clang when
        // trying to reinterpret_cast to the actual function pointer type. To avoid this warning, we cast the result
        // to void* first:
        return reinterpret_cast<void*>(GetProcAddress(hModule, fctName));
    }

    UniquePtrWithDeleter<void> pLibrary;
    PFN_vkGetInstanceProcAddr vkGetInstanceProcAddr;
    PFN_vkGetDeviceProcAddr vkGetDeviceProcAddr;
};

}  // namespace im3e::vulkan_loader