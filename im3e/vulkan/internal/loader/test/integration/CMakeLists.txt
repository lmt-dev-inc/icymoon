im3e_add_integration_tests_executable(
  TARGET
    integration_im3e_vulkan_internal_loader
  SOURCES
    integration_vulkan_loaders.cpp
)

target_link_libraries(integration_im3e_vulkan_internal_loader
  PRIVATE
    im3e_test_utils
    im3e_vulkan_internal_loader
)
