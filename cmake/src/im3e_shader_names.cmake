# Names of the code generated for shaders. Used by cmake/im3e_shaders.cmake.

# Sets OUTPUT_VARIABLE to the qualified name of the function generated for a shader, from the stem of its file name:
# plane.slang gives im3e::shaders::getPlaneShader, and terrain_tile.slang gives im3e::shaders::getTerrainTileShader.
# "Shader" isn't repeated: test_shader.slang gives im3e::shaders::getTestShader.
#
# Fails unless the stem is snake_case, so that it gives a valid C++ name.
function(im3e_internal_shader_function_name STEM OUTPUT_VARIABLE)
    if (NOT STEM MATCHES "^[a-z][a-z0-9]*(_[a-z0-9]+)*$")
        message(FATAL_ERROR "Shader file name \"${STEM}.slang\" must be snake_case, e.g. terrain_tile.slang")
    endif()

    string(REGEX REPLACE "_shader$" "" _NAME "${STEM}")
    string(REPLACE "_" ";" _WORDS "${_NAME}")
    set(_PASCAL_NAME "")
    foreach(_WORD IN LISTS _WORDS)
        string(SUBSTRING "${_WORD}" 0 1 _FIRST)
        string(TOUPPER "${_FIRST}" _FIRST)
        string(SUBSTRING "${_WORD}" 1 -1 _REST)
        string(APPEND _PASCAL_NAME "${_FIRST}${_REST}")
    endforeach()

    set(${OUTPUT_VARIABLE} "im3e::shaders::get${_PASCAL_NAME}Shader" PARENT_SCOPE)
endfunction()

# Records the function name of a shader, and fails if another shader already uses it: both functions would be defined
# in im3e::shaders, and a target linking both libraries would fail to link.
function(im3e_internal_register_shader_function FUNCTION_NAME SHADER_PATH)
    get_property(_EXISTING_PATH GLOBAL PROPERTY "IM3E_SHADER_FUNCTION_${FUNCTION_NAME}")
    if (_EXISTING_PATH)
        message(FATAL_ERROR
            "Shaders \"${SHADER_PATH}\" and \"${_EXISTING_PATH}\" both generate ${FUNCTION_NAME}(): rename one of them")
    endif()
    set_property(GLOBAL PROPERTY "IM3E_SHADER_FUNCTION_${FUNCTION_NAME}" "${SHADER_PATH}")
endfunction()
