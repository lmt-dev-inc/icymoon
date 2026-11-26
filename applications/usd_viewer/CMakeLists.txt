im3e_add_executable(
  TARGET im3e_usd_viewer
  SOURCES
    usd_viewer.cpp
)

target_link_libraries(im3e_usd_viewer
  PRIVATE
    im3e_guis
)
