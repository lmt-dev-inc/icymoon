add_executable(im3e_usd_viewer
    usd_viewer.cpp
)

target_link_libraries(im3e_usd_viewer
  PRIVATE
    im3e_guis
)

im3e_copy_runtime_dlls(im3e_usd_viewer)
