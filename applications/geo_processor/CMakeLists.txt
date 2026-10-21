add_executable(im3e_geo_processor
    im3e_geo_processor.cpp
)

target_link_libraries(im3e_geo_processor
  PRIVATE
    GDAL::GDAL
    glm::glm
    im3e_geo
    im3e_utils
)

add_dependencies(im3e_geo_processor
    im3e_resources_proj
)

im3e_copy_runtime_dlls(im3e_geo_processor)
