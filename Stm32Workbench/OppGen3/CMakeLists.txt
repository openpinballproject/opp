# Automatically derive target name from directory
get_filename_component(TARGET_NAME ${CMAKE_CURRENT_SOURCE_DIR} NAME)

set(TARGET_ELF  ${TARGET_NAME}.elf)
set(TARGET_HEX  ${TARGET_NAME}.hex)
set(TARGET_BIN  ${TARGET_NAME}.bin)
set(TARGET_MAP  ${TARGET_NAME}.map)

# Include the target-specific config (must be named <dirname>.cmake)
include(${TARGET_NAME}.cmake)

add_executable(${TARGET_ELF}
    ${APP_SOURCES}
    ${APP_MIDDLEWARE_SOURCES}
    ${APP_HAL_SOURCES}
    ${APP_CMSIS_SOURCES}
)

# === Target-specific compile definitions ===
target_compile_definitions(${TARGET_ELF} PRIVATE
    ${APP_DEFINITIONS}   # Expands to APP_DEFINITIONS or BOOT_DEFINITIONS
)

target_compile_options(${TARGET_ELF} PRIVATE
    -march=armv7-m
    -fdata-sections
    -ffunction-sections
    -Wall
    -Wextra
    # -Werror # Not yet...

    $<$<CONFIG:Debug>:-Og -g3 -gdwarf-4>
    $<$<CONFIG:Release>:-Os>
)

# === Target-specific include directories ===
target_include_directories(${TARGET_ELF} PRIVATE
    Inc
    Drivers/STM32F1xx_HAL_Driver/Inc
    Drivers/STM32F1xx_HAL_Driver/Inc/Legacy
    Middlewares/ST/STM32_USB_Device_Library/Core/Inc
    Middlewares/ST/STM32_USB_Device_Library/Class/CDC/Inc
    Drivers/CMSIS/Device/ST/STM32F1xx/Include
    Drivers/CMSIS/Include
)

target_link_options(${TARGET_ELF} PRIVATE
    -T${APP_LINKER_SCRIPT}
    -specs=nosys.specs
    -specs=nano.specs
    -Wl,--gc-sections
    -Wl,--print-memory-usage
    -Wl,-Map=${TARGET_MAP}
)

# Make sure the bootloader binary exists before linking the app
add_dependencies(${TARGET_ELF} Booty.elf)

add_custom_command(TARGET ${TARGET_ELF} POST_BUILD
    COMMAND ${CMAKE_OBJCOPY} -O ihex   ${TARGET_ELF} ${TARGET_HEX}
    COMMAND ${CMAKE_OBJCOPY} -O binary ${TARGET_ELF} ${TARGET_BIN}

    # Custom CRC update (requires python2 or python3)
    COMMAND python3 ${CMAKE_CURRENT_SOURCE_DIR}/scripts/updCrc.py
            -hex=${TARGET_HEX}
            -crc=crc32.bin

    # Inject CRC into ELF section
    COMMAND ${CMAKE_OBJCOPY} --update-section .app_crc32_sect=crc32.bin ${TARGET_ELF}

    COMMAND ${CMAKE_SIZE} ${TARGET_ELF}

    COMMENT "Generated hex/bin, updated CRC, and printed size"
)

# Copy artifacts to known location
add_custom_command(TARGET ${TARGET_ELF} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_CURRENT_BINARY_DIR}/${TARGET_ELF}
        ${CMAKE_CURRENT_SOURCE_DIR}/../Gen3Images/${TARGET_NAME}-TEMP.elf   # Fixed known location
    COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_CURRENT_BINARY_DIR}/${TARGET_HEX}
        ${CMAKE_CURRENT_SOURCE_DIR}/../Gen3Images/${TARGET_NAME}-TEMP.hex   # Fixed known location
)
