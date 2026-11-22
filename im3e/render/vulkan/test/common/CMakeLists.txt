# Helpers shared by the test types of im3e_render_vulkan: mocks of its private interfaces.
add_library(test_common_im3e_render_vulkan STATIC
    include/im3e/render/vulkan/test/mock_vulkan_render_context.h
    src/mock_vulkan_render_context.cpp
)

target_include_directories(test_common_im3e_render_vulkan
  PUBLIC
    include
    # The mocked private interfaces are declared there: only the tests of im3e_render_vulkan link this target.
    ../../lib/src
  PRIVATE
    src
)

target_link_libraries(test_common_im3e_render_vulkan
  PUBLIC
    GTest::gmock
    im3e_render_vulkan
    im3e_test_utils
    mock_im3e_vulkan
)

im3e_exclude_from_coverage()
