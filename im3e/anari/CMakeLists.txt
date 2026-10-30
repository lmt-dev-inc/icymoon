add_library(im3e_anari STATIC
    anari.h
    src/anari_device.cpp
    src/anari_device.h
    src/anari_engine.cpp
    src/anari_extension_utility.cpp
    src/anari_frame_pipeline.cpp
    src/anari_frame_pipeline.h
    src/anari_height_field_tile.cpp
    src/anari_height_field_tile.h
    src/anari_height_field.cpp
    src/anari_height_field.h
    src/anari_instance_set.cpp
    src/anari_instance_set.h
    src/anari_map_camera.cpp
    src/anari_map_camera.h
    src/anari_plane.cpp
    src/anari_plane.h
    src/anari_renderer.cpp
    src/anari_renderer.h
    src/anari_utils.h
    src/anari_world.cpp
    src/anari_world.h
)

target_include_directories(im3e_anari
  PUBLIC
    ${IM3E_INCLUDE_DIR}
  PRIVATE
    .
    src
)

target_link_libraries(im3e_anari
  PUBLIC
    anari::anari_static
    im3e_api
    im3e_geo
    im3e_property
    im3e_scene
    im3e_utils
  PRIVATE
    fmt::fmt
)

im3e_copy_anari_devices(im3e_anari)