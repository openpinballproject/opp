cmake_minimum_required(VERSION 3.15)

# Main project (name is just for IDE/display)
project(OPP LANGUAGES C ASM)

# Load toolchain (passed from command line)
# Note: Do NOT set CMAKE_TOOLCHAIN_FILE here — it's set via -DCMAKE_TOOLCHAIN_FILE

# Common settings shared across all targets
add_compile_definitions(
)

add_link_options(
    -march=armv7-m
    -specs=nosys.specs
    -lc -lm -lnosys
    -Wl,--gc-sections
    -Wl,--print-memory-usage
)

# Include common directories
include_directories(
    OppGen3/Drivers/STM32F1xx_HAL_Driver/Inc
    OppGen3/Inc
)

# Add subdirectories (each adds its own executable target)
add_subdirectory(Booty)
add_subdirectory(OppGen3)

# Optional: Clean entire output directories for specific targets
# (useful if you generate many files)
set_property(
    DIRECTORY ${CMAKE_SOURCE_DIR}
    APPEND
    PROPERTY ADDITIONAL_CLEAN_FILES
        ${CMAKE_BINARY_DIR}/OppGen3
        ${CMAKE_BINARY_DIR}/Booty
)

# =============================================================================
# Linting target using cppcheck
# =============================================================================

find_program(CPPCHECK cppcheck)

if(CPPCHECK)
    # List all source files to lint (across all targets)
    set(ALL_SOURCE_FILES
        ${APP_SOURCES}
        ${BL_SOURCES}
        # Add more directories as needed
    )

    add_custom_target(lint
        COMMAND ${CPPCHECK}
            --enable=all
            --suppress=missingIncludeSystem          # Suppress warnings about system headers
            --suppress=unmatchedSuppression          # Clean output
            --suppress=knownConditionTrueFalse       # Optional: reduce noise
            --inconclusive                                   # Allow inconclusive checks
            --quiet                                          # Only show errors/warnings
            --error-exitcode=1                               # Fail build if issues found
            -I ${CMAKE_SOURCE_DIR}/Inc
            -I ${CMAKE_SOURCE_DIR}/Drivers/STM32F1xx_HAL_Driver/Inc
            -I ${CMAKE_SOURCE_DIR}/Drivers/CMSIS/Include
            -I ${CMAKE_SOURCE_DIR}/Drivers/CMSIS/Device/ST/STM32F1xx/Include
            -I ${CMAKE_SOURCE_DIR}/Middlewares/ST/STM32_USB_Device_Library/Core/Inc
            -I ${CMAKE_SOURCE_DIR}/Middlewares/ST/STM32_USB_Device_Library/Class/CDC/Inc
            ${ALL_SOURCE_FILES}
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        COMMENT "Running cppcheck static analysis..."
        VERBATIM
    )
else()
    message(WARNING "cppcheck not found — lint target will be unavailable. Install with: sudo apt install cppcheck")
endif()
# find_program(CLANG_TIDY clang-tidy)
#
# if(CLANG_TIDY)
#     add_custom_target(lint
#         COMMAND ${CLANG_TIDY}
#             -checks='*,-llvm-*,-google-*,-cert-*'  # Customize checks
#             -header-filter=.*
#             --quiet
#             ${ALL_SOURCE_FILES}
#             --
#             -I${CMAKE_SOURCE_DIR}/Inc
#             -I${CMAKE_SOURCE_DIR}/Drivers/STM32F1xx_HAL_Driver/Inc
#             # ... other includes
#         WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
#         COMMENT "Running clang-tidy..."
#     )
# endif()
