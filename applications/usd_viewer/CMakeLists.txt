add_executable(im3e_usd_viewer
    usd_viewer.cpp
)

target_link_libraries(im3e_usd_viewer
  PRIVATE
    im3e_guis
)

add_dependencies(im3e_usd_viewer
    im3e_runtime_dlls
)
