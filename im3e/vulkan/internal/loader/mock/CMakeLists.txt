im3e_add_mock_library(
  TARGET mock_im3e_vulkan_internal_loader
  MOCKED im3e_vulkan_internal_loader
  SOURCES
    include/im3e/vulkan/internal/loader/mock_vulkan_loader.h
    src/mock_vulkan_loader.cpp
)

target_link_libraries(mock_im3e_vulkan_internal_loader
  PUBLIC
    im3e_api
    mock_im3e
  PRIVATE
    im3e_utils
)
