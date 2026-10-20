if (NOT(WIN32) AND (CMAKE_BUILD_TYPE STREQUAL "Debug" OR GENERATOR_IS_MULTI_CONFIG))
    include("${CMAKE_CURRENT_LIST_DIR}/src/code_coverage.cmake")
endif()
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
        # The executables must also be linked with the profile runtime. clang-cl doesn't need it: CMake links with
        # the linker directly rather than through clang-cl, and the instrumented objects already request the runtime
        # with /DEFAULTLIB.
        if(NOT MSVC)
            add_link_options(-fprofile-instr-generate)
        endif()
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
        im3e_internal_init_llvm_code_coverage()
    endif()
endfunction()
