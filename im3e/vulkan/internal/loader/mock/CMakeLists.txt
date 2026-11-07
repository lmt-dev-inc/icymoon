im3e_add_mock_library(
  TARGET
    mock_im3e_vulkan_internal_loader
  SOURCES
    include/im3e/vulkan/internal/loader/mock_vulkan_loader.h
    src/mock_vulkan_loader.cpp
)

target_include_directories(mock_im3e_vulkan_internal_loader
  PUBLIC
    include
  PRIVATE
    src
)

target_link_libraries(mock_im3e_vulkan_internal_loader
  PUBLIC
    GTest::gmock
    im3e_api
    mock_im3e
    # Only the headers: the tests that use the mock link the loader's owner, which already contains its object files.
    $<COMPILE_ONLY:im3e_vulkan_internal_loader>
  PRIVATE
    im3e_utils
)
