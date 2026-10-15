# Lets the VS Code Test Explorer navigate to each test's source location: see im3e_test_source_lines_script.cmake
function(im3e_internal_add_test_source_lines TARGET)
    get_target_property(_TARGET_SOURCES ${TARGET} SOURCES)
    set(_SOURCES)
    foreach(_SOURCE ${_TARGET_SOURCES})
        if (_SOURCE MATCHES "\\.cpp$")
            cmake_path(ABSOLUTE_PATH _SOURCE BASE_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}")
            list(APPEND _SOURCES "${_SOURCE}")
        endif()
    endforeach()

    set(_INCLUDE_FILE "${CMAKE_CURRENT_BINARY_DIR}/${TARGET}_source_lines.cmake")
    file(CONFIGURE OUTPUT "${_INCLUDE_FILE}" CONTENT
        "set(IM3E_TEST_SOURCES \"${_SOURCES}\")\ninclude(\"${CMAKE_CURRENT_FUNCTION_LIST_DIR}/im3e_test_source_lines_script.cmake\")\n"
    )
    # Must come after the include file appended by gtest_discover_tests(), which defines the tests
    set_property(DIRECTORY APPEND PROPERTY TEST_INCLUDE_FILES "${_INCLUDE_FILE}")
endfunction()
