add_library(im3e_scene STATIC
    include/im3e/scene/camera.h
    include/im3e/scene/perspective_projection.h
    include/im3e/scene/view_transform.h
    src/camera.cpp
    src/perspective_projection.cpp
    src/view_transform.cpp
)

target_include_directories(im3e_scene
  PUBLIC
    include
  PRIVATE
    src
)

target_link_libraries(im3e_scene
  PUBLIC
    glm::glm
    im3e_utils_properties
)