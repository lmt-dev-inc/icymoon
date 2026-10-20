# Copies the DLLs of the Conan dependencies next to our executables, so that they start without any environment set up.
# Windows has no RPATH: the loader looks for DLLs in the executable's folder, then in the system folders and the PATH.
# On Linux, CMake adds the folders of the shared libraries we link to the build-tree RPATH, so nothing is copied.
#
# Not every DLL is an import of our executables: anari devices (anari_library_*.dll) are plugins loaded at run time.
# So all the DLLs of the Conan dependencies are copied, rather than only the ones found in the executables' imports.
# All executables are built in the same folder, so they are copied once: executables depend on im3e_runtime_dlls.

set(_RUNTIME_DLL_COPIES "")

if(WIN32)
    if(NOT DEFINED CONAN_RUNTIME_LIB_DIRS)
        message(STATUS "CONAN_RUNTIME_LIB_DIRS is not defined: no runtime DLL is copied next to the executables")
    endif()

    foreach(_RUNTIME_DIR IN LISTS CONAN_RUNTIME_LIB_DIRS)
        # Conan still lists the packages whose binaries it skipped. Their package_info() never ran, so their dirs keep
        # the relative defaults ("bin"). They have nothing to copy, and globbing a relative dir would search for a "bin"
        # folder in this script folder i.e. our own code.
        if(NOT IS_ABSOLUTE "${_RUNTIME_DIR}")
            continue()
        endif()

        file(GLOB _RUNTIME_DLLS "${_RUNTIME_DIR}/*.dll")
        foreach(_RUNTIME_DLL IN LISTS _RUNTIME_DLLS)
            cmake_path(GET _RUNTIME_DLL FILENAME _RUNTIME_DLL_NAME)
            set(_RUNTIME_DLL_COPY "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${_RUNTIME_DLL_NAME}")

            add_custom_command(
              OUTPUT
                "${_RUNTIME_DLL_COPY}"
              COMMAND
                ${CMAKE_COMMAND} -E copy_if_different "${_RUNTIME_DLL}" "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}"
              DEPENDS
                "${_RUNTIME_DLL}"
              COMMENT
                "Copying ${_RUNTIME_DLL_NAME} from \"${_RUNTIME_DIR}\" to \"${CMAKE_RUNTIME_OUTPUT_DIRECTORY}\""
            )
            list(APPEND _RUNTIME_DLL_COPIES "${_RUNTIME_DLL_COPY}")
        endforeach()
    endforeach()
endif()

add_custom_target(im3e_runtime_dlls
  DEPENDS
    ${_RUNTIME_DLL_COPIES}
)
