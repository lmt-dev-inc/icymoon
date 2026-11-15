# Compiles test_shader.slang with im3e_shader_compiler through SHADERS, then tests the generated code.
im3e_add_unit_tests_executable(
  TARGET
    test_im3e_shader_compiler
  SOURCES
    test_module.slang
    test_shader_compiler.cpp
    test_syntax_error.slang
  SHADERS
    test_shader.slang
)

# Read by the tests of the dependency file
target_compile_definitions(test_im3e_shader_compiler
  PRIVATE
    TEST_SHADER_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}"
    TEST_SHADER_GENERATED_DIR="${CMAKE_CURRENT_BINARY_DIR}/shaders"
)

target_link_libraries(test_im3e_shader_compiler
  PRIVATE
    im3e_shader
    im3e_test_utils
)

# Tests of the tool itself, on invalid inputs and on a successful run. They run under CTest rather than during the
# build, so that the coverage report includes them: the coverage data of the build is deleted before the tests run.
set(_TOOL_TESTS_DIR "${CMAKE_CURRENT_BINARY_DIR}/tool_tests")

function(_im3e_add_shader_compiler_test NAME)
    add_test(NAME ShaderCompilerTool.${NAME} COMMAND ${ARGN})
    set_tests_properties(ShaderCompilerTool.${NAME} PROPERTIES LABELS "unit_test")
    if (WIN32)
        # Same as the launcher of the build step, in cmake/im3e_shaders.cmake
        set_tests_properties(ShaderCompilerTool.${NAME} PROPERTIES
            ENVIRONMENT_MODIFICATION "PATH=path_list_prepend:$<TARGET_RUNTIME_DLL_DIRS:im3e_shader_compiler>"
        )
    endif()
endfunction()

# Runs the tool through expect_compiler_error.cmake, which checks both the exit code and the error
function(_im3e_add_shader_compiler_error_test NAME INPUT FUNCTION EXPECTED_ERROR)
    _im3e_add_shader_compiler_test(${NAME}
        ${CMAKE_COMMAND}
        "-DTOOL=$<TARGET_FILE:im3e_shader_compiler>"
        "-DINPUT=${INPUT}"
        "-DOUTPUT_DIR=${_TOOL_TESTS_DIR}/${NAME}"
        "-DFUNCTION=${FUNCTION}"
        "-DEXPECTED_ERROR=${EXPECTED_ERROR}"
        -P "${CMAKE_CURRENT_SOURCE_DIR}/expect_compiler_error.cmake"
    )
endfunction()

_im3e_add_shader_compiler_error_test(failsOnMissingInput
    "${CMAKE_CURRENT_SOURCE_DIR}/missing.slang" "im3e::getShader" "missing\\.slang: error: file not found"
)
_im3e_add_shader_compiler_error_test(failsOnModuleWithoutEntryPoints
    "${CMAKE_CURRENT_SOURCE_DIR}/test_module.slang" "im3e::getShader" "test_module\\.slang: error: no entry points"
)
# Slang's diagnostic, printed as it is: "--> <file>:<line>:<column>", line 8 being the missing ";"
_im3e_add_shader_compiler_error_test(failsOnSlangError
    "${CMAKE_CURRENT_SOURCE_DIR}/test_syntax_error.slang" "im3e::getShader" "test_syntax_error\\.slang:8:"
)
_im3e_add_shader_compiler_error_test(failsOnInvalidFunctionName
    "${CMAKE_CURRENT_SOURCE_DIR}/test_shader.slang" "1invalid" "not a qualified C\\+\\+ name"
)

# Successful runs, with and without a namespace, which generate different headers, and with debug information. Extra
# arguments are passed to the tool.
function(_im3e_add_shader_compiler_success_test NAME FUNCTION)
    _im3e_add_shader_compiler_test(${NAME}
        $<TARGET_FILE:im3e_shader_compiler>
        "${CMAKE_CURRENT_SOURCE_DIR}/test_shader.slang"
        --header "${_TOOL_TESTS_DIR}/${NAME}/shader.h"
        --source "${_TOOL_TESTS_DIR}/${NAME}/shader.cpp"
        --depfile "${_TOOL_TESTS_DIR}/${NAME}/shader.d"
        --function "${FUNCTION}"
        ${ARGN}
    )
endfunction()

_im3e_add_shader_compiler_success_test(compilesShaderInNamespace "im3e::test::getShader")
_im3e_add_shader_compiler_success_test(compilesShaderInGlobalNamespace "getShader")
_im3e_add_shader_compiler_success_test(compilesShaderWithDebugInformation "im3e::test::getShader" --debug-info)
