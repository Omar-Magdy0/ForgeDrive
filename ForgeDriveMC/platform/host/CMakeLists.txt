cmake_minimum_required(VERSION 3.20)

# Host platform module used by top-level CMake via add_subdirectory(platform/host).

set(DEVICE_DEFINITIONS
    USE_FULL_ASSERT
)

set(CMSISCORE ${THIRDPARTY_DIR}/CMSIS)
set(IMGUI_DIR ${THIRDPARTY_DIR}/imgui)
set(IMPLOT_DIR ${THIRDPARTY_DIR}/implot)
set(IMGUIFD_DIR ${THIRDPARTY_DIR}/ImGuiFileDialog)

set(HOST_COMMON_INCLUDES
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/hconf
    ${IMGUI_DIR}
    ${IMGUI_DIR}
    ${IMGUI_DIR}/backends
    ${IMPLOT_DIR}
    ${IMGUIFD_DIR}
    ${EIGEN_DIR}
    ${FDLIB_DIR}/fd
    ${FDLIB_DIR}/middleware
    ${FDDRV_DIR}
)

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

file(GLOB platform_src
    ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/*.c
)

find_package(Threads REQUIRED)


find_package(PkgConfig REQUIRED)
find_package(OpenGL REQUIRED)
find_package(glfw3 REQUIRED)
find_package(HDF5 REQUIRED COMPONENTS C CXX)

add_library(PLATFORM STATIC)
target_sources(PLATFORM PRIVATE
    ${IMGUI_SRC}
    ${platform_src}
)

target_include_directories(PLATFORM PUBLIC 
    ${HOST_COMMON_INCLUDES}
    ${HDF5_INCLUDE_DIRS}
    ${CMSISCORE}/Include
)

target_compile_definitions(PLATFORM PUBLIC
    ${DEVICE_DEFINITIONS}
    projCOVERAGE_TEST=0
    projENABLE_TRACING=0
)

target_link_libraries(PLATFORM PUBLIC ${HDF5_LIBRARIES} ${HDF5_CXX_LIBRARIES} Threads::Threads CMSISDSP OpenGL::GL glfw)


