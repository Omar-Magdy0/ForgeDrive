cmake_minimum_required(VERSION 3.20)
# Include device-specific CMake configuration
find_path(ASIO_INCLUDE_DIR asio.hpp REQUIRED)

file(GLOB_RECURSE fd_drv_local CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/*.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp"
)
# Collect directories containing headers recursively
file(GLOB_RECURSE fd_drv_headers CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/*.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/*.hpp"
)
set(fd_drv_includes "")
foreach(header IN LISTS fd_drv_headers)
    get_filename_component(header_dir "${header}" DIRECTORY)
    list(APPEND fd_drv_includes "${header_dir}")
endforeach()
# Remove duplicate include directories
list(REMOVE_DUPLICATES fd_drv_includes)

add_library(fd_drv STATIC)
target_sources(fd_drv PRIVATE
    ${fd_drv_local}
)

target_include_directories(fd_drv PUBLIC 
    ${ASIO_INCLUDE_DIR}
    ${fd_drv_includes} 
    ${FDLIB_DIR}/fd
)

target_compile_features(fd_drv PUBLIC cxx_std_20)
