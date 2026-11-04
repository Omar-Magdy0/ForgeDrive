include(${CMAKE_CURRENT_LIST_DIR}/../arm/arm.cmake)
target_sources(fd_freertos PRIVATE
    "${FREERTOS_DIR}/portable/GCC/${FREERTOS_PORT}/port.c"
)
target_include_directories(fd_freertos PUBLIC
    "${FREERTOS_DIR}/portable/GCC/${FREERTOS_PORT}"
)

set(PLATFORM_SRC 
        ${CMAKE_CURRENT_LIST_DIR}/platform.c 
        ${CMAKE_CURRENT_LIST_DIR}/../syscalls.c
        ${CMAKE_CURRENT_LIST_DIR}/../sysmem.c
    )
set(PLATFORM_DIR ${CMAKE_CURRENT_LIST_DIR})