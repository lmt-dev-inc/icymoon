add_executable(im3e_shader_compiler
    im3e_shader_compiler.cpp
)

target_link_libraries(im3e_shader_compiler
  PRIVATE
    fmt::fmt
    im3e_utils
    slang::slang
)
