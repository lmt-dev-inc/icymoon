im3e_add_library(
  TARGET
    im3e_shader
  SOURCES
    include/im3e/shader/compiled_shader.h
    src/compiled_shader.cpp
)

target_link_libraries(im3e_shader
  PRIVATE
    im3e_utils_core
)
