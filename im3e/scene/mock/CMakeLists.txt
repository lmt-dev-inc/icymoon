im3e_add_mock_library(
  TARGET
    mock_im3e_scene
  SOURCES
    include/im3e/scene/mock_camera.h
    src/mock_camera.cpp
)

target_include_directories(mock_im3e_scene
  PUBLIC
    include
  PRIVATE
    src
)

target_link_libraries(mock_im3e_scene
  PUBLIC
    glm::glm
    GTest::gmock
    im3e_scene
)
