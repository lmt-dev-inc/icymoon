# Project built by incremental_build_script.cmake, to test that a shader compiles again only when a file it reads
# changes. It has no C++ language, as shaders must build without a C++ target. IM3E_CMAKE_DIR, IM3E_SHADER_COMPILER and
# IM3E_SHADER_COMPILER_DLL_DIRS are set by the script.
cmake_minimum_required(VERSION 4.2.3)

project(IncrementalShaderBuild LANGUAGES NONE)

include("${IM3E_CMAKE_DIR}/im3e_shaders.cmake")

im3e_compile_shaders(
  SHADERS
    importing_shader.slang
    standalone_shader.slang
  OUTPUT_DIR
    "${CMAKE_CURRENT_BINARY_DIR}/shaders"
  GENERATED_SOURCES
    _GENERATED_SOURCES
)

add_custom_target(shaders ALL
  DEPENDS
    ${_GENERATED_SOURCES}
)
