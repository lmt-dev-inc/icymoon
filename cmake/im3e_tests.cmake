include("${CMAKE_CURRENT_LIST_DIR}/src/im3e_test_source_lines.cmake")

set_property(GLOBAL PROPERTY COVERAGE_EXCLUDES_PROPERTY "")

macro(im3e_exclude_from_coverage)
    get_property(_COVERAGE_EXCLUDES GLOBAL PROPERTY COVERAGE_EXCLUDES_PROPERTY)
    set(_COVERAGE_EXCLUDES ${_COVERAGE_EXCLUDES} "${CMAKE_CURRENT_SOURCE_DIR}/*")
    set_property(GLOBAL PROPERTY COVERAGE_EXCLUDES_PROPERTY ${_COVERAGE_EXCLUDES})
endmacro()

function(im3e_add_unit_tests_executable)
    set(options)
    set(oneValueArgs TARGET)
    set(multiValueArgs SOURCES)
    cmake_parse_arguments(ARG
        "${options}" 
        "${oneValueArgs}"
        "${multiValueArgs}" 
        ${ARGN}
    )

    if (NOT ARG_TARGET OR NOT ARG_SOURCES)
        message(FATAL_ERROR "Missing arguments to im3e_add_unit_tests_executable")
    endif()

    if (ARG_UNPARSED_ARGUMENTS)
        message(FATAL_ERROR "Invalid arguments passed to im3e_add_unit_tests_executable")
    endif()

    add_executable(${ARG_TARGET} ${ARG_SOURCES})
    set_property(GLOBAL APPEND PROPERTY IM3E_COVERAGE_TEST_TARGETS ${ARG_TARGET})

    gtest_discover_tests(${ARG_TARGET}
      NO_PRETTY_VALUES
      PROPERTIES 
        LABELS "unit_test"
    )
    im3e_internal_add_test_source_lines(${ARG_TARGET})

    im3e_exclude_from_coverage()
endfunction()

function(im3e_add_integration_tests_executable)
    set(options)
    set(oneValueArgs TARGET)
    set(multiValueArgs SOURCES)
    cmake_parse_arguments(ARG
        "${options}" 
        "${oneValueArgs}"
        "${multiValueArgs}" 
        ${ARGN}
    )

    if (NOT ARG_TARGET OR NOT ARG_SOURCES)
        message(FATAL_ERROR "Missing arguments to im3e_add_integration_tests_executable")
    endif()

    if (ARG_UNPARSED_ARGUMENTS)
        message(FATAL_ERROR "Invalid arguments passed to im3e_add_integration_tests_executable")
    endif()

    add_executable(${ARG_TARGET} ${ARG_SOURCES})
    set_property(GLOBAL APPEND PROPERTY IM3E_COVERAGE_TEST_TARGETS ${ARG_TARGET})

    gtest_discover_tests(${ARG_TARGET}
      NO_PRETTY_VALUES
      PROPERTIES 
        LABELS "integration_test"
    )
    im3e_internal_add_test_source_lines(${ARG_TARGET})

    im3e_exclude_from_coverage()
endfunction()

function(im3e_add_mock_library)
    set(options)
    set(oneValueArgs TARGET)
    set(multiValueArgs SOURCES)
    cmake_parse_arguments(ARG
        "${options}" 
        "${oneValueArgs}"
        "${multiValueArgs}" 
        ${ARGN}
    )

    if (NOT ARG_TARGET OR NOT ARG_SOURCES)
        message(FATAL_ERROR "Missing arguments to im3e_add_mock_library")
    endif()

    if (ARG_UNPARSED_ARGUMENTS)
        message(FATAL_ERROR "Invalid arguments passed to im3e_add_mock_library")
    endif()

    add_library(${ARG_TARGET} STATIC ${ARG_SOURCES})

    im3e_exclude_from_coverage()
endfunction()
