# Checks that a library follows the naming rules of docs/development/code_standards.md ("Names come from paths").
# Used by the functions of im3e_libraries.cmake.

# Fails if the library target is not defined in a lib/ folder under im3e/, or if its name is not im3e_ followed by the
# path of the library below im3e/, with / replaced by _: im3e/vulkan/lib/ defines im3e_vulkan.
function(im3e_internal_check_library_name LIBRARY_TARGET)
    cmake_path(SET _IM3E_DIR NORMALIZE "${CMAKE_CURRENT_FUNCTION_LIST_DIR}/../../im3e")
    cmake_path(SET _CURRENT_DIR NORMALIZE "${CMAKE_CURRENT_SOURCE_DIR}")

    cmake_path(IS_PREFIX _IM3E_DIR "${_CURRENT_DIR}" _IS_IN_IM3E)
    cmake_path(GET _CURRENT_DIR FILENAME _FOLDER_NAME)
    if (NOT _IS_IN_IM3E OR NOT _FOLDER_NAME STREQUAL "lib")
        message(FATAL_ERROR
            "Library ${LIBRARY_TARGET} must be defined in a lib/ folder under im3e/, not in ${CMAKE_CURRENT_SOURCE_DIR}")
    endif()

    cmake_path(GET _CURRENT_DIR PARENT_PATH _LIBRARY_DIR)
    cmake_path(RELATIVE_PATH _LIBRARY_DIR BASE_DIRECTORY "${_IM3E_DIR}" OUTPUT_VARIABLE _LIBRARY_PATH)
    string(REPLACE "/" "_" _EXPECTED_TARGET "im3e_${_LIBRARY_PATH}")
    if (NOT LIBRARY_TARGET STREQUAL _EXPECTED_TARGET)
        message(FATAL_ERROR
            "Library defined in im3e/${_LIBRARY_PATH}/lib/ must be named ${_EXPECTED_TARGET}, not ${LIBRARY_TARGET}")
    endif()
endfunction()
