im3e_add_executable(
  TARGET im3e_viewer
  SOURCES
    im3e_viewer.cpp
)

target_link_libraries(im3e_viewer
  PRIVATE
    CLI11::CLI11
    fmt::fmt
    im3e_guis
    im3e_render_vulkan
    im3e_utils
)
