im3e_add_library(
  TARGET
    im3e_render_vulkan
  SOURCES
    include/im3e/render/vulkan/vulkan_render_engine.h
    src/vulkan_frame_pipeline.cpp
    src/vulkan_frame_pipeline.h
    src/vulkan_render_context.cpp
    src/vulkan_render_context.h
    src/vulkan_render_engine.cpp
)

target_link_libraries(im3e_render_vulkan
  PUBLIC
    im3e_utils
    im3e_vulkan
)
