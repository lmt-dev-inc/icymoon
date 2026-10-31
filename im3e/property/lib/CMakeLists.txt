add_library(im3e_property STATIC
    include/im3e/property/properties.h
    include/im3e/property/property_change_notifier.h
    include/im3e/property/property_interface.h
    src/property_change_notifier.cpp
    src/property_group.cpp
    src/read_only_property_value_proxy.cpp
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
