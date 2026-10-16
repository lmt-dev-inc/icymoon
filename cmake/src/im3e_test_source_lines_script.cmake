# CTest-time script (added to TEST_INCLUDE_FILES by im3e_tests.cmake): sets the DEF_SOURCE_LINE property of each
# GTest test to the location of its TEST/TEST_F macro, which the VS Code Test Explorer uses to navigate to the test.
# Without it, CMake Tools points every test of an executable to the first line of its first source file.
# Expects IM3E_TEST_SOURCES to contain the absolute paths of the test executable source files.
foreach(_source ${IM3E_TEST_SOURCES})
    if (NOT EXISTS "${_source}")
        continue()
    endif()
    file(READ "${_source}" _content)

    # Blank out comments so that commented-out tests are ignored, keeping their line feeds so line numbers are unchanged
    string(REGEX REPLACE "//[^\n]*" "" _content "${_content}")
    string(FIND "${_content}" "/*" _start)
    while(NOT _start EQUAL -1)
        string(SUBSTRING "${_content}" 0 ${_start} _before)
        string(SUBSTRING "${_content}" ${_start} -1 _comment)
        string(FIND "${_comment}" "*/" _end)
        if (_end EQUAL -1)
            set(_after "")
        else()
            math(EXPR _end "${_end} + 2")
            string(SUBSTRING "${_comment}" ${_end} -1 _after)
            string(SUBSTRING "${_comment}" 0 ${_end} _comment)
        endif()
        string(REGEX REPLACE "[^\n]" "" _comment "${_comment}")
        set(_content "${_before}${_comment}${_after}")
        string(FIND "${_content}" "/*" _start)
    endwhile()

    # Scan the macros in order, counting line feeds incrementally: line number = 1 + number of line feeds before it
    set(_regex
        "(^|[^A-Za-z0-9_])TEST(_F)?[ \t]*\\([ \t\r\n]*[A-Za-z0-9_]+[ \t\r\n]*,[ \t\r\n]*[A-Za-z0-9_]+[ \t\r\n]*\\)")
    set(_line 1)
    string(REGEX MATCH "${_regex}" _test_macro "${_content}")
    while(_test_macro)
        string(REGEX REPLACE ".*\\([ \t\r\n]*([A-Za-z0-9_]+)[ \t\r\n]*,[ \t\r\n]*([A-Za-z0-9_]+).*" "\\1.\\2"
            _test_name "${_test_macro}")

        # The first occurrence of the matched text is the match itself: any earlier copy would have matched first
        string(FIND "${_content}" "${_test_macro}" _offset)
        if (NOT _test_macro MATCHES "^TEST")
            # Skip the character preceding the macro, which may be the previous line's line feed
            math(EXPR _offset "${_offset} + 1")
        endif()
        string(SUBSTRING "${_content}" 0 ${_offset} _before)
        string(REGEX REPLACE "[^\n]" "" _line_feeds "${_before}")
        string(LENGTH "${_line_feeds}" _line_feed_count)
        math(EXPR _line "${_line} + ${_line_feed_count}")

        set_tests_properties("${_test_name}" PROPERTIES DEF_SOURCE_LINE "${_source}:${_line}")

        # Continue right after the macro's first character, so that the same macro cannot match again
        math(EXPR _offset "${_offset} + 1")
        string(SUBSTRING "${_content}" ${_offset} -1 _content)
        string(REGEX MATCH "${_regex}" _test_macro "${_content}")
    endwhile()
endforeach()
