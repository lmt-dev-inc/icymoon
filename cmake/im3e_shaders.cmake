# Functions to compile Slang shaders with im3e_shader_compiler, embedding their SPIR-V in generated C++ code.
#
# A shader is a .slang file with entry points, like a .cpp file. Modules without entry points are imported by shaders,
# like headers: they aren't compiled on their own, and are listed in the SOURCES of their target only to show them in
# IDEs. The dependency file written by the tool makes a shader compile again when a module it imports changes.
#
# For a shader plane.slang, the generated files are plane.slang.h and plane.slang.cpp, and the header declares:
#
#   namespace im3e::shaders {
#   [[nodiscard]] auto getPlaneShader() -> const im3e::CompiledShader&;
#   }
#
# The function name comes from the file name (see im3e_internal_shader_function_name), so it must be unique across the
# project: configuring fails otherwise.
#
# Modules shared between libraries go in the include/ folder of their library, next to its public headers, and are
# imported through their path: include/im3e/shader/constants.slang is imported with "import im3e.shader.constants;".
# As for headers, a shader can only import the shared modules of the libraries its target links.

include("${CMAKE_CURRENT_LIST_DIR}/src/im3e_shader_names.cmake")

# Adds one build step per shader, without needing a C++ target.
#
#   im3e_compile_shaders(
#     SHADERS <.slang files with entry points>...
#     OUTPUT_DIR <folder of the generated files>
#     GENERATED_SOURCES <variable set to the generated .h and .cpp files>
#     [IMPORT_PATHS <folders where import looks for modules>...]
#   )
#
# Relative shader paths are relative to the current source folder. IMPORT_PATHS may hold generator expressions, and
# modules are also found in the folder of the importing shader. The files in GENERATED_SOURCES must be used by a
# target of the same folder, which runs the build steps: a C++ target compiling them, or a custom target depending on
# them.
#
# The tool is the im3e_shader_compiler target by default, which may be defined after this call. A project without that
# target sets IM3E_SHADER_COMPILER to the tool's path instead and, on Windows, IM3E_SHADER_COMPILER_DLL_DIRS to the
# folders of its DLLs.
function(im3e_compile_shaders)
    set(options)
    set(oneValueArgs OUTPUT_DIR GENERATED_SOURCES)
    set(multiValueArgs SHADERS IMPORT_PATHS)
    cmake_parse_arguments(ARG
        "${options}"
        "${oneValueArgs}"
        "${multiValueArgs}"
        ${ARGN}
    )

    if (NOT ARG_SHADERS OR NOT ARG_OUTPUT_DIR OR NOT ARG_GENERATED_SOURCES)
        message(FATAL_ERROR "Missing arguments to im3e_compile_shaders")
    endif()

    if (ARG_UNPARSED_ARGUMENTS)
        message(FATAL_ERROR "Invalid arguments passed to im3e_compile_shaders")
    endif()

    if (DEFINED IM3E_SHADER_COMPILER)
        set(_COMPILER "${IM3E_SHADER_COMPILER}")
        set(_COMPILER_DLL_DIRS "${IM3E_SHADER_COMPILER_DLL_DIRS}")
        set(_COMPILER_DEPENDENCY "${IM3E_SHADER_COMPILER}")
    else()
        # A target dependency is also a file dependency: the shaders are compiled again when the tool changes
        set(_COMPILER $<TARGET_FILE:im3e_shader_compiler>)
        set(_COMPILER_DLL_DIRS $<TARGET_RUNTIME_DLL_DIRS:im3e_shader_compiler>)
        set(_COMPILER_DEPENDENCY im3e_shader_compiler)
    endif()

    set(_LAUNCHER "")
    if (WIN32)
        # Slang loads slang-glslang and its modules from the folder of slang-compiler.dll. They are found through the
        # PATH rather than copied next to the tool. An empty PATH entry is the current folder on POSIX, hence Windows
        # only: on Linux, the RPATH of the tool finds the library in its package. The ";" of a list of folders is also
        # the separator of the PATH on Windows.
        set(_LAUNCHER ${CMAKE_COMMAND} -E env --modify "PATH=path_list_prepend:${_COMPILER_DLL_DIRS}")
    endif()

    set(_GENERATED_SOURCES "")
    foreach(_SHADER IN LISTS ARG_SHADERS)
        cmake_path(
          ABSOLUTE_PATH _SHADER
          BASE_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
          NORMALIZE
          OUTPUT_VARIABLE _SHADER_PATH
        )
        cmake_path(GET _SHADER_PATH FILENAME _SHADER_FILE)
        cmake_path(GET _SHADER_PATH STEM _SHADER_STEM)

        im3e_internal_shader_function_name("${_SHADER_STEM}" _FUNCTION_NAME)
        im3e_internal_register_shader_function("${_FUNCTION_NAME}" "${_SHADER_PATH}")

        set(_HEADER "${ARG_OUTPUT_DIR}/${_SHADER_FILE}.h")
        set(_SOURCE "${ARG_OUTPUT_DIR}/${_SHADER_FILE}.cpp")
        set(_DEPFILE "${ARG_OUTPUT_DIR}/${_SHADER_FILE}.d")

        add_custom_command(
          OUTPUT
            "${_HEADER}"
            "${_SOURCE}"
          COMMAND
            ${_LAUNCHER}
            ${_COMPILER}
            "${_SHADER_PATH}"
            --header "${_HEADER}"
            --source "${_SOURCE}"
            --depfile "${_DEPFILE}"
            --function "${_FUNCTION_NAME}"
            $<IF:$<CONFIG:Debug>,--debug-info,--no-debug-info>
            # One argument for the whole list, which may be empty (see the tool)
            "--import-paths=${ARG_IMPORT_PATHS}"
          DEPENDS
            "${_COMPILER_DEPENDENCY}"
            "${_SHADER_PATH}"
          DEPFILE
            "${_DEPFILE}"
          COMMENT
            "Compiling ${_SHADER_FILE}"
          VERBATIM
        )

        list(APPEND _GENERATED_SOURCES "${_HEADER}" "${_SOURCE}")
    endforeach()

    set(${ARG_GENERATED_SOURCES} ${_GENERATED_SOURCES} PARENT_SCOPE)
endfunction()

# Compiles shaders and adds the generated code to a C++ target.
#
#   im3e_target_shaders(
#     TARGET <target>
#     SHADERS <.slang files with entry points>...
#   )
#
# The generated files are written to the shaders/ folder of the current binary folder, which is a private include
# folder of the target: plane.slang is used with #include "plane.slang.h". The target links im3e_shader, which the
# generated header includes. Called by the functions creating targets, through their SHADERS argument.
#
# The import paths are the include folders of the target, so that import finds what #include finds: the target's own
# folders, and the public folders of every library it links. A target makes more modules importable by adding their
# folder with target_include_directories.
function(im3e_target_shaders)
    set(options)
    set(oneValueArgs TARGET)
    set(multiValueArgs SHADERS)
    cmake_parse_arguments(ARG
        "${options}"
        "${oneValueArgs}"
        "${multiValueArgs}"
        ${ARGN}
    )

    if (NOT ARG_TARGET OR NOT ARG_SHADERS)
        message(FATAL_ERROR "Missing arguments to im3e_target_shaders")
    endif()

    if (ARG_UNPARSED_ARGUMENTS)
        message(FATAL_ERROR "Invalid arguments passed to im3e_target_shaders")
    endif()

    set(_OUTPUT_DIR "${CMAKE_CURRENT_BINARY_DIR}/shaders")
    im3e_compile_shaders(
      SHADERS
        ${ARG_SHADERS}
      OUTPUT_DIR
        "${_OUTPUT_DIR}"
      GENERATED_SOURCES
        _GENERATED_SOURCES
      IMPORT_PATHS
        "$<TARGET_PROPERTY:${ARG_TARGET},INCLUDE_DIRECTORIES>"
    )

    # The shaders are listed too, so that IDEs show them
    target_sources(${ARG_TARGET}
      PRIVATE
        ${_GENERATED_SOURCES}
        ${ARG_SHADERS}
    )

    target_include_directories(${ARG_TARGET}
      PRIVATE
        "${_OUTPUT_DIR}"
    )

    target_link_libraries(${ARG_TARGET}
      PRIVATE
        im3e_shader
    )
endfunction()
