# The DLLs of Slang aren't copied next to the tool: Slang loads slang-glslang and its standard modules from the folder
# of slang-compiler.dll at run time, so copying slang-compiler.dll alone wouldn't be enough. The build steps run the
# tool with that folder on the PATH instead (see im3e_compile_shaders in cmake/im3e_shaders.cmake), and so do its
# tests. This also keeps the shader build steps from waiting for the DLL copy of every other executable.
im3e_add_executable(
  TARGET
    im3e_shader_compiler
  SOURCES
    im3e_shader_compiler.cpp
  NO_RUNTIME_DLL_COPY
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
