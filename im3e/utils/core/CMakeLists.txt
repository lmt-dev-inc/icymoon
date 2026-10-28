add_library(im3e_utils_core STATIC
    src/path_utils.cpp
    src/platform_utils.cpp
    flags.h
    path_utils.h
    platform_utils.h
    throw_utils.h
    types.h
)

target_include_directories(im3e_utils_core
  PUBLIC
    ${IM3E_INCLUDE_DIR}
  PRIVATE
    .
)

target_link_libraries(im3e_utils_core
  PUBLIC
    fmt::fmt
  PRIVATE
    whereami::whereami
)

add_subdirectory(test)
