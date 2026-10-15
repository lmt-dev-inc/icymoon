# CTest-time script (added to TEST_INCLUDE_FILES by im3e_tests.cmake): sets the DEF_SOURCE_LINE property of each
# GTest test to the location of its TEST/TEST_F macro, which the VS Code Test Explorer uses to navigate to the test.
# Without it, CMake Tools points every test of an executable to the first line of its first source file.
# Expects IM3E_TEST_SOURCES to contain the absolute paths of the test executable source files.
foreach(_source ${IM3E_TEST_SOURCES})
    if (NOT EXISTS "${_source}")
        continue()
    endif()
    file(READ "${_source}" _content)
    string(REGEX MATCHALL
        "(^|[^A-Za-z0-9_])TEST(_F)?[ \t]*\\([ \t\r\n]*[A-Za-z0-9_]+[ \t\r\n]*,[ \t\r\n]*[A-Za-z0-9_]+[ \t\r\n]*\\)"
        _test_macros "${_content}")
    foreach(_test_macro ${_test_macros})
        string(REGEX REPLACE ".*\\([ \t\r\n]*([A-Za-z0-9_]+)[ \t\r\n]*,[ \t\r\n]*([A-Za-z0-9_]+).*" "\\1.\\2"
            _test_name "${_test_macro}")

        # Line number = 1 + number of line feeds before the macro
        string(FIND "${_content}" "${_test_macro}" _offset)
        if (NOT _test_macro MATCHES "^TEST")
            # Skip the character preceding the macro, which may be the previous line's line feed
            math(EXPR _offset "${_offset} + 1")
        endif()
        string(SUBSTRING "${_content}" 0 ${_offset} _before)
        string(REGEX REPLACE "[^\n]" "" _line_feeds "${_before}")
        string(LENGTH "${_line_feeds}" _line)
        math(EXPR _line "${_line} + 1")

        set_tests_properties("${_test_name}" PROPERTIES DEF_SOURCE_LINE "${_source}:${_line}")
    endforeach()
endforeach()
