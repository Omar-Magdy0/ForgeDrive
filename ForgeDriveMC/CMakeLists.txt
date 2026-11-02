cmake_minimum_required(VERSION 3.20)

set(THIRDPARTY_DIR ${CMAKE_SOURCE_DIR}/../thirdparty)

# Select target platform
set(DEVICE "STM32F401xC" CACHE STRING "Target MCU or platform")

set(FDLIB_DIR ${CMAKE_SOURCE_DIR}/../fdlib)
set(PMSMCONTROL_DIR ${CMAKE_SOURCE_DIR}/PmsmControl)
set(CMSIS_DSP_DIR ${THIRDPARTY_DIR}/CMSIS-DSP)
set(CMSIS_CORE_DIR ${THIRDPARTY_DIR}/CMSIS)
set(FREERTOS_DIR "${THIRDPARTY_DIR}/FreeRTOS/FreeRTOS-Kernel")
set(TINYCBOR_DIR "${THIRDPARTY_DIR}/tinycbor")
# Prevent in-source builds
if(CMAKE_SOURCE_DIR STREQUAL CMAKE_BINARY_DIR)
    message(FATAL_ERROR "
    In-source builds are not allowed!
    Please use:
    mkdir build && cd build && cmake ..
    ")
endif()

###### FD_DRIVER GENERATION START #######

set(Python3_EXECUTABLE "${CMAKE_SOURCE_DIR}/../venv/bin/python3")
set(GEN_SCRIPT "${FDLIB_DIR}/driver/gen/generate.py")
set(CONFIG_FILE "${CMAKE_CURRENT_SOURCE_DIR}/fd_driver_conf.py")
set(FDDRV_DIR "${CMAKE_CURRENT_SOURCE_DIR}/fd_driver_gen")

# Optional: Glob templates or dependencies inside fdlib or local directory for auto-reconfiguration
file(GLOB_RECURSE TEMPLATE_FILES 
    "${FDLIB_DIR}/driver/*.c"
    "${FDLIB_DIR}/driver/*.h"
    "${FDLIB_DIR}/driver/*.j2"
)
# Tell CMake to re-configure if the generator script, config, or templates change
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS 
    "${GEN_SCRIPT}"
    "${CONFIG_FILE}"
    ${TEMPLATE_FILES}
)

# Run your exact generator command at configure time
execute_process(
    COMMAND ${Python3_EXECUTABLE} ${GEN_SCRIPT} ${CONFIG_FILE} -d ${DEVICE} -o ${FDDRV_DIR}
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    RESULT_VARIABLE GEN_RESULT
    OUTPUT_VARIABLE GEN_OUTPUT
    ERROR_VARIABLE GEN_ERROR
)

message(STATUS
    "Result ${GEN_RESULT}\n"
    "stdout:\n${GEN_OUTPUT}\n"
    "stderr:\n${GEN_ERROR}\n"
)

########### FD_DRIVER GENERATION END ################
###### Serializer GENERATION START #######
set(SER_SCRIPT "${FDLIB_DIR}/middleware/Serializer.py")
set(SER_FILES 
    ${CMAKE_CURRENT_SOURCE_DIR}/PmsmControl/PmsmControlTypes.h
)
set(SER_DIR "${CMAKE_CURRENT_SOURCE_DIR}/SerializedTypes")
# Tell CMake to re-configure if the generator script, config, or templates change
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS
    "${SER_SCRIPT}"
    "${SER_FILES}"
)

execute_process(
    COMMAND "${Python3_EXECUTABLE}" "${SER_SCRIPT}"
            -i ${SER_FILES}
            -o "${SER_DIR}"
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
    RESULT_VARIABLE GEN_RESULT
    OUTPUT_VARIABLE GEN_OUTPUT
    ERROR_VARIABLE GEN_ERROR
)

if(NOT GEN_RESULT EQUAL 0)
    message(FATAL_ERROR
        "Serializer generation failed!\n"
        "Exit code: ${GEN_RESULT}\n"
        "Output:\n${GEN_OUTPUT}\n"
        "Errors:\n${GEN_ERROR}"
    )
endif()

if(GEN_OUTPUT)
    message(STATUS "${GEN_OUTPUT}")
endif()
########### Serializer GENERATION END ################


####platform specific settings
# SET TOOLCHAIN BEFORE PROJECT

set(CMAKE_TOOLCHAIN_FILE "${FDDRV_DIR}/toolchain.cmake")
include(${FDDRV_DIR}/device.cmake)
message(STATUS "Target device: ${DEVICE_NAME}")
message(STATUS "Target platform: ${DEVICE_PLATFORM}")
project(ForgeDriveMC LANGUAGES C CXX ASM)
add_library(fd_freertos STATIC
    ${FREERTOS_DIR}/tasks.c
    ${FREERTOS_DIR}/queue.c
    ${FREERTOS_DIR}/list.c
)
target_include_directories(fd_freertos PUBLIC
    ${FREERTOS_DIR}/include
)
add_compile_definitions($<$<CONFIG:Debug>:FD_DEBUG>)
add_subdirectory("${FDDRV_DIR}")
add_subdirectory("platform/${DEVICE_PLATFORM}")
# Setup cmake module path and compiler settings
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake")
message("Build type: " ${CMAKE_BUILD_TYPE})

# Set C++ standard
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS ON)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Control where files go WITHIN the build directory
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# Include directories
include_directories(
    ${CMAKE_SOURCE_DIR}
    ${FDLIB_DIR}
    ${FDLIB_DIR}/middleware
    ${FDDRV_DIR}
    ${PMSMCONTROL_DIR}
    ${PMSMCONTROL_DIR}/PosDriver
    ${CMSIS_DSP_DIR}/Include
    ${CMSIS_CORE_DIR}/Include   
)
# Link with platform EARLY to get include paths





add_subdirectory(${TINYCBOR_DIR} ${CMAKE_CURRENT_BINARY_DIR}/tinycbor)
add_subdirectory(${THIRDPARTY_DIR}/etl etl)
add_subdirectory(${CMSIS_DSP_DIR}/Source ${CMAKE_CURRENT_BINARY_DIR}/cmsis_dsp)

file(GLOB_RECURSE MIDDLEWARE_SRC    
    ${FDLIB_DIR}/middleware/*.cpp
    ${FDLIB_DIR}/middleware/*.c
)



if(DEVICE_PLATFORM STREQUAL "HOST")
list(APPEND POSDRIVER "${PMSMCONTROL_DIR}/PosDriver/PosDriverSIL.cpp")
else()
set(POSDRIVER "${PMSMCONTROL_DIR}/PosDriver/PosDriverVIRT.cpp")
endif()


set(APPLICATION_SRC
    ${PMSMCONTROL_DIR}/PmsmControlCore.cpp
    ${PMSMCONTROL_DIR}/PmsmControlCore_Foc.cpp
    ${PMSMCONTROL_DIR}/PmsmControlCore_SComm.cpp
    ${POSDRIVER}
    ${CMAKE_SOURCE_DIR}/main.cpp
    ${CMAKE_SOURCE_DIR}/sys.cpp
)
file(GLOB_RECURSE CORE_SRC
    ${FDLIB_DIR}/fd/*.cpp
    ${FDLIB_DIR}/fd/*.c
)
target_link_libraries(fd_freertos PUBLIC fd_drv)
add_executable(${PROJECT_NAME} ${APPLICATION_SRC} ${CORE_SRC} ${MIDDLEWARE_SRC})
include(${FDDRV_DIR}/post_build.cmake OPTIONAL)
target_link_libraries(${PROJECT_NAME} PRIVATE
    fd_drv
    tinycbor
    fd_freertos
    CMSISDSP
    PLATFORM
    etl::etl
)

# BUILD Platform Tied TEST CODE
if(DEVICE_PLATFORM STREQUAL "HOST")
    target_compile_definitions(${PROJECT_NAME} PRIVATE
        ETL_THROW_EXCEPTIONS
        ETL_VERBOSE_ERRORS
        ETL_CHECK_PUSH_POP
    )
endif()
