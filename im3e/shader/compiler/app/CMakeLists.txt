add_executable(im3e_shader_compiler
    im3e_shader_compiler.cpp
)

target_link_libraries(im3e_shader_compiler
  PRIVATE
    CLI11::CLI11
    fmt::fmt
    im3e_utils_core
    slang::slang
)
