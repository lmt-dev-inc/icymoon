include("${CMAKE_CURRENT_LIST_DIR}/src/im3e_gcc_code_coverage.cmake")
include("${CMAKE_CURRENT_LIST_DIR}/src/im3e_llvm_code_coverage.cmake")

option(TEST_COVERAGE "Builds with coverage instrumentation and adds coverage targets")

set(IM3E_COVERAGE_DIR "${CMAKE_BINARY_DIR}/coverage" CACHE INTERNAL "")
set(IM3E_COVERAGE_PROFILES_DIR "${IM3E_COVERAGE_DIR}/profiles" CACHE INTERNAL "")

# Coverage compiler flags, to be added BEFORE creating targets
function(im3e_set_coverage_flags)
    if(NOT TEST_COVERAGE)
        return()
    endif()

    if(CMAKE_COMPILER_IS_GNUCXX)
        # Optimizations inline and remove lines, which makes the percentages reported by gcov wrong
        if(NOT CMAKE_BUILD_TYPE STREQUAL "Debug")
            message(FATAL_ERROR "GCC coverage needs a Debug build, not '${CMAKE_BUILD_TYPE}'")
        endif()

        # -fprofile-update=atomic: counters updated concurrently by several threads would otherwise be corrupted, and
        # lcov rejects the resulting negative counts
        add_compile_options(--coverage -fprofile-abs-path -fprofile-update=atomic)
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

# Coverage linker flags, to be added BEFORE creating any target: anything linking our instrumented static libraries
# needs them, including targets that aren't instrumented themselves
function(im3e_set_coverage_link_flags)
    if(NOT TEST_COVERAGE)
        return()
    endif()

    if(CMAKE_COMPILER_IS_GNUCXX)
        add_link_options(--coverage)
    elseif(CMAKE_CXX_COMPILER_ID STREQUAL "Clang" AND NOT MSVC)
        # clang-cl doesn't need it: CMake links with the linker directly rather than through clang-cl, and the
        # instrumented objects already request the profile runtime with /DEFAULTLIB.
        add_link_options(-fprofile-instr-generate)
    endif()
endfunction()

function(im3e_init_code_coverage)
    if(NOT TEST_COVERAGE)
        return()
    endif()

    if(CMAKE_COMPILER_IS_GNUCXX)
        im3e_internal_init_gcc_code_coverage()
    elseif(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
        im3e_internal_init_llvm_code_coverage()
    endif()
endfunction()
