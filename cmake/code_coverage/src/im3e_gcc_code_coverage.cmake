# Targets for gcc coverage, run by VS Code before and after running tests with coverage
# (see cmake.preRunCoverageTarget and cmake.postRunCoverageTarget in .vscode/settings.json):
# - im3e_coverage_clean deletes the counters of previous runs
# - im3e_coverage_report exports the counters to ${IM3E_COVERAGE_DIR}/lcov.info
# - im3e_coverage_html generates an HTML report in ${IM3E_COVERAGE_DIR}/html (not run by VS Code, to keep it fast)
function(im3e_internal_init_gcc_code_coverage)
    # gcov must match the compiler version, and plain gcov may belong to a newer gcc installed on the system
    string(REGEX MATCH "^[0-9]+" GCC_MAJOR_VERSION "${CMAKE_CXX_COMPILER_VERSION}")
    cmake_path(GET CMAKE_CXX_COMPILER PARENT_PATH COMPILER_DIR)
    find_program(GCOV_EXECUTABLE "gcov-${GCC_MAJOR_VERSION}" HINTS "${COMPILER_DIR}" REQUIRED)
    message(STATUS "Coverage uses ${GCOV_EXECUTABLE}")

    # Installed by conanfile.py (system_requirements) for the coverage configurations
    find_program(LCOV_EXECUTABLE lcov REQUIRED)
    find_program(GENHTML_EXECUTABLE genhtml REQUIRED)

    # Only our code is reported (im3e/), without the directories excluded from coverage (tests and mocks)
    get_property(COVERAGE_EXCLUDES GLOBAL PROPERTY COVERAGE_EXCLUDES_PROPERTY)
    set(EXCLUDE_ARGS)
    foreach(COVERAGE_EXCLUDE IN LISTS COVERAGE_EXCLUDES)
        list(APPEND EXCLUDE_ARGS --exclude "${COVERAGE_EXCLUDE}")
    endforeach()

    set(LCOV_FILE "${IM3E_COVERAGE_DIR}/lcov.info")

    # lcov stops at these checks by default, and each name is given twice to also silence its warnings:
    # - mismatch and inconsistent: instantiations of a template don't all have the same branches on a line
    # - unused: excluded directories have no data when the tests they hold didn't run
    set(IGNORE_ERRORS_ARGS --ignore-errors mismatch,mismatch,inconsistent,inconsistent,unused,unused)

    add_custom_target(im3e_coverage_clean
        COMMAND find "${CMAKE_BINARY_DIR}" -name "*.gcda" -delete
        COMMENT "Deleting coverage data of previous runs"
        VERBATIM
    )

    add_custom_target(im3e_coverage_report
        COMMAND ${CMAKE_COMMAND} -E make_directory "${IM3E_COVERAGE_DIR}"
        COMMAND "${LCOV_EXECUTABLE}"
            --capture
            --directory "${CMAKE_BINARY_DIR}"
            --gcov-tool "${GCOV_EXECUTABLE}"
            --rc branch_coverage=1
            --include "${CMAKE_SOURCE_DIR}/im3e/*"
            ${EXCLUDE_ARGS}
            ${IGNORE_ERRORS_ARGS}
            --parallel
            --all
            --quiet
            --output-file "${LCOV_FILE}"
        COMMAND "${LCOV_EXECUTABLE}" --summary "${LCOV_FILE}" --rc branch_coverage=1 ${IGNORE_ERRORS_ARGS}
        COMMENT "Generating coverage report ${LCOV_FILE}"
        VERBATIM
    )

    add_custom_target(im3e_coverage_html
        COMMAND "${GENHTML_EXECUTABLE}" "${LCOV_FILE}"
            --output-directory "${IM3E_COVERAGE_DIR}/html"
            --branch-coverage
            ${IGNORE_ERRORS_ARGS}
            --quiet
        COMMENT "Generating HTML coverage report in ${IM3E_COVERAGE_DIR}/html"
        VERBATIM
    )
    add_dependencies(im3e_coverage_html im3e_coverage_report)
endfunction()
