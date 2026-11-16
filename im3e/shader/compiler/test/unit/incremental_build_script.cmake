# Builds a copy of the project in "incremental build/", and checks that a shader compiles again only when a file it
# reads changes: after a first build, a build without changes compiles nothing, and editing the imported module only
# compiles the shader importing it. Run by the ShaderCompilerTool.rebuildsOnlyShadersImportingChangedModule test, with:
#
#   cmake -DPROJECT_DIR=<folder of the test project>
#         -DWORK_DIR=<folder for the copy of the project and its build, deleted first>
#         -DIM3E_CMAKE_DIR=<cmake/ folder of IcyMoon>
#         -DGENERATOR=<CMake generator> -DMAKE_PROGRAM=<its build tool>
#         -DCOMPILER=<path of im3e_shader_compiler>
#         -DCOMPILER_DLL_DIRS=<folders of its DLLs, separated by "|", as ";" doesn't survive the command line>
#         -P incremental_build_script.cmake

foreach(VARIABLE PROJECT_DIR WORK_DIR IM3E_CMAKE_DIR GENERATOR MAKE_PROGRAM COMPILER COMPILER_DLL_DIRS)
    if(NOT DEFINED ${VARIABLE})
        message(FATAL_ERROR "Missing -D${VARIABLE}")
    endif()
endforeach()

string(REPLACE "|" ";" COMPILER_DLL_DIRS "${COMPILER_DLL_DIRS}")

# The project is copied, so that its module can be edited without changing the repository
set(SOURCE_DIR "${WORK_DIR}/source")
set(BUILD_DIR "${WORK_DIR}/build")
file(REMOVE_RECURSE "${WORK_DIR}")
file(COPY "${PROJECT_DIR}/" DESTINATION "${SOURCE_DIR}")

execute_process(
    COMMAND "${CMAKE_COMMAND}"
        -S "${SOURCE_DIR}"
        -B "${BUILD_DIR}"
        -G "${GENERATOR}"
        "-DCMAKE_MAKE_PROGRAM=${MAKE_PROGRAM}"
        "-DIM3E_CMAKE_DIR=${IM3E_CMAKE_DIR}"
        "-DIM3E_SHADER_COMPILER=${COMPILER}"
        "-DIM3E_SHADER_COMPILER_DLL_DIRS=${COMPILER_DLL_DIRS}"
    RESULT_VARIABLE RESULT
    OUTPUT_VARIABLE OUTPUT
    ERROR_VARIABLE OUTPUT
)
if(NOT RESULT EQUAL 0)
    message(FATAL_ERROR "Configuring the test project failed. Output:\n${OUTPUT}")
endif()

# Builds the project, and fails unless exactly the shaders given after STEP are compiled, which the build tool shows
# with the comments of their build steps: "Compiling <shader>.slang".
function(check_build STEP)
    set(EXPECTED_SHADERS ${ARGN})

    execute_process(
        COMMAND "${CMAKE_COMMAND}" --build "${BUILD_DIR}"
        RESULT_VARIABLE RESULT
        OUTPUT_VARIABLE OUTPUT
        ERROR_VARIABLE OUTPUT
    )
    if(NOT RESULT EQUAL 0)
        message(FATAL_ERROR "${STEP}: the build failed. Output:\n${OUTPUT}")
    endif()

    foreach(SHADER importing_shader standalone_shader)
        string(FIND "${OUTPUT}" "Compiling ${SHADER}.slang" POSITION)
        if(SHADER IN_LIST EXPECTED_SHADERS AND POSITION EQUAL -1)
            message(FATAL_ERROR "${STEP}: expected ${SHADER}.slang to be compiled. Output:\n${OUTPUT}")
        elseif(NOT SHADER IN_LIST EXPECTED_SHADERS AND NOT POSITION EQUAL -1)
            message(FATAL_ERROR "${STEP}: expected ${SHADER}.slang not to be compiled. Output:\n${OUTPUT}")
        endif()
    endforeach()
endfunction()

check_build("First build" importing_shader standalone_shader)
check_build("Build without changes")

# Waits, so that the edited module is newer than the generated files on file systems with coarse timestamps
execute_process(COMMAND "${CMAKE_COMMAND}" -E sleep 1)
file(APPEND "${SOURCE_DIR}/imported_module.slang" "\npublic float4 getOtherColor() { return float4(0.0); }\n")
check_build("Build after editing the imported module" importing_shader)

# The function added to the module is unused, so the generated files don't change and the tool leaves them as they
# were, older than the module. The build tool must still not compile the shader again on every build.
check_build("Build after compiling the edited shader")
