add_executable(im3e_glfw_app
    glfw_app.cpp
)

target_link_libraries(im3e_glfw_app
  PRIVATE
    im3e_guis
)

add_dependencies(im3e_glfw_app
    im3e_runtime_dlls
)