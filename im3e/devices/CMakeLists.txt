add_library(im3e_devices STATIC
    devices.h
    src/vulkan_command_buffer.cpp
    src/vulkan_command_buffer.h
    src/vulkan_command_queue.cpp
    src/vulkan_command_queue.h
    src/vulkan_debug_message_handler.cpp
    src/vulkan_debug_message_handler.h
    src/vulkan_device.cpp
    src/vulkan_device.h
    src/vulkan_extensions.cpp
    src/vulkan_extensions.h
    src/vulkan_images.cpp
    src/vulkan_images.h
    src/vulkan_instance.cpp
    src/vulkan_instance.h
    src/vulkan_memory_allocator.cpp
    src/vulkan_memory_allocator.h
    src/vulkan_physical_devices.cpp
    src/vulkan_physical_devices.h
)

target_include_directories(im3e_devices
  PUBLIC
    ${IM3E_INCLUDE_DIR}
  PRIVATE
    .
    src
)

target_link_libraries(im3e_devices
  PUBLIC
    im3e_api
  PRIVATE
    cimg::cimg
    im3e_utils
)

add_subdirectory(test)