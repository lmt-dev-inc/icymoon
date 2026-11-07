# Owned by im3e_devices until the device code moves into im3e_vulkan: the owner will then be im3e_vulkan.
im3e_add_internal_library(
  TARGET
    im3e_vulkan_internal_loader
  OWNER
    im3e_devices
  SOURCES
    include/im3e/vulkan/internal/loader/vulkan_loaders.h
    src/vulkan_loader.cpp
    src/vulkan_loader.h
    $<$<PLATFORM_ID:Linux>:src/vulkan_library_loader_linux.h>
    $<$<PLATFORM_ID:Windows>:src/vulkan_library_loader_windows.h>
)

target_link_libraries(im3e_vulkan_internal_loader
  PUBLIC
    im3e_api
    Vulkan::Headers
  PRIVATE
    fmt::fmt
)
