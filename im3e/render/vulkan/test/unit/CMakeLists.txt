im3e_add_unit_tests_executable(
  TARGET
    test_im3e_render_vulkan
  SOURCES
    test_vulkan_render_engine.cpp
)

target_link_libraries(test_im3e_render_vulkan
  PRIVATE
    im3e_render_vulkan
    im3e_test_utils
    mock_im3e_vulkan
    test_common_im3e_render_vulkan
)
