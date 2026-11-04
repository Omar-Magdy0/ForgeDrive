cmake_minimum_required(VERSION 3.20)

# Host platform module used by top-level CMake via add_subdirectory(platform/host).

set(CMSISCORE ${THIRDPARTY_DIR}/CMSIS)
set(IMGUI_DIR ${THIRDPARTY_DIR}/imgui)
set(IMPLOT_DIR ${THIRDPARTY_DIR}/implot)
set(IMGUIFD_DIR ${THIRDPARTY_DIR}/ImGuiFileDialog)

set(HOST_COMMON_INCLUDES
    ${CMAKE_CURRENT_LIST_DIR}
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

set(platform_src
    ${CMAKE_CURRENT_LIST_DIR}/platform.cpp
    ${CMAKE_CURRENT_LIST_DIR}/fd_mc3p.cpp
)

find_package(Threads REQUIRED)
find_package(PkgConfig REQUIRED)
find_package(OpenGL REQUIRED)
find_package(glfw3 REQUIRED)
find_package(HDF5 REQUIRED COMPONENTS C CXX)

target_sources(fd_freertos PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}/port.cpp
    ${FREERTOS_DIR}/portable/ThirdParty/GCC/Posix/utils/wait_for_event.c
)

target_include_directories(fd_freertos PUBLIC
    ${FREERTOS_DIR}/portable/ThirdParty/GCC/Posix
    ${FREERTOS_DIR}/portable/ThirdParty/GCC/Posix/utils
)

set(PLATFORM_SRC ${IMGUI_SRC} ${platform_src} )
set(PLATFORM_DIR  
    ${HOST_COMMON_INCLUDES}
    ${HDF5_INCLUDE_DIRS}
    ${CMSISCORE}/Include
)
set(PLATFORM_DEFINITIONS
    projCOVERAGE_TEST=0
    projENABLE_TRACING=0
    ETL_THROW_EXCEPTIONS
    ETL_VERBOSE_ERRORS
    ETL_CHECK_PUSH_POP
)
set(PLATFORM_LIBRARIES
    ${HDF5_LIBRARIES}
    ${HDF5_CXX_LIBRARIES}
    Threads::Threads
    OpenGL::GL
    glfw
    CMSISDSP
)



