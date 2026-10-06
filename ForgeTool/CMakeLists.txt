cmake_minimum_required(VERSION 3.10)

# Project name and version
project(ForgeTool VERSION 1.0.0 LANGUAGES CXX)

set(THIRDPARTY_DIR ${CMAKE_SOURCE_DIR}/../thirdparty)
set(FDLIB_DIR ${CMAKE_SOURCE_DIR}/../fdlib)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Set build type to Release if not specified
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Build type" FORCE)
endif()

# Find libusb-1.0
find_package(PkgConfig REQUIRED)
find_path(ASIO_INCLUDE_DIR asio.hpp REQUIRED)
pkg_check_modules(LIBUSB REQUIRED libusb-1.0)
add_subdirectory(${THIRDPARTY_DIR}/etl etl)


file(GLOB_RECURSE MIDDLEWARE_SRC    
    ${FDLIB_DIR}/middleware/*.cpp
    ${FDLIB_DIR}/middleware/*.c
)

file(GLOB_RECURSE CORE_SRC    
    ${FORGELIB}/fd/*.cpp
    ${FORGELIB}/fd/*.c
)

# Add executable
add_executable(${PROJECT_NAME}
    ForgeTool.cpp
    # Header files are listed for IDE visibility, not strictly required for build
    Usbxch.cpp
    ${MIDDLEWARE_SRC}
    ${CORE_SRC}
)

find_package(PkgConfig REQUIRED)
find_package(OpenGL REQUIRED)
find_package(glfw3 REQUIRED)
find_package(Threads REQUIRED)
# Link libusb and threads
target_link_libraries(${PROJECT_NAME}
    ${LIBUSB_LIBRARIES}
    Threads::Threads
    OpenGL::GL 
    glfw
    etl::etl
)

# Include directories for libusb
target_include_directories(${PROJECT_NAME} PRIVATE
    ${ASIO_INCLUDE_DIR}
    ${LIBUSB_INCLUDE_DIRS}
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${FDLIB_DIR}/middleware
    ${FDLIB_DIR}
)

# Compiler flags (optional - remove if not needed)
target_compile_options(${PROJECT_NAME} PRIVATE
    -Wall
    -Wextra
    -Wpedantic
    -O2
)

target_compile_definitions(${PROJECT_NAME} PRIVATE
    ETL_THROW_EXCEPTIONS

    $<$<CONFIG:Debug>:ETL_VERBOSE_ERRORS>
    $<$<CONFIG:Debug>:ETL_CHECK_PUSH_POP>
)

# For debugging, uncomment this:
# target_compile_definitions(${PROJECT_NAME} PRIVATE DEBUG)

# Install (optional - remove if not needed)
install(TARGETS ${PROJECT_NAME} DESTINATION bin)