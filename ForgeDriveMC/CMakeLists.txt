cmake_minimum_required(VERSION 3.20)


# Select target platform
set(PLATFORM_NAME "host" CACHE STRING "Target platform (host, stm32f4)")
set(FDLIB_DIR ${CMAKE_SOURCE_DIR}/../fdlib)
set(THIRDPARTY_DIR ${CMAKE_SOURCE_DIR}/../thirdparty)
set(PMSMCONTROL_DIR ${CMAKE_SOURCE_DIR}/PmsmControl)

####platform specific settings
# SET TOOLCHAIN BEFORE PROJECT
include(platform/${PLATFORM_NAME}/platform.cmake)
project(ForgeDriveMC LANGUAGES C CXX ASM)

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
set(FDDRV_OUT_DIR "${CMAKE_BINARY_DIR}/generated")

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
message(STATUS "Running driver generator for platform: ${PLATFORM_NAME}...")
execute_process(
    COMMAND ${Python3_EXECUTABLE} ${GEN_SCRIPT} ${CONFIG_FILE} -d ${PLATFORM_NAME} -o ${FDDRV_OUT_DIR}
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    RESULT_VARIABLE GEN_RESULT
)

if(NOT GEN_RESULT EQUAL 0)
    message(FATAL_ERROR "Driver code generation failed with exit code ${GEN_RESULT}")
endif()

########### FD_DRIVER GENERATION END ################

# Setup cmake module path and compiler settings
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake")

message("Build type: " ${CMAKE_BUILD_TYPE})

set(CMAKE_BINARY_DIR ${CMAKE_SOURCE_DIR}/build)

# Set C++ standard
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS ON)
set(CMAKE_CXX_STANDARD 20)
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
    ${FDLIB_DIR}/middleware/cortos
    ${FDDRV_OUT_DIR}
    ${PMSMCONTROL_DIR}
)


# Link with platform EARLY to get include paths
add_subdirectory(platform/${PLATFORM_NAME})
add_subdirectory(${THIRDPARTY_DIR}/etl etl)

message(STATUS "LIBRARIES TO LINK: ${PLATFORM_LIB}")

# Add platform includes to global include path
include_directories(${PLATFORM_INCLUDES})

file(GLOB_RECURSE MIDDLEWARE_SRC    
    ${FDLIB_DIR}/middleware/*.cpp
    ${FDLIB_DIR}/middleware/*.c
)

file(GLOB_RECURSE FDDRV_SRC    
    ${FDDRV_OUT_DIR}/*.cpp
    ${FDDRV_OUT_DIR}/*.c
)

set(APPLICATION_SRC
    ${PMSMCONTROL_DIR}/PmsmControl.cpp
    ${PMSMCONTROL_DIR}/PosDriver.cpp
    ${PMSMCONTROL_DIR}/PmsmControlCore.cpp
    ${PMSMCONTROL_DIR}/PmsmControlCore_Foc.cpp
    ${PMSMCONTROL_DIR}/PmsmControlCore_SComm.cpp
    ${CMAKE_SOURCE_DIR}/main.cpp
    ${CMAKE_SOURCE_DIR}/sys.cpp
)


file(GLOB_RECURSE CORE_SRC
    ${FDLIB_DIR}/fd/*.cpp
    ${FDLIB_DIR}/fd/*.c
)


# Create executable
add_executable(${PROJECT_NAME} ${APPLICATION_SRC} ${CORE_SRC} ${MIDDLEWARE_SRC} ${FDDRV_SRC})

# Add eld sources directly to the executable to ensure strong IRQ handlers
# override weak startup symbols (avoids linker archive symbol resolution issues)
target_sources(${PROJECT_NAME} PRIVATE ${PLATFORM_SOURCE})

target_link_libraries(${PROJECT_NAME} PRIVATE
    ${PLATFORM_INTERFACE}
    ${PLATFORM_LIB}
    ${TOOLCHAIN_LINK_LIBRARIES}
    etl::etl
)

# BUILD Platform Tied TEST CODE
if(PLATFORM_NAME STREQUAL "host")
    target_compile_definitions(${PROJECT_NAME} PRIVATE
        PLATFORM_HOST
        ETL_THROW_EXCEPTIONS
        ETL_VERBOSE_ERRORS
        ETL_CHECK_PUSH_POP
    )
endif()



if(NOT PLATFORM_NAME STREQUAL "host")
# ADD THIS: Show size automatically after every build
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E echo "=== Memory Usage ==="
    COMMAND ${CMAKE_SIZE} --format=berkeley $<TARGET_FILE:${PROJECT_NAME}>
    COMMENT "Displaying memory usage"
)

# Size target for checking memory usage
add_custom_target(size
    DEPENDS ${PROJECT_NAME}
    COMMAND ${CMAKE_COMMAND} -E echo "=== Detailed Memory Analysis ==="
    COMMAND ${CMAKE_SIZE} --format=berkeley $<TARGET_FILE:${PROJECT_NAME}>
)

# Convert output to hex and binary
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_OBJCOPY} -O ihex $<TARGET_FILE:${PROJECT_NAME}> ${PROJECT_NAME}.hex
)
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_OBJCOPY} -O binary $<TARGET_FILE:${PROJECT_NAME}> ${PROJECT_NAME}.bin
)
endif()
