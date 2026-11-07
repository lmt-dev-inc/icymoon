im3e_add_unit_tests_executable(
  TARGET
    test_im3e_vulkan_internal_loader
  SOURCES
    test_vulkan_loaders.cpp
)

target_include_directories(test_im3e_vulkan_internal_loader
  PRIVATE
    ../../lib/src
)

target_link_libraries(test_im3e_vulkan_internal_loader
  PRIVATE
    fmt::fmt
    im3e_test_utils
    im3e_vulkan_internal_loader
)
