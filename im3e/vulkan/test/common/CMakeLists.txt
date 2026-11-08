# Helpers shared by the test types of im3e_vulkan: mocks of its private interfaces.
add_library(test_common_im3e_vulkan STATIC
    include/im3e/vulkan/test/mock_vulkan_helper.h
    include/im3e/vulkan/test/mock_vulkan_loader.h
    include/im3e/vulkan/test/mock_vulkan_memory_allocator.h
    src/mock_vulkan_loader.cpp
    src/mock_vulkan_memory_allocator.cpp
)

target_include_directories(test_common_im3e_vulkan
  PUBLIC
    include
    # The mocked private interfaces are declared there: only the tests of im3e_vulkan link this target.
    ../../lib/src
  PRIVATE
    src
)

target_link_libraries(test_common_im3e_vulkan
  PUBLIC
    GTest::gmock
    im3e_test_utils
    im3e_vulkan
    mock_im3e
)

im3e_exclude_from_coverage()
