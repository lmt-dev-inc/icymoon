add_executable(im3e_glfw_app
    glfw_app.cpp
)

target_link_libraries(im3e_glfw_app
  PRIVATE
    im3e_guis
)

im3e_copy_runtime_dlls(im3e_glfw_app)
