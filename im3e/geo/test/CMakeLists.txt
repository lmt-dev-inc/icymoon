im3e_add_unit_tests_executable(
  TARGET
    test_im3e_geo
  SOURCES
    test_height_map_quad_tree.cpp
)

target_include_directories(test_im3e_geo
  PRIVATE
    ..
)

target_link_libraries(test_im3e_geo
  PRIVATE
    im3e_geo
    im3e_test_utils
    mock_im3e_geo
)
