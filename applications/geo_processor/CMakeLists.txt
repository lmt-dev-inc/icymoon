im3e_add_executable(
  TARGET im3e_geo_processor
  SOURCES
    im3e_geo_processor.cpp
)

target_link_libraries(im3e_geo_processor
  PRIVATE
    GDAL::GDAL
    glm::glm
    im3e_geo
    im3e_utils
)
