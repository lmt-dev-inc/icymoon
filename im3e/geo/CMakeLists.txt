add_library(im3e_geo STATIC
    geo.h
    height_map.h
    src/gdal_geotiff_height_map.cpp
    src/gdal_geotiff_height_map.h
    src/gdal_instance.cpp
    src/gdal_instance.h
    src/gdal_utils.h
    src/height_map_quad_tree.cpp
)

target_include_directories(im3e_geo
  PUBLIC
    ${IM3E_INCLUDE_DIR}
  PRIVATE
    .
    src
)

target_link_libraries(im3e_geo
  PUBLIC
    im3e_utils
  PRIVATE
    GDAL::GDAL
)

add_subdirectory(mock)
add_subdirectory(test)