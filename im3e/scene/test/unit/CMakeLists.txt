im3e_add_unit_tests_executable(
  TARGET
    test_im3e_scene
  SOURCES
    test_camera.cpp
    test_map_camera_controller.cpp
    test_perspective_projection.cpp
    test_view_transform.cpp
)

target_link_libraries(test_im3e_scene
  PRIVATE
    im3e_scene
    im3e_test_utils
    mock_im3e_scene
)