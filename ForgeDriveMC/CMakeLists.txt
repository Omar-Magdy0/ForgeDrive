cmake_minimum_required(VERSION 3.20)

set(THIRDPARTY_DIR ${CMAKE_SOURCE_DIR}/../thirdparty)
set(DEVICE "STM32F401xC" CACHE STRING "Target MCU or platform") # Select target platform
set(FDMC_DIR ${CMAKE_SOURCE_DIR}/fdmc)
set(FDLIB_DIR ${CMAKE_SOURCE_DIR}/../fdlib)
set(PMSMCONTROL_DIR ${CMAKE_SOURCE_DIR}/PmsmControl)
set(CMSIS_DSP_DIR ${THIRDPARTY_DIR}/CMSIS-DSP)
set(CMSIS_CORE_DIR ${THIRDPARTY_DIR}/CMSIS)
set(FREERTOS_DIR "${THIRDPARTY_DIR}/FreeRTOS/FreeRTOS-Kernel")
set(Python3_DIR "${CMAKE_SOURCE_DIR}/../venv")
set(Python3_EXECUTABLE "${Python3_DIR}/bin/python")
set(GEN_SCRIPT "${FDLIB_DIR}/driver/gen/generate.py")
set(CONFIG_FILE "${CMAKE_CURRENT_SOURCE_DIR}/fd_driver_conf.py")
set(FDDRV_DIR "${CMAKE_CURRENT_SOURCE_DIR}/fd_driver_gen")
set(NANOPB_DIR ${THIRDPARTY_DIR}/nanopb)
# Prevent in-source builds
if(CMAKE_SOURCE_DIR STREQUAL CMAKE_BINARY_DIR)
    message(FATAL_ERROR "
    In-source builds are not allowed!
    Please use:
    mkdir build && cd build && cmake ..
    ")
endif()

###### FD_DRIVER GENERATION START #######

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
message(STATUS "Result ${GEN_RESULT}\n" "stdout:\n${GEN_OUTPUT}\n" "stderr:\n${GEN_ERROR}\n")

########### FD_DRIVER GENERATION END ################

####platform specific settings
# SET TOOLCHAIN BEFORE PROJECT

set(CMAKE_TOOLCHAIN_FILE "${FDDRV_DIR}/toolchain.cmake")
set(CMAKE_C_STANDARD 23)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
include(${FDDRV_DIR}/device.cmake)
message(STATUS "Target device: ${DEVICE_NAME}")
message(STATUS "Target platform: ${DEVICE_PLATFORM}")
project(ForgeDriveMC LANGUAGES C CXX ASM)
message("Build type: " ${CMAKE_BUILD_TYPE})
add_library(fd_freertos STATIC
    ${FREERTOS_DIR}/tasks.c
    ${FREERTOS_DIR}/queue.c
    ${FREERTOS_DIR}/list.c
)
target_compile_definitions(fd_freertos PRIVATE _GNU_SOURCE)
target_include_directories(fd_freertos PUBLIC
    ${FREERTOS_DIR}/include
)
add_compile_definitions($<$<CONFIG:Debug>:FD_DEBUG>)
add_subdirectory("${FDDRV_DIR}")
include("platform/${DEVICE_PLATFORM}/platform.cmake")

########### PROTO_BUF BEGIN #########################

set(FDRPC_DIR ${FDLIB_DIR}/middleware/fdrpc)
file(COPY ${FDRPC_DIR}/fdrpc_pb2.py DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
configure_file(${FDRPC_DIR}/protoc-gen-fdrpc.py ${CMAKE_CURRENT_BINARY_DIR}/protoc-gen-fdrpc @ONLY)
file(CHMOD ${CMAKE_CURRENT_BINARY_DIR}/protoc-gen-fdrpc PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE)
find_program(PROTOC_EXECUTABLE protoc REQUIRED)
execute_process(
    COMMAND
        ${PROTOC_EXECUTABLE}
        --plugin=protoc-gen-nanopb=${Python3_DIR}/bin/protoc-gen-nanopb
        --plugin=protoc-gen-fdrpc=${CMAKE_CURRENT_BINARY_DIR}/protoc-gen-fdrpc
        -I ${FDMC_DIR} -I ${FDRPC_DIR}
        --nanopb_out=${FDMC_DIR}
        --nanopb_opt=-s,mangle_names:M_STRIP_PACKAGE
        --nanopb_opt=-s,long_names:false
        --fdrpc_out=${FDMC_DIR}
        fdmc.proto
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    RESULT_VARIABLE GEN_RESULT
    OUTPUT_VARIABLE GEN_OUTPUT
    ERROR_VARIABLE GEN_ERROR
)
message(STATUS "Result ${GEN_RESULT}\n" "stdout:\n${GEN_OUTPUT}\n" "stderr:\n${GEN_ERROR}\n")
########### PROTO_BUF END #########################

# Include directories
include_directories(
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/PosDriver
    ${FDLIB_DIR}
    ${FDLIB_DIR}/math
    ${FDLIB_DIR}/middleware
    ${FDDRV_DIR}
    ${FDRPC_DIR}
    ${FDMC_DIR}
    ${PMSMCONTROL_DIR}
    ${CMSIS_DSP_DIR}/Include
    ${CMSIS_CORE_DIR}/Include  
    ${PLATFORM_DIR} 
    ${NANOPB_DIR}
    ${THIRDPARTY_DIR}/fonts/FontAwesome7
)
target_compile_definitions(fd_drv PUBLIC SILSERVER_IP_ADDRESS="127.0.0.1")
target_include_directories(fd_drv PRIVATE ${PLATFORM_DIR})
add_subdirectory(${THIRDPARTY_DIR}/etl etl)
add_subdirectory(${CMSIS_DSP_DIR}/Source ${CMAKE_CURRENT_BINARY_DIR}/cmsis_dsp)

if(DEVICE_PLATFORM STREQUAL "HOST")
list(APPEND POSDRIVER "${CMAKE_CURRENT_SOURCE_DIR}/PosDriver/PosDriverSIL.cpp")
else()
set(POSDRIVER "${CMAKE_CURRENT_SOURCE_DIR}/PosDriver/PosDriverVIRT.cpp")
endif()

set(APPLICATION_SRC
    ${PMSMCONTROL_DIR}/PmsmControlCore.cpp
    ${PMSMCONTROL_DIR}/PmsmControlCore_Foc.cpp
    ${PMSMCONTROL_DIR}/PmsmControlCore_SComm.cpp
    ${PMSMCONTROL_DIR}/Sys.cpp
    ${PMSMCONTROL_DIR}/FdmcRpcDef.cpp
    ${POSDRIVER}
    ${PLATFORM_SRC}
    ${CMAKE_SOURCE_DIR}/main.cpp
)

set(MIDDLEWARE_SRC    
    ${FDLIB_DIR}/middleware/Abf.c
    ${FDMC_DIR}/fdmc.pb.c
    ${NANOPB_DIR}/pb_common.c
    ${NANOPB_DIR}/pb_decode.c
    ${NANOPB_DIR}/pb_encode.c
)

target_link_libraries(fd_freertos PUBLIC fd_drv)
add_executable(${PROJECT_NAME} ${APPLICATION_SRC} ${CORE_SRC} ${MIDDLEWARE_SRC})
include(${FDDRV_DIR}/post_build.cmake OPTIONAL)
target_link_libraries(${PROJECT_NAME} PRIVATE
    fd_drv
    fd_freertos
    ${PLATFORM_LIBRARIES}
    CMSISDSP
    etl::etl
)
target_compile_definitions(${PROJECT_NAME} PRIVATE
    ${PLATFORM_DEFINITIONS}
    PB_NO_ERRMSG
    PB_BUFFER_ONLY
    FONTS_DIR="${THIRDPARTY_DIR}/fonts"
    SILSERVER_IP_ADDRESS="127.0.0.1"
)

