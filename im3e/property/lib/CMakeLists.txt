add_library(im3e_property STATIC
    include/im3e/property/property_group.h
    include/im3e/property/property_interface.h
    include/im3e/property/property.h
    src/property_group.cpp
)

target_include_directories(im3e_property
  PUBLIC
    include
  PRIVATE
    src
)

target_link_libraries(im3e_property
  PUBLIC
    fmt::fmt
    im3e_utils_core
)
