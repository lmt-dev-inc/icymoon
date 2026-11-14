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

# Reported with the tests, from its runs under CTest (see im3e/shader/compiler/test/unit/). Its runs during the build
# aren't reported, as the coverage data of the build is deleted before the tests run.
set_property(GLOBAL APPEND PROPERTY IM3E_COVERAGE_TEST_TARGETS im3e_shader_compiler)
