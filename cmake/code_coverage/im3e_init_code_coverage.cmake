if (NOT(WIN32) AND (CMAKE_BUILD_TYPE STREQUAL "Debug" OR GENERATOR_IS_MULTI_CONFIG))
    include(code_coverage/src/code_coverage.cmake)
endif()

option(TEST_COVERAGE "Builds with coverage instrumentation and adds coverage targets")

set(IM3E_COVERAGE_DIR "${CMAKE_BINARY_DIR}/coverage" CACHE INTERNAL "")
set(IM3E_COVERAGE_PROFILES_DIR "${IM3E_COVERAGE_DIR}/profiles" CACHE INTERNAL "")

# Coverage compiler flags, to be added BEFORE creating targets
function(im3e_set_coverage_flags)
    if(NOT TEST_COVERAGE)
        return()
    endif()

    if(CMAKE_COMPILER_IS_GNUCXX)
        append_coverage_compiler_flags()
        # append_coverage_compiler_flags() only sets the flags in this function's scope: forward them to the caller
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS}" PARENT_SCOPE)
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}" PARENT_SCOPE)
        set(CMAKE_Fortran_FLAGS "${CMAKE_Fortran_FLAGS}" PARENT_SCOPE)
    elseif(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
        # %4m makes all runs of the same executable merge their counters into a pool of at most 4 files, instead of
        # one file per process (%p) or each run overwriting the previous one (no pattern). The pool lets parallel runs
        # of the same executable write without waiting on a single file lock.
        add_compile_options(
            "-fprofile-instr-generate=${IM3E_COVERAGE_PROFILES_DIR}/%4m.profraw"
            -fcoverage-mapping
        )
    endif()
endfunction()

function(im3e_init_code_coverage)
    if(NOT TEST_COVERAGE)
        return()
    endif()

    if(CMAKE_COMPILER_IS_GNUCXX)
        get_property(COVERAGE_EXCLUDES GLOBAL PROPERTY COVERAGE_EXCLUDES_PROPERTY)
        set(COVERAGE_EXCLUDES
            ${COVERAGE_EXCLUDES}
            "~/.conan/*"
            "~/.conan2/*"
            "/opt/*"
            "/usr/*"
            "build/*"
            "applications/*"
            "external/*"
            "resources/*"
        )

        setup_target_for_coverage_lcov(
        NAME
            im3e_test_coverage
        EXECUTABLE
            ctest
        DEPENDENCIES
            all
        EXCLUDE
            ${COVERAGE_EXCLUDES}
        )

        setup_target_for_coverage_lcov(
        NAME
            im3e_unit_test_coverage
        EXECUTABLE
            ctest -L "unit_test"
        DEPENDENCIES
            all
        EXCLUDE
            ${COVERAGE_EXCLUDES}
        )
    elseif(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
        im3e_init_llvm_code_coverage()
    endif()
endfunction()

# Targets for clang source-based coverage, run by VS Code before and after running tests with coverage
# (see cmake.preRunCoverageTarget and cmake.postRunCoverageTarget in .vscode/settings.json):
# - im3e_coverage_clean deletes the raw profiles of previous runs
# - im3e_coverage_report merges the raw profiles and exports them to ${IM3E_COVERAGE_DIR}/lcov.info
function(im3e_init_llvm_code_coverage)
    # llvm-profdata and llvm-cov must match the compiler version: look for them next to the compiler first
    cmake_path(GET CMAKE_CXX_COMPILER PARENT_PATH COMPILER_DIR)
    find_program(LLVM_PROFDATA llvm-profdata HINTS "${COMPILER_DIR}" REQUIRED)
    find_program(LLVM_COV llvm-cov HINTS "${COMPILER_DIR}" REQUIRED)

    get_property(TEST_TARGETS GLOBAL PROPERTY IM3E_COVERAGE_TEST_TARGETS)
    set(TEST_EXECUTABLES)
    foreach(TEST_TARGET IN LISTS TEST_TARGETS)
        list(APPEND TEST_EXECUTABLES "$<TARGET_FILE:${TEST_TARGET}>")
    endforeach()

    # Only our code is reported (im3e/), without the directories excluded from coverage (tests and mocks).
    # Excluded directories are made relative so that the report doesn't depend on the case of the drive letter.
    get_property(COVERAGE_EXCLUDES GLOBAL PROPERTY COVERAGE_EXCLUDES_PROPERTY)
    set(EXCLUDED_DIRS)
    foreach(COVERAGE_EXCLUDE IN LISTS COVERAGE_EXCLUDES)
        string(REGEX REPLACE "/\\*$" "" EXCLUDED_DIR "${COVERAGE_EXCLUDE}")
        cmake_path(RELATIVE_PATH EXCLUDED_DIR BASE_DIRECTORY "${CMAKE_SOURCE_DIR}")
        list(APPEND EXCLUDED_DIRS "${EXCLUDED_DIR}")
    endforeach()

    set(CONFIG_FILE "${IM3E_COVERAGE_DIR}/coverage_config.cmake")
    file(GENERATE OUTPUT "${CONFIG_FILE}" CONTENT "\
set(LLVM_PROFDATA \"${LLVM_PROFDATA}\")
set(LLVM_COV \"${LLVM_COV}\")
set(PROFILES_DIR \"${IM3E_COVERAGE_PROFILES_DIR}\")
set(PROFDATA_FILE \"${IM3E_COVERAGE_DIR}/coverage.profdata\")
set(LCOV_FILE \"${IM3E_COVERAGE_DIR}/lcov.info\")
set(TEST_EXECUTABLES \"${TEST_EXECUTABLES}\")
set(SOURCES_DIR \"${CMAKE_SOURCE_DIR}/im3e\")
set(EXCLUDED_DIRS \"${EXCLUDED_DIRS}\")
")

    add_custom_target(im3e_coverage_clean
        COMMAND ${CMAKE_COMMAND} -E rm -rf "${IM3E_COVERAGE_PROFILES_DIR}"
        COMMENT "Deleting coverage data of previous runs"
        VERBATIM
    )

    add_custom_target(im3e_coverage_report
        COMMAND ${CMAKE_COMMAND} -DCONFIG_FILE=${CONFIG_FILE}
            -P "${CMAKE_CURRENT_FUNCTION_LIST_DIR}/im3e_llvm_coverage_report.cmake"
        COMMENT "Generating coverage report ${IM3E_COVERAGE_DIR}/lcov.info"
        VERBATIM
    )
endfunction()
