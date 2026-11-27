cmake_minimum_required(VERSION 3.10)

# Project name and version
project(ForgeTool VERSION 1.0.0 LANGUAGES C CXX)
set(THIRDPARTY_DIR ${CMAKE_SOURCE_DIR}/../thirdparty)
set(FDLIB_DIR ${CMAKE_SOURCE_DIR}/../fdlib)
set(IMGUI_DIR ${THIRDPARTY_DIR}/imgui)
set(IMPLOT_DIR ${THIRDPARTY_DIR}/implot)
set(IMGUIFD_DIR ${THIRDPARTY_DIR}/ImGuiFileDialog)
set(FDMC_DIR ${CMAKE_SOURCE_DIR}/../ForgeDriveMC/fdmc)
set(CMAKE_C_STANDARD 23)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
# Set build type to Release if not specified
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Build type" FORCE)
endif()

set(IMGUI_SRC  
    ${IMGUI_DIR}/imgui_draw.cpp
    ${IMGUI_DIR}/imgui_tables.cpp
    ${IMGUI_DIR}/imgui_widgets.cpp
    ${IMGUI_DIR}/imgui.cpp
    ${IMGUI_DIR}/imgui_demo.cpp
    ${IMPLOT_DIR}/implot_items.cpp
    ${IMPLOT_DIR}/implot.cpp
    ${IMPLOT_DIR}/implot_demo.cpp
    ${IMGUI_DIR}/backends/imgui_impl_opengl3.cpp
    ${IMGUI_DIR}/backends/imgui_impl_glfw.cpp
    ${IMGUIFD_DIR}/ImGuiFileDialog.cpp
)

set(IMGUI_INCLUDES
    ${IMGUI_DIR}
    ${IMGUI_DIR}/backends
    ${IMGUIFD_DIR}
    ${IMPLOT_DIR}
)

find_path(ASIO_INCLUDE_DIR asio.hpp REQUIRED)
find_package(Protobuf REQUIRED)
find_package(PkgConfig REQUIRED)
find_package(OpenGL REQUIRED)
find_package(glfw3 REQUIRED)
find_package(Threads REQUIRED)

pkg_check_modules(LIBUSB REQUIRED libusb-1.0)
add_subdirectory(${THIRDPARTY_DIR}/etl etl)

set(MIDDLEWARE_SRC    
    ${FDLIB_DIR}/middleware/Abf.c
    ${FDLIB_DIR}/middleware/fdrpc/fdrpc.pb.cc
)

# Add executable
add_executable(${PROJECT_NAME}
    ${CMAKE_SOURCE_DIR}/ForgeTool.cpp
    ${CMAKE_SOURCE_DIR}/Usbxch.cpp
    ${CMAKE_SOURCE_DIR}/ConnectionPage.cpp
    ${CMAKE_SOURCE_DIR}/LaunchablesPage.cpp
    ${CMAKE_SOURCE_DIR}/FdrpcTool.cpp
    ${CMAKE_SOURCE_DIR}/fdmc/fdmc.cpp
    ${MIDDLEWARE_SRC}
    ${IMGUI_SRC}
    ${CORE_SRC}
)

# Link libusb and threads
target_link_libraries(${PROJECT_NAME}
    ${LIBUSB_LIBRARIES}
    Threads::Threads
    OpenGL::GL 
    glfw
    etl::etl
    protobuf::libprotobuf
    protobuf::libprotoc
)

# Include directories for libusb
target_include_directories(${PROJECT_NAME} PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/fdmc
    ${LIBUSB_INCLUDE_DIRS}
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${ASIO_INCLUDE_DIR}
    ${FDLIB_DIR}/middleware
    ${FDLIB_DIR}/middleware/fdrpc
    ${FDLIB_DIR}/fd
    ${FDLIB_DIR}
    ${FDMC_DIR}
    ${THIRDPARTY_DIR}/fonts/FontAwesome7
    ${IMGUI_INCLUDES}
)

# Compiler flags (optional - remove if not needed)
target_compile_options(${PROJECT_NAME} PRIVATE
    -Wall
    -Wextra
    -Wpedantic
    -finput-charset=UTF-8
    -fexec-charset=UTF-8
)

target_compile_definitions(${PROJECT_NAME} PRIVATE
    ETL_THROW_EXCEPTIONS
    FONTS_DIR="${THIRDPARTY_DIR}/fonts"
    $<$<CONFIG:Debug>:ETL_VERBOSE_ERRORS>
    $<$<CONFIG:Debug>:ETL_CHECK_PUSH_POP>
)

# Install (optional - remove if not needed)
install(TARGETS ${PROJECT_NAME} DESTINATION bin)