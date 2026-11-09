include("${CMAKE_CURRENT_LIST_DIR}/src/im3e_library_names.cmake")

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
    im3e_copy_runtime_dlls(${ARG_TARGET})
    set_property(GLOBAL APPEND PROPERTY IM3E_COVERAGE_TEST_TARGETS ${ARG_TARGET})

    gtest_discover_tests(${ARG_TARGET}
      NO_PRETTY_VALUES
      PROPERTIES
        LABELS "unit_test"
    )

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
    im3e_copy_runtime_dlls(${ARG_TARGET})
    set_property(GLOBAL APPEND PROPERTY IM3E_COVERAGE_TEST_TARGETS ${ARG_TARGET})

    gtest_discover_tests(${ARG_TARGET}
      NO_PRETTY_VALUES
      PROPERTIES
        LABELS "integration_test"
    )

    im3e_exclude_from_coverage()
endfunction()

# Creates a mock library: a STATIC library of mocks for the interfaces of the library it mocks.
#
#   im3e_add_mock_library(
#     TARGET <target>
#     [MOCKED <mocked library target>]
#     SOURCES <sources>...
#   )
#
# With MOCKED, for libraries that follow the layout of docs/development/code_standards.md ("Library folders"):
# - the names must match the folder: im3e/vulkan/mock/ defines mock_im3e_vulkan, which mocks im3e_vulkan;
# - the mock exposes its include/ folder to the targets that link it, and keeps src/ private;
# - the mock links GoogleMock and the mocked library. An internal library is linked with $<COMPILE_ONLY:...>, for its
#   headers only: the tests that use the mock link the internal library's owner, which already contains its object
#   files, and a target that gets the same object files twice can end up with duplicate symbols.
# Without MOCKED, the caller sets the include directories and links the libraries itself.
function(im3e_add_mock_library)
    set(options)
    set(oneValueArgs TARGET MOCKED)
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

    if (ARG_MOCKED)
        im3e_internal_check_mock_name(${ARG_TARGET} ${ARG_MOCKED})

        if (NOT TARGET ${ARG_MOCKED})
            message(FATAL_ERROR "Library ${ARG_MOCKED} mocked by ${ARG_TARGET} is not defined: add its folder before mock/")
        endif()
    endif()

    add_library(${ARG_TARGET} STATIC ${ARG_SOURCES})

    if (ARG_MOCKED)
        target_include_directories(${ARG_TARGET}
          PUBLIC
            include
          PRIVATE
            src
        )

        get_target_property(_INTERNAL_OWNER ${ARG_MOCKED} IM3E_INTERNAL_OWNER)
        if (_INTERNAL_OWNER)
            set(_MOCKED_LINK $<COMPILE_ONLY:${ARG_MOCKED}>)
        else()
            set(_MOCKED_LINK ${ARG_MOCKED})
        endif()

        target_link_libraries(${ARG_TARGET}
          PUBLIC
            GTest::gmock
            ${_MOCKED_LINK}
        )
    endif()

    im3e_exclude_from_coverage()
endfunction()
