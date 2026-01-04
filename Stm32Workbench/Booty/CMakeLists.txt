# Automatically derive target name from directory
get_filename_component(TARGET_NAME ${CMAKE_CURRENT_SOURCE_DIR} NAME)

set(TARGET_ELF  ${TARGET_NAME}.elf)
set(TARGET_HEX  ${TARGET_NAME}.hex)
set(TARGET_BIN  ${TARGET_NAME}.bin)
set(TARGET_MAP  ${TARGET_NAME}.map)

# Include the target-specific config (must be named <dirname>.cmake)
include(${TARGET_NAME}.cmake)

add_executable(${TARGET_ELF}
    ${BL_SOURCES}
    ${BL_HAL_SOURCES}
    ${BL_CMSIS_SOURCES}
)

# === Target-specific compile definitions ===
target_compile_definitions(${TARGET_ELF} PRIVATE
    ${BL_DEFINITIONS}   # Expands to APP_DEFINITIONS or BOOT_DEFINITIONS
)

target_compile_options(${TARGET_ELF} PRIVATE
    -march=armv7-m
    -fdata-sections
    -ffunction-sections
    -Os
    -g3
    -flto
    -Wall
    -Wextra
    # -Werror # not yet...
)

# === Target-specific include directories ===
target_include_directories(${TARGET_ELF} PRIVATE
    inc
    src
)

target_link_options(${TARGET_ELF} PRIVATE
    -T${BL_LINKER_SCRIPT}
    -L${CMAKE_CURRENT_SOURCE_DIR} # to find sections.ld linker script
    -nostartfiles
    -Wl,--gc-sections
    -Wl,--print-memory-usage
    -Wl,-Map=${TARGET_MAP}
)

add_custom_command(TARGET ${TARGET_ELF} POST_BUILD
    COMMAND ${CMAKE_OBJCOPY} -O ihex   ${TARGET_ELF} ${TARGET_HEX}
    COMMAND ${CMAKE_OBJCOPY} -O binary ${TARGET_ELF} ${TARGET_BIN}
    COMMAND ${CMAKE_SIZE} ${TARGET_ELF}
    COMMENT "Generating ${TARGET_HEX} and ${TARGET_BIN}"
)

# Copy artifacts to known location
add_custom_command(TARGET ${TARGET_ELF} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_CURRENT_BINARY_DIR}/${TARGET_BIN}
        ${CMAKE_CURRENT_SOURCE_DIR}/../Gen3Images/${TARGET_NAME}-TEMP.bin   # Fixed known location
    COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_CURRENT_BINARY_DIR}/${TARGET_ELF}
        ${CMAKE_CURRENT_SOURCE_DIR}/../Gen3Images/${TARGET_NAME}-TEMP.elf   # Fixed known location
    COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_CURRENT_BINARY_DIR}/${TARGET_HEX}
        ${CMAKE_CURRENT_SOURCE_DIR}/../Gen3Images/${TARGET_NAME}-TEMP.hex   # Fixed known location
)
