im3e_add_unit_tests_executable(
  TARGET
    test_im3e_utils_core
  SOURCES
    test_path_utils.cpp
    test_platform_utils.cpp
    test_throw_utils.cpp
)

target_include_directories(test_im3e_utils_core
  PRIVATE
    ..
)

target_link_libraries(test_im3e_utils_core
  PRIVATE
    im3e_test_utils
    im3e_utils_core
)
