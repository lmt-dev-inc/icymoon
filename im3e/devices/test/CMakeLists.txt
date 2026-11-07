im3e_add_unit_tests_executable(
  TARGET 
    test_im3e_devices
  SOURCES
    mock_vulkan_helper.h
    mock_vulkan_memory_allocator.cpp
    mock_vulkan_memory_allocator.h
    test_vulkan_command_buffer.cpp
    test_vulkan_command_queue.cpp
    test_vulkan_debug_message_handler.cpp
    test_vulkan_extensions.cpp
    test_vulkan_images.cpp
    test_vulkan_instance.cpp
)

target_include_directories(test_im3e_devices
  PRIVATE
    ..
)

target_link_libraries(test_im3e_devices
  PRIVATE
    fmt::fmt
    im3e_devices
    im3e_test_utils
    mock_im3e
    mock_im3e_vulkan_internal_loader
)

add_subdirectory(integration)