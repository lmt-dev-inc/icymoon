add_library(im3e_state STATIC
    include/im3e/state/state.h
    src/state.cpp
)

target_include_directories(im3e_state
  PUBLIC
    include
  PRIVATE
    src
)