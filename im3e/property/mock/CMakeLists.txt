im3e_add_mock_library(
  TARGET mock_im3e_property
  MOCKED im3e_property
  SOURCES
    include/im3e/property/mock_property_interface.h
    src/mock_property_interface.cpp
)
