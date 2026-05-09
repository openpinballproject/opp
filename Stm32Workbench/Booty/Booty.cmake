# Bootloader CMake file for OppGen3 project

set(BL_DEFINITIONS
    STM32F1
    STM32F103x6
    USBD_SOF_DISABLED
)

set(BL_HAL_SOURCES
)

set(BL_CMSIS_SOURCES
)

set(BL_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/src/startup.s
    ${CMAKE_CURRENT_SOURCE_DIR}/src/usbd_devfs_asm.S
    ${CMAKE_CURRENT_SOURCE_DIR}/src/booty.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/cdc_loop.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/stdlflash.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/usbd_core.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/usbd_devfs.c
)

set(BL_LINKER_SCRIPT
    ${CMAKE_CURRENT_SOURCE_DIR}/stm32f103x6.ld
)
