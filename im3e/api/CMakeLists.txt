add_library(im3e_api INTERFACE
    gui.h
    height_map.h
    window.h
)

target_include_directories(im3e_api
  INTERFACE
    ${IM3E_INCLUDE_DIR}
)

target_link_libraries(im3e_api
  INTERFACE
    fmt::fmt
    glm::glm
    im3e_utils
    im3e_vulkan
)