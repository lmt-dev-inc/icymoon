im3e_add_executable(
  TARGET im3e_glfw_app
  SOURCES
    glfw_app.cpp
)

target_link_libraries(im3e_glfw_app
  PRIVATE
    im3e_guis
)
