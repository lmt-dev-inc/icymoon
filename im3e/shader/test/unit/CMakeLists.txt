im3e_add_unit_tests_executable(
  TARGET
    test_im3e_shader
  SOURCES
    test_compiled_shader.cpp
)

target_link_libraries(test_im3e_shader
  PRIVATE
    im3e_shader
    im3e_test_utils
)
