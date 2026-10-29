im3e_add_unit_tests_executable(
  TARGET
    test_im3e_state
  SOURCES
    test_state.cpp
)

target_link_libraries(test_im3e_state
  PRIVATE
    im3e_state
    im3e_test_utils
)