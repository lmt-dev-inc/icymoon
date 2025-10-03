add_library(im3e_utils_properties STATIC
    src/property_change_notifier.cpp
    src/property_group.cpp
    src/read_only_property_value_proxy.cpp
    properties.h
    property_change_notifier.h
)

target_include_directories(im3e_utils_properties
  PUBLIC
    ${IM3E_INCLUDE_DIR}
  PRIVATE
    .
    src
)

target_link_libraries(im3e_utils_properties
  PUBLIC
    im3e_utils_core
    im3e_utils_properties_api
)

add_subdirectory(api)
add_subdirectory(test)