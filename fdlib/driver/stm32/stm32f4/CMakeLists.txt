cmake_minimum_required(VERSION 3.20)
# Include device-specific CMake configuration

# Set device specific definitions
set(DEVICE_DEFINITIONS
    USE_HAL_DRIVER
    STM32F{{ device.product |upper }}x{{ device.density_code | upper }}
    USE_FULL_LL_DRIVER
)

# Path configurations
set(HAL_CONFIG_DIR ${CMAKE_CURRENT_SOURCE_DIR})
set(STM32CMX_DIR ${THIRDPARTY_DIR}/stm32)
set(STM32F4xx_HAL_DRIVER_DIR ${THIRDPARTY_DIR}/stm32/f4/STM32F4xx_HAL_Driver)
set(CMSIS_DEVICE_DIR ${THIRDPARTY_DIR}/CMSIS/Device/ST/STM32F4xx/Include)
set(CMSIS_CORE_DIR ${THIRDPARTY_DIR}/CMSIS)
set(USBD_DIR ${THIRDPARTY_DIR}/stm32/usbd)

set(STARTUP_FILE ${CMSIS_CORE_DIR}/Device/ST/STM32F4xx/Source/Templates/gcc/startup_stm32f{{ device.product | lower }}x{{ device.density_code | lower }}.s)
set(SYSTEM_FILE ${CMSIS_CORE_DIR}/Device/ST/STM32F4xx/Source/Templates/system_stm32f4xx.c)
# Common include directories for all STM32 targets
set(STM32_COMMON_INCLUDES
    ${CMSIS_CORE_DIR}/Include
    ${CMSIS_CORE_DIR}/Core/Include
    ${STM32F4xx_HAL_DRIVER_DIR}/Inc
    ${STM32F4xx_HAL_DRIVER_DIR}/Inc/Legacy  # Important for HAL/LL compatibility
    ${CMSIS_DEVICE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${HAL_CONFIG_DIR}
    ${USBD_DIR}/Core/Inc
    ${CMAKE_CURRENT_SOURCE_DIR}/usbd
    ${USBD_DIR}/Class/XCH/Inc
    ${USBD_DIR}/App
    ${STM32CMX_DIR}/f4/Target
    ${FDLIB_DIR}/fd
)

# ===== HAL DRIVERS (for complex protocols) =====
set(STM32_Drivers_Src
    # Core HAL
    ${STM32F4xx_HAL_DRIVER_DIR}/Src/stm32f4xx_hal.c
    ${STM32F4xx_HAL_DRIVER_DIR}/Src/stm32f4xx_ll_usb.c
    ${STM32F4xx_HAL_DRIVER_DIR}/Src/stm32f4xx_hal_pcd.c
    ${STM32F4xx_HAL_DRIVER_DIR}/Src/stm32f4xx_hal_pcd_ex.c

    ${STM32F4xx_HAL_DRIVER_DIR}/Src/stm32f4xx_hal_rcc.c
    ${STM32F4xx_HAL_DRIVER_DIR}/Src/stm32f4xx_hal_rcc_ex.c
    ${STM32F4xx_HAL_DRIVER_DIR}/Src/stm32f4xx_hal_gpio.c
    ${STM32F4xx_HAL_DRIVER_DIR}/Src/stm32f4xx_hal_cortex.c
    ${STM32F4xx_HAL_DRIVER_DIR}/Src/stm32f4xx_hal_dma.c
    ${STM32F4xx_HAL_DRIVER_DIR}/Src/stm32f4xx_hal_pwr.c
    ${STM32F4xx_HAL_DRIVER_DIR}/Src/stm32f4xx_hal_pwr_ex.c
    ${STM32F4xx_HAL_DRIVER_DIR}/Src/stm32f4xx_hal_flash.c
    ${STM32F4xx_HAL_DRIVER_DIR}/Src/stm32f4xx_hal_flash_ex.c
    ${STM32F4xx_HAL_DRIVER_DIR}/Src/stm32f4xx_hal_tim.c     
    ${STM32F4xx_HAL_DRIVER_DIR}/Src/stm32f4xx_hal_tim_ex.c  
    # Communication HAL (add as needed)
    ${STM32F4xx_HAL_DRIVER_DIR}/Src/stm32f4xx_hal_uart.c
    ${STM32F4xx_HAL_DRIVER_DIR}/Src/stm32f4xx_hal_can.c

        # Core LL
    ${STM32F4xx_HAL_DRIVER_DIR}/Src/stm32f4xx_ll_utils.c
    ${STM32F4xx_HAL_DRIVER_DIR}/Src/stm32f4xx_ll_rcc.c
    ${STM32F4xx_HAL_DRIVER_DIR}/Src/stm32f4xx_ll_gpio.c
    ${STM32F4xx_HAL_DRIVER_DIR}/Src/stm32f4xx_ll_dma.c
    
    # Motor Control LL (add as needed)
    ${STM32F4xx_HAL_DRIVER_DIR}/Src/stm32f4xx_ll_tim.c
    ${STM32F4xx_HAL_DRIVER_DIR}/Src/stm32f4xx_ll_adc.c
    ${STM32F4xx_HAL_DRIVER_DIR}/Src/stm32f4xx_ll_usart.c
)


set(STM32CMX_Src
    ${STM32CMX_DIR}/syscalls.c
    ${STM32CMX_DIR}/sysmem.c
    ${STARTUP_FILE}
    ${SYSTEM_FILE}
)

set(USBD_Src
    ${USBD_DIR}/Core/Src/usbd_core.c
    ${USBD_DIR}/Core/Src/usbd_ctlreq.c
    ${USBD_DIR}/Core/Src/usbd_ioreq.c
    ${USBD_DIR}/Class/XCH/Src/usbd_xch.c
)

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
    ${STM32CMX_Src}
    ${STM32_Drivers_Src}
    ${USBD_Src}
    ${fd_drv_local}
)

target_include_directories(fd_drv PUBLIC ${STM32_COMMON_INCLUDES} ${fd_drv_includes})
target_compile_definitions(fd_drv PUBLIC ${DEVICE_DEFINITIONS})

