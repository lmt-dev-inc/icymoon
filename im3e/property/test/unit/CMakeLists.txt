im3e_add_unit_tests_executable(
  TARGET
    test_im3e_property
  SOURCES
    test_property_change_notifier.cpp
    test_property_group.cpp
    test_property_value_t.cpp
    test_property_value.cpp
)

target_link_libraries(test_im3e_property
  PRIVATE
    im3e_property
    im3e_test_utils
)
