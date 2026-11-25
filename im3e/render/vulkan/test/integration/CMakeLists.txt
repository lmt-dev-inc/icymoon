im3e_add_integration_tests_executable(
  TARGET
    integration_im3e_render_vulkan
  SOURCES
    integration_vulkan_frame_pipeline.cpp
)

target_link_libraries(integration_im3e_render_vulkan
  PRIVATE
    im3e_render_vulkan
    im3e_test_utils
)