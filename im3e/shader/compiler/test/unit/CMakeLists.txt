# Compiles test_shader.slang with im3e_shader_compiler, then tests the generated code.
#
# Written by hand until cmake/im3e_shaders.cmake replaces it (work item 4 of M1 in
# docs/work_in_progress/vulkan_renderer_plan.md). The tool is called as:
#
#   im3e_shader_compiler <input .slang>
#     --header <output .h>
#     --source <output .cpp>
#     --depfile <output dependency file>
#     --function <qualified name of the generated function, e.g. im3e::test::getTestShader>

set(_GENERATED_DIR "${CMAKE_CURRENT_BINARY_DIR}/generated")

set(_SHADER_LAUNCHER "")
if (WIN32)
    # Slang loads slang-glslang and its modules from the folder of slang-compiler.dll. They are found through the PATH
    # rather than copied next to the tool. An empty PATH entry is the current folder on POSIX, hence Windows only:
    # on Linux, the RPATH of the tool finds the library in its package.
    set(_SHADER_LAUNCHER
        ${CMAKE_COMMAND} -E env
        --modify "PATH=path_list_prepend:$<TARGET_RUNTIME_DLL_DIRS:im3e_shader_compiler>"
    )
endif()

add_custom_command(
  OUTPUT
    "${_GENERATED_DIR}/test_shader.h"
    "${_GENERATED_DIR}/test_shader.cpp"
  COMMAND
    ${_SHADER_LAUNCHER}
    $<TARGET_FILE:im3e_shader_compiler>
    "${CMAKE_CURRENT_SOURCE_DIR}/test_shader.slang"
    --header "${_GENERATED_DIR}/test_shader.h"
    --source "${_GENERATED_DIR}/test_shader.cpp"
    --depfile "${_GENERATED_DIR}/test_shader.d"
    --function "im3e::test::getTestShader"
  DEPENDS
    # Also a file-level dependency: the shader is compiled again when the tool changes
    im3e_shader_compiler
    "${CMAKE_CURRENT_SOURCE_DIR}/test_shader.slang"
  DEPFILE
    "${_GENERATED_DIR}/test_shader.d"
  COMMENT
    "Compiling test_shader.slang"
  VERBATIM
)

im3e_add_unit_tests_executable(
  TARGET
    test_im3e_shader_compiler
  SOURCES
    "${_GENERATED_DIR}/test_shader.cpp"
    "${_GENERATED_DIR}/test_shader.h"
    test_shader.slang
    test_shader_compiler.cpp
)

target_include_directories(test_im3e_shader_compiler
  PRIVATE
    "${_GENERATED_DIR}"
)

target_link_libraries(test_im3e_shader_compiler
  PRIVATE
    im3e_shader
    im3e_test_utils
)
