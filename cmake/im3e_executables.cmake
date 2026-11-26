# Functions to create the executable targets of im3e/ and applications/.

# Creates an executable, built in CMAKE_RUNTIME_OUTPUT_DIRECTORY with the DLLs it needs.
#
#   im3e_add_executable(
#     TARGET <target>
#     SOURCES <sources>...
#     [SHADERS <.slang files with entry points>...]
#     [NO_RUNTIME_DLL_COPY]
#   )
#
# The DLLs that the executable imports are copied next to it on Windows (see im3e_copy_runtime_dlls in
# cmake/im3e_runtime_dlls.cmake), so that it starts without any environment set up. NO_RUNTIME_DLL_COPY leaves them
# out, for executables that find their DLLs another way, such as the build tools run with their DLL folders on the
# PATH. Their DLLs then stay out of the output folder, and they don't wait for the DLLs of the other executables.
# SHADERS are compiled and embedded in the executable (see im3e_target_shaders in cmake/im3e_shaders.cmake).
function(im3e_add_executable)
    set(options NO_RUNTIME_DLL_COPY)
    set(oneValueArgs TARGET)
    set(multiValueArgs SOURCES SHADERS)
    cmake_parse_arguments(ARG
        "${options}"
        "${oneValueArgs}"
        "${multiValueArgs}"
        ${ARGN}
    )

    if (NOT ARG_TARGET OR NOT ARG_SOURCES)
        message(FATAL_ERROR "Missing arguments to im3e_add_executable")
    endif()

    if (ARG_UNPARSED_ARGUMENTS)
        message(FATAL_ERROR "Invalid arguments passed to im3e_add_executable")
    endif()

    add_executable(${ARG_TARGET} ${ARG_SOURCES})

    if (ARG_SHADERS)
        im3e_target_shaders(TARGET ${ARG_TARGET} SHADERS ${ARG_SHADERS})
    endif()

    if (NOT ARG_NO_RUNTIME_DLL_COPY)
        im3e_copy_runtime_dlls(${ARG_TARGET})
    endif()
endfunction()
