add_library(im3e_utils STATIC
    src/stats_provider.cpp
    src/stream_logger.cpp
    src/stream_logger.h
    src/view_frustum.cpp
    src/vk_utils.cpp
    imgui_utils.h
    loggers.h
    math_utils.h
    stats.h
    transform.h
    view_frustum.h
    vk_utils.h
)

target_include_directories(im3e_utils
  PUBLIC
    ${IM3E_INCLUDE_DIR}
  PRIVATE
    .
    src
)

target_link_libraries(im3e_utils
  PUBLIC
    fmt::fmt
    glm::glm
    im3e_utils_core
    GPUOpen::VulkanMemoryAllocator
    Vulkan::Headers
  PRIVATE
    whereami::whereami
)

add_subdirectory(core)
add_subdirectory(mock)
add_subdirectory(properties)
add_subdirectory(test)
