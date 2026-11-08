im3e_add_mock_library(
  TARGET
    mock_im3e
  SOURCES
    src/mock_height_map.cpp
    mock_height_map.h
)

target_include_directories(mock_im3e
  PUBLIC
    ${IM3E_INCLUDE_DIR}
  PRIVATE
    .
    src
)

target_link_libraries(mock_im3e
  PUBLIC
    GTest::gmock
    im3e_api
)
