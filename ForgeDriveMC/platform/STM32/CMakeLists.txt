add_library(PLATFORM STATIC)
include(${CMAKE_CURRENT_SOURCE_DIR}/../arm/arm.cmake)
target_sources(fd_freertos PRIVATE
    "${FREERTOS_DIR}/portable/GCC/${FREERTOS_PORT}/port.c"
)
target_include_directories(fd_freertos PUBLIC
    "${FREERTOS_DIR}/portable/GCC/${FREERTOS_PORT}"
)
target_sources(PLATFORM PUBLIC platform.c)
target_include_directories(PLATFORM PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})