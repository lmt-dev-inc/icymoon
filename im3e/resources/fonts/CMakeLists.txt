set(srcFile "SourceCodePro-Regular.ttf")
set(dstFolder "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/fonts")

set(srcPath "${CMAKE_CURRENT_SOURCE_DIR}/${srcFile}")
set(dstPath "${dstFolder}/${srcFile}")

file(MAKE_DIRECTORY ${dstFolder})

# The copy is an output of a custom command, so it only runs when the font changes
add_custom_command(
  OUTPUT
    "${dstPath}"
  COMMAND
    ${CMAKE_COMMAND} -E copy_if_different "${srcPath}" "${dstPath}"
  DEPENDS
    "${srcPath}"
  COMMENT
    "Copying \"${srcFile}\" to \"${dstFolder}\""
)

add_custom_target(im3e_resources_fonts
  DEPENDS
    "${dstPath}"
)

add_dependencies(im3e_resources im3e_resources_fonts)