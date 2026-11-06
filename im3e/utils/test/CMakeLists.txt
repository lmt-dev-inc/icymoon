im3e_add_unit_tests_executable(
  TARGET
    test_im3e_utils
  SOURCES
    test_imgui_utils.cpp
    test_logger_tracker.cpp
    test_loggers.cpp
    test_math_utils.cpp
    test_path.cpp
    test_stream_logger_global_tracker.cpp
    test_stream_logger.cpp
    test_transform.cpp
    test_view_frustum.cpp
    test_vk_utils.cpp
)

target_include_directories(test_im3e_utils
  PRIVATE
    ..
)

target_link_libraries(test_im3e_utils
  PRIVATE
    im3e_test_utils
    im3e_utils
    mock_im3e
)