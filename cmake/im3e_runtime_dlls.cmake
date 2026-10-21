# Copies the DLLs our executables need next to them, so that they start without any environment set up.
# Windows has no RPATH: the loader looks for DLLs in the executable's folder, then in the system folders and the PATH.
# On Linux, CMake adds the folders of the shared libraries we link to the build-tree RPATH, so nothing is copied.

# Copies the DLLs that the executable imports, directly or through its dependencies, next to it.
# Only the shared libraries linked as SHARED IMPORTED targets are listed, which is what Conan generates for them.
# The copy runs on every build, before the executable is linked: a DLL updated or deleted without any relink is still
# copied, and the DLLs are in place for the post-build steps that run the executable (e.g. gtest_discover_tests).
# copy_if_different compares the contents, so the DLLs that did not change are not copied again.
# All executables are built in the same folder, so the DLLs of all of them are copied by a single target:
# separate targets would write the same files at the same time in parallel builds.
function(im3e_copy_runtime_dlls TARGET)
    if(NOT WIN32)
        return()
    endif()

    get_property(_TARGETS GLOBAL PROPERTY IM3E_RUNTIME_DLL_TARGETS)
    if(NOT _TARGETS)
        # The copy target is created once all executables are known, at the end of the top-level directory
        cmake_language(DEFER DIRECTORY "${CMAKE_SOURCE_DIR}" CALL _im3e_add_runtime_dlls_target)
    endif()
    set_property(GLOBAL APPEND PROPERTY IM3E_RUNTIME_DLL_TARGETS ${TARGET})

    # Dependencies are resolved at generate time, so the copy target does not need to exist yet
    add_dependencies(${TARGET} im3e_runtime_dlls)
endfunction()

function(_im3e_add_runtime_dlls_target)
    get_property(_TARGETS GLOBAL PROPERTY IM3E_RUNTIME_DLL_TARGETS)
    set(_RUNTIME_DLLS "")
    foreach(_TARGET IN LISTS _TARGETS)
        list(APPEND _RUNTIME_DLLS "$<TARGET_RUNTIME_DLLS:${_TARGET}>")
    endforeach()
    set(_RUNTIME_DLLS "$<REMOVE_DUPLICATES:${_RUNTIME_DLLS}>")

    # The target has no dependencies, so it can run before anything else created the folder, and copy_if_different
    # would then write a file named after it with one DLL, or fail with several.
    # copy_if_different fails without files, so a no-op runs instead when there is no DLL dependency
    add_custom_target(im3e_runtime_dlls
      COMMAND
        ${CMAKE_COMMAND} -E make_directory "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}"
      COMMAND
        ${CMAKE_COMMAND} -E
        "$<IF:$<BOOL:${_RUNTIME_DLLS}>,copy_if_different;${_RUNTIME_DLLS};${CMAKE_RUNTIME_OUTPUT_DIRECTORY},true>"
      COMMAND_EXPAND_LISTS
    )
endfunction()

# Anari devices (anari_library_*.dll) are plugins loaded at run time, so they are not in the imports of any executable.
# They are copied with the rest of the anari package's DLLs, which they import (e.g. the debug device imports anari.dll).
# All executables are built in the same folder, so they are copied once: im3e_anari depends on im3e_anari_devices.
function(im3e_copy_anari_devices TARGET)
    if(NOT WIN32)
        return()
    endif()

    if(NOT TARGET im3e_anari_devices)
        # anari is packaged as a static library, so Conan has no bin dir for it: its DLLs are in the package folder
        string(TOUPPER "${CMAKE_BUILD_TYPE}" _BUILD_TYPE)
        set(_ANARI_BIN_DIR "${anari_PACKAGE_FOLDER_${_BUILD_TYPE}}/bin")
        file(GLOB _ANARI_DLLS "${_ANARI_BIN_DIR}/*.dll")
        if(NOT _ANARI_DLLS)
            message(WARNING "No anari device found in \"${_ANARI_BIN_DIR}\": anari applications will fail to load devices")
        endif()

        set(_ANARI_DLL_COPIES "")
        foreach(_ANARI_DLL IN LISTS _ANARI_DLLS)
            cmake_path(GET _ANARI_DLL FILENAME _ANARI_DLL_NAME)
            set(_ANARI_DLL_COPY "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${_ANARI_DLL_NAME}")

            add_custom_command(
              OUTPUT
                "${_ANARI_DLL_COPY}"
              COMMAND
                ${CMAKE_COMMAND} -E copy_if_different "${_ANARI_DLL}" "${_ANARI_DLL_COPY}"
              DEPENDS
                "${_ANARI_DLL}"
              COMMENT
                "Copying ${_ANARI_DLL_NAME} from \"${_ANARI_BIN_DIR}\" to \"${CMAKE_RUNTIME_OUTPUT_DIRECTORY}\""
            )
            list(APPEND _ANARI_DLL_COPIES "${_ANARI_DLL_COPY}")
        endforeach()

        add_custom_target(im3e_anari_devices
          DEPENDS
            ${_ANARI_DLL_COPIES}
        )
    endif()

    add_dependencies(${TARGET} im3e_anari_devices)
endfunction()
