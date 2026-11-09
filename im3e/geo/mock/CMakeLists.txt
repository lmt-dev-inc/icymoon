im3e_add_mock_library(
  TARGET mock_im3e_geo
  MOCKED im3e_geo
  SOURCES
    include/im3e/geo/mock_height_map.h
    src/mock_height_map.cpp
)
