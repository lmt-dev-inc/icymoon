add_executable(im3e_anari_viewer
    anari_dem_surface_generator.cpp
    anari_dem_surface_generator.h
    dem_block_sampler.h
    gdal_dem_loader.cpp
    gdal_dem_loader.h
    im3e_anari_viewer.cpp
)

target_link_libraries(im3e_anari_viewer
  PRIVATE
    anari::anari_static
    fmt::fmt
    GDAL::GDAL
    glm::glm
    im3e_anari
    im3e_devices
    im3e_geo
    im3e_guis
    im3e_utils
    im3e_utils_properties
)

add_dependencies(im3e_anari_viewer
    im3e_resources_proj
)

im3e_copy_runtime_dlls(im3e_anari_viewer)
