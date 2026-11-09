# Checks that libraries and their mocks follow the naming rules of docs/development/code_standards.md ("Names come
# from paths"). Used by the functions of im3e_libraries.cmake and im3e_tests.cmake.
include_guard(GLOBAL)

# Sets OUT_VAR to the name of the library whose FOLDER_NAME folder (lib or mock) is the current source folder: im3e_
# followed by the path of the library below im3e/, with / replaced by _. im3e/vulkan/lib/ and im3e/vulkan/mock/ give
# im3e_vulkan. Fails if the current source folder is not a FOLDER_NAME folder of a library under im3e/.
function(im3e_internal_get_library_name FOLDER_NAME OUT_VAR)
    cmake_path(SET _IM3E_DIR NORMALIZE "${CMAKE_CURRENT_FUNCTION_LIST_DIR}/../../im3e")
    cmake_path(SET _CURRENT_DIR NORMALIZE "${CMAKE_CURRENT_SOURCE_DIR}")

    cmake_path(IS_PREFIX _IM3E_DIR "${_CURRENT_DIR}" _IS_IN_IM3E)
    cmake_path(GET _CURRENT_DIR FILENAME _CURRENT_FOLDER_NAME)
    if (NOT _IS_IN_IM3E OR NOT _CURRENT_FOLDER_NAME STREQUAL FOLDER_NAME)
        message(FATAL_ERROR
            "Expected a ${FOLDER_NAME}/ folder of a library under im3e/, not ${CMAKE_CURRENT_SOURCE_DIR}")
    endif()

    cmake_path(GET _CURRENT_DIR PARENT_PATH _LIBRARY_DIR)
    cmake_path(RELATIVE_PATH _LIBRARY_DIR BASE_DIRECTORY "${_IM3E_DIR}" OUTPUT_VARIABLE _LIBRARY_PATH)
    string(REPLACE "/" "_" _LIBRARY_NAME "im3e_${_LIBRARY_PATH}")
    set(${OUT_VAR} ${_LIBRARY_NAME} PARENT_SCOPE)
endfunction()

# Fails if the library target is not defined in a lib/ folder under im3e/, or if its name does not match the folder:
# im3e/vulkan/lib/ defines im3e_vulkan.
function(im3e_internal_check_library_name LIBRARY_TARGET)
    im3e_internal_get_library_name(lib _EXPECTED_TARGET)
    if (NOT LIBRARY_TARGET STREQUAL _EXPECTED_TARGET)
        message(FATAL_ERROR
            "Library defined in ${CMAKE_CURRENT_SOURCE_DIR} must be named ${_EXPECTED_TARGET}, not ${LIBRARY_TARGET}")
    endif()
endfunction()

# Fails if the mock target is not defined in a mock/ folder under im3e/, if it does not mock the library of that
# folder, or if its name is not mock_ followed by the name of that library: im3e/vulkan/mock/ defines mock_im3e_vulkan,
# which mocks im3e_vulkan.
function(im3e_internal_check_mock_name MOCK_TARGET MOCKED_TARGET)
    im3e_internal_get_library_name(mock _EXPECTED_MOCKED_TARGET)
    if (NOT MOCKED_TARGET STREQUAL _EXPECTED_MOCKED_TARGET)
        message(FATAL_ERROR
            "Mock defined in ${CMAKE_CURRENT_SOURCE_DIR} must mock ${_EXPECTED_MOCKED_TARGET}, not ${MOCKED_TARGET}")
    endif()
    if (NOT MOCK_TARGET STREQUAL "mock_${_EXPECTED_MOCKED_TARGET}")
        message(FATAL_ERROR
            "Mock defined in ${CMAKE_CURRENT_SOURCE_DIR} must be named mock_${_EXPECTED_MOCKED_TARGET}, not ${MOCK_TARGET}")
    endif()
endfunction()
