# Targets for clang source-based coverage, run by VS Code before and after running tests with coverage
# (see cmake.preRunCoverageTarget and cmake.postRunCoverageTarget in .vscode/settings.json):
# - im3e_coverage_clean deletes the raw profiles of previous runs
# - im3e_coverage_report merges the raw profiles and exports them to ${IM3E_COVERAGE_DIR}/lcov.info
function(im3e_internal_init_llvm_code_coverage)
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
            -P "${CMAKE_CURRENT_FUNCTION_LIST_DIR}/im3e_llvm_coverage_report_script.cmake"
        COMMENT "Generating coverage report ${IM3E_COVERAGE_DIR}/lcov.info"
        VERBATIM
    )
endfunction()
