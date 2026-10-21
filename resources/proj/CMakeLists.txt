option(PROJ_RES_PATH "Path to proj res folder where proj.db file for GDAL is located")
if(NOT PROJ_RES_PATH)
    message(FATAL_ERROR "PROJ_RES_PATH, required by GDAL, is not defined")
endif()

set(dstFolder "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")

set(srcPath "${PROJ_RES_PATH}/proj.db")
set(dstPath "${dstFolder}/proj.db")

# The copy is an output of a custom command, so it only runs when proj.db changes
add_custom_command(
  OUTPUT
    "${dstFolder}/proj.db"
  COMMAND
    ${CMAKE_COMMAND} -E copy_if_different "${srcPath}" "${dstPath}"
  DEPENDS
    "${srcPath}"
  COMMENT
    "Copying proj.db from \"${PROJ_RES_PATH}\" to \"${dstFolder}\""
)

add_custom_target(im3e_resources_proj
  DEPENDS
    "${dstPath}"
)
