# Functions to create the library targets of im3e/ (see "Library folders" in docs/development/code_standards.md).

include("${CMAKE_CURRENT_LIST_DIR}/src/im3e_library_names.cmake")

# Creates a library: a STATIC library defined in the lib/ folder of a library group.
#
#   im3e_add_library(
#     TARGET <target>
#     SOURCES <sources>...
#   )
#
# The target name must match the folder: im3e/vulkan/lib/ defines im3e_vulkan.
# The library exposes its include/ folder to the targets that link it, and keeps src/ private.
function(im3e_add_library)
    set(options)
    set(oneValueArgs TARGET)
    set(multiValueArgs SOURCES)
    cmake_parse_arguments(ARG
        "${options}"
        "${oneValueArgs}"
        "${multiValueArgs}"
        ${ARGN}
    )

    if (NOT ARG_TARGET OR NOT ARG_SOURCES)
        message(FATAL_ERROR "Missing arguments to im3e_add_library")
    endif()

    if (ARG_UNPARSED_ARGUMENTS)
        message(FATAL_ERROR "Invalid arguments passed to im3e_add_library")
    endif()

    im3e_internal_check_library_name(${ARG_TARGET})

    add_library(${ARG_TARGET} STATIC ${ARG_SOURCES})

    target_include_directories(${ARG_TARGET}
      PUBLIC
        include
      PRIVATE
        src
    )
endfunction()

# Creates an internal library: an OBJECT library that is part of exactly one STATIC library, its owner.
#
#   im3e_add_internal_library(
#     TARGET <target>
#     OWNER <owner target>
#     SOURCES <sources>...
#   )
#
# The internal library exposes its include/ folder to the targets that link it, and keeps src/ private.
# It is linked privately into its owner, so that its object files are archived into the owner's library, and its
# include/ folder doesn't propagate to the users of the owner. The link is wrapped in $<BUILD_INTERFACE:...> so that the
# internal library stays out of exported and installed packages: its code is already in the owner's archive.
#
# The target name must match the folder: im3e/vulkan/internal/loader/lib/ defines im3e_vulkan_internal_loader.
# The owner must be defined before calling this function: add the owner's lib/ folder before internal/.
function(im3e_add_internal_library)
    set(options)
    set(oneValueArgs TARGET OWNER)
    set(multiValueArgs SOURCES)
    cmake_parse_arguments(ARG
        "${options}"
        "${oneValueArgs}"
        "${multiValueArgs}"
        ${ARGN}
    )

    if (NOT ARG_TARGET OR NOT ARG_OWNER OR NOT ARG_SOURCES)
        message(FATAL_ERROR "Missing arguments to im3e_add_internal_library")
    endif()

    if (ARG_UNPARSED_ARGUMENTS)
        message(FATAL_ERROR "Invalid arguments passed to im3e_add_internal_library")
    endif()

    im3e_internal_check_library_name(${ARG_TARGET})

    if (NOT TARGET ${ARG_OWNER})
        message(FATAL_ERROR
            "Owner ${ARG_OWNER} of internal library ${ARG_TARGET} is not defined: add its folder before internal/")
    endif()

    get_target_property(_OWNER_TYPE ${ARG_OWNER} TYPE)
    if (NOT _OWNER_TYPE STREQUAL "STATIC_LIBRARY")
        message(FATAL_ERROR
            "Owner ${ARG_OWNER} of internal library ${ARG_TARGET} must be a STATIC library, not ${_OWNER_TYPE}")
    endif()

    add_library(${ARG_TARGET} OBJECT ${ARG_SOURCES})

    # Read by im3e_add_mock_library: a mock links an internal library differently.
    set_target_properties(${ARG_TARGET} PROPERTIES IM3E_INTERNAL_OWNER ${ARG_OWNER})

    target_include_directories(${ARG_TARGET}
      PUBLIC
        include
      PRIVATE
        src
    )

    target_link_libraries(${ARG_OWNER}
      PRIVATE
        $<BUILD_INTERFACE:${ARG_TARGET}>
    )
endfunction()
