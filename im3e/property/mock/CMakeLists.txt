im3e_add_mock_library(
  TARGET
    mock_im3e_property
  SOURCES
    include/im3e/property/mock_property_interface.h
    src/mock_property_interface.cpp
)

target_include_directories(mock_im3e_property
  PUBLIC
    include
  PRIVATE
    src
)

target_link_libraries(mock_im3e_property
  PUBLIC
    GTest::gmock
    im3e_property
)