im3e_add_unit_tests_executable(
  TARGET
    test_im3e_scene
  SOURCES
    test_perspective_projection.cpp
)

target_link_libraries(test_im3e_scene
  PRIVATE
    im3e_scene
    im3e_test_utils
)