# Script run by the im3e_coverage_report target (see im3e_internal_init_llvm_code_coverage()):
# merges the raw profiles written by the test executables and exports them in lcov format.
#
# Usage: cmake -DCONFIG_FILE=<build>/coverage/coverage_config.cmake -P im3e_llvm_coverage_report_script.cmake

include("${CONFIG_FILE}")

file(GLOB PROFRAW_FILES "${PROFILES_DIR}/*.profraw")
if(NOT PROFRAW_FILES)
    message(FATAL_ERROR "No coverage data in ${PROFILES_DIR}: run tests first")
endif()

execute_process(
    COMMAND "${LLVM_PROFDATA}" merge -sparse ${PROFRAW_FILES} -o "${PROFDATA_FILE}"
    COMMAND_ERROR_IS_FATAL ANY
)

# llvm-cov reads its first positional argument as an executable and the next ones as sources to report
list(POP_FRONT TEST_EXECUTABLES OBJECT_ARGS)
foreach(TEST_EXECUTABLE IN LISTS TEST_EXECUTABLES)
    list(APPEND OBJECT_ARGS -object "${TEST_EXECUTABLE}")
endforeach()

# Paths in the coverage data may use either separator
set(IGNORED_FILES_REGEX)
foreach(EXCLUDED_DIR IN LISTS EXCLUDED_DIRS)
    string(REGEX REPLACE "([][.+*?()^$|\\\\{}])" "\\\\\\1" EXCLUDED_DIR_REGEX "${EXCLUDED_DIR}")
    string(REPLACE "/" "[/\\\\]" EXCLUDED_DIR_REGEX "${EXCLUDED_DIR_REGEX}")
    list(APPEND IGNORED_FILES_REGEX "[/\\\\]${EXCLUDED_DIR_REGEX}[/\\\\]")
endforeach()
list(JOIN IGNORED_FILES_REGEX "|" IGNORED_FILES_REGEX)

# Paths in the coverage data keep the case of the drive letter used when CMake was configured (VS Code uses a
# lowercase one). llvm-cov compares paths case-sensitively, and ignores a source filter that matches no file, which
# would report every file. The sources directory is passed with both cases.
set(SOURCE_FILTERS "${SOURCES_DIR}")
if(SOURCES_DIR MATCHES "^([A-Za-z]):(.*)$")
    set(PATH_AFTER_DRIVE "${CMAKE_MATCH_2}")
    string(TOUPPER "${CMAKE_MATCH_1}" UPPER_DRIVE)
    string(TOLOWER "${CMAKE_MATCH_1}" LOWER_DRIVE)
    set(SOURCE_FILTERS "${UPPER_DRIVE}:${PATH_AFTER_DRIVE}" "${LOWER_DRIVE}:${PATH_AFTER_DRIVE}")
endif()

execute_process(
    COMMAND "${LLVM_COV}" export -format=lcov "-instr-profile=${PROFDATA_FILE}" ${OBJECT_ARGS}
        "-ignore-filename-regex=${IGNORED_FILES_REGEX}" ${SOURCE_FILTERS}
    OUTPUT_FILE "${LCOV_FILE}"
    COMMAND_ERROR_IS_FATAL ANY
)
