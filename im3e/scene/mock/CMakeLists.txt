im3e_add_mock_library(
  TARGET mock_im3e_scene
  MOCKED im3e_scene
  SOURCES
    include/im3e/scene/mock_camera.h
    src/mock_camera.cpp
)

target_link_libraries(mock_im3e_scene
  PUBLIC
    glm::glm
)
