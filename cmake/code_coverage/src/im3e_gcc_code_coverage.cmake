# Targets for gcc coverage, run by VS Code before and after running tests with coverage
# (see cmake.preRunCoverageTarget and cmake.postRunCoverageTarget in .vscode/settings.json):
# - im3e_coverage_clean deletes the counters of previous runs
# - im3e_coverage_report exports the counters to ${IM3E_COVERAGE_DIR}/lcov.info and ${IM3E_COVERAGE_DIR}/index.html
function(im3e_internal_init_gcc_code_coverage)
    # gcov must match the compiler version, and plain gcov may belong to a newer gcc installed on the system
    string(REGEX MATCH "^[0-9]+" GCC_MAJOR_VERSION "${CMAKE_CXX_COMPILER_VERSION}")
    cmake_path(GET CMAKE_CXX_COMPILER PARENT_PATH COMPILER_DIR)
    find_program(GCOV_EXECUTABLE "gcov-${GCC_MAJOR_VERSION}" HINTS "${COMPILER_DIR}" REQUIRED)
    message(STATUS "Coverage uses ${GCOV_EXECUTABLE}")

    # .venv isn't on the PATH when VS Code configures
    find_program(GCOVR_EXECUTABLE gcovr HINTS "${CMAKE_SOURCE_DIR}/.venv/bin" REQUIRED)

    # Only our code is reported (im3e/), without the directories excluded from coverage (tests and mocks)
    get_property(COVERAGE_EXCLUDES GLOBAL PROPERTY COVERAGE_EXCLUDES_PROPERTY)
    set(EXCLUDE_ARGS)
    foreach(COVERAGE_EXCLUDE IN LISTS COVERAGE_EXCLUDES)
        string(REGEX REPLACE "/\\*$" "" EXCLUDED_DIR "${COVERAGE_EXCLUDE}")
        string(REGEX REPLACE "([][.+*?()^$|\\\\{}])" "\\\\\\1" EXCLUDED_DIR_REGEX "${EXCLUDED_DIR}")
        list(APPEND EXCLUDE_ARGS --exclude "${EXCLUDED_DIR_REGEX}/")
    endforeach()
    string(REGEX REPLACE "([][.+*?()^$|\\\\{}])" "\\\\\\1" SOURCES_DIR_REGEX "${CMAKE_SOURCE_DIR}/im3e/")

    add_custom_target(im3e_coverage_clean
        COMMAND find "${CMAKE_BINARY_DIR}" -name "*.gcda" -delete
        COMMENT "Deleting coverage data of previous runs"
        VERBATIM
    )

    # --lcov-format-1.x: CMake Tools can't parse the VER records of the 2.x format
    add_custom_target(im3e_coverage_report
        COMMAND ${CMAKE_COMMAND} -E make_directory "${IM3E_COVERAGE_DIR}"
        COMMAND "${GCOVR_EXECUTABLE}"
            --root "${CMAKE_SOURCE_DIR}"
            --filter "${SOURCES_DIR_REGEX}"
            ${EXCLUDE_ARGS}
            --gcov-executable "${GCOV_EXECUTABLE}"
            --merge-lines
            --lcov "${IM3E_COVERAGE_DIR}/lcov.info"
            --lcov-format-1.x
            --html-details "${IM3E_COVERAGE_DIR}/index.html"
            -j
            --print-summary
            "${CMAKE_BINARY_DIR}"
        WORKING_DIRECTORY "${CMAKE_BINARY_DIR}"
        COMMENT "Generating coverage report ${IM3E_COVERAGE_DIR}/lcov.info"
        VERBATIM
    )
endfunction()
