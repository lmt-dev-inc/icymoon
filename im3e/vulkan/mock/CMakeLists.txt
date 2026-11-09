im3e_add_mock_library(
  TARGET mock_im3e_vulkan
  MOCKED im3e_vulkan
  SOURCES
    include/im3e/vulkan/mock_command_buffer.h
    include/im3e/vulkan/mock_device.h
    include/im3e/vulkan/mock_image.h
    include/im3e/vulkan/mock_vulkan_functions.h
    src/mock_command_buffer.cpp
    src/mock_device.cpp
    src/mock_image.cpp
    src/mock_vulkan_functions.cpp
)

target_link_libraries(mock_im3e_vulkan
  PUBLIC
    mock_im3e_utils
  PRIVATE
    im3e_test_utils
)
