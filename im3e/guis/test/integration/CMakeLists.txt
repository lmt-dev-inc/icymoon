im3e_add_integration_tests_executable(
  TARGET
    integration_im3e_guis
  SOURCES
    clear_color_test_pipeline.h
    integration_glfw_window_application.cpp
    integration_glfw_window.cpp
    integration_imgui_pipeline.cpp
    integration_imgui_render_panel.cpp
    integration_imgui_vulkan_backend.cpp
)

target_include_directories(integration_im3e_guis
  PRIVATE
    ../..
)

target_link_libraries(integration_im3e_guis
  PRIVATE
    glfw
    imgui
    im3e_guis
    im3e_test_utils
    im3e_vulkan
)
