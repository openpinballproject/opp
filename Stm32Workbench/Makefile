# Makefile for multi-target STM32 CMake project (app + bootloader)
# Build Structure
#   Stm32Workbench/
#   ├── Makefile                           # Build flow entry point (this file)
#   ├── CMakeLists.txt                     # Top-level CMake file
#   ├── arm-none-eabi-gcc.cmake            # Toolchain file
#   ├── Booty/
#   │   ├── CMakeLists.txt                 # Adds bootloader target, target-specific options
#   │   ├── Booty.cmake                    # Bootloader-specific files and settings
#   │   ├── src/
#   │   │   ├── booty.c
#   │   │   └── ...
#   │   ├── inc/
#   │   └── stm32F103x6.ld                 # Bootloader linker script
#   ├── OppGen3/
#   │   ├── CMakeLists.txt                 # Adds app target, target-specific options
#   │   ├── OppGen3.cmake                  # App-specific files and settings
#   │   ├── src/
#   │   │   ├── main.c
#   │   │   └── ...
#   │   ├── Inc/
#   |   ├── Drivers/
#   │   |   ├── STM32F1xx_HAL_Driver/
#   │   |   └── CMSIS/
#   |   ├── Middlewares/ST/STM32+USB_Device_Library/
#   │   |   |   ├── Class/
#   │   |   |   |   └── CDC/
#   │   |   |   └── Core/
#   │   └── STM32F103C8Tx_FLASH.ld.template    # Template for build-generated linker script
#   └── _logs/                             # build-generated logs
#   └── _build/                            # build-generated artifacts

# Primary workflow (recommended):
#   make setup                # Configure once
#   cd _build
#   ninja                     # Fast native builds
#   ninja OppGen3.elf
#
# Top-level convenience:
#   make OppGen3              # Auto-setup + build app
#   make Booty                # Auto-setup + build bootloader
#   make all                  # Build both
#   make clean                # Remove build directory

BUILD_DIR     := _build
GENERATOR     := Ninja
BUILD_TYPE    ?= Debug
TOOLCHAIN_FILE := arm-none-eabi-gcc.cmake

# Logging
LOG_DIR       := _logs
TIMESTAMP     := $(shell date +%Y-%m-%d_%H-%M-%S)

$(shell mkdir -p $(LOG_DIR))

# Default: build both
all: Booty OppGen3

# ------------------------------------------------------------------
# Top-level convenience targets – auto-setup + native build
# ------------------------------------------------------------------
OppGen3 Booty: setup
	@echo "Building $@... (log: $(LOG_DIR)/$@-$(TIMESTAMP).log)"
	@cd $(BUILD_DIR) && ninja $@.elf 2>&1 | tee ../$(LOG_DIR)/$@-$(TIMESTAMP).log
	@echo "$@ build complete → $(BUILD_DIR)/$@/$@.elf"

lint: setup
	@echo "Running lint..."
	@cd $(BUILD_DIR) && ninja lint

# ------------------------------------------------------------------
# Setup: Configure CMake (only if needed)
# ------------------------------------------------------------------
setup: $(BUILD_DIR)/CMakeCache.txt
	@echo "Project ready in $(BUILD_DIR)/"
	@echo "Tip: cd $(BUILD_DIR) && ninja   # for fastest workflow"

$(BUILD_DIR)/CMakeCache.txt:
	@echo "Configuring CMake in $(BUILD_DIR)..."
	@rm -rf $(BUILD_DIR)
	@mkdir -p $(BUILD_DIR)
	@cd $(BUILD_DIR) && cmake .. \
		-G $(GENERATOR) \
		-DCMAKE_TOOLCHAIN_FILE=../$(TOOLCHAIN_FILE) \
		-DCMAKE_BUILD_TYPE=$(BUILD_TYPE)
	@echo "Configuration complete."

# ------------------------------------------------------------------
# Clean
# ------------------------------------------------------------------
clean:
	@echo "Cleaning build directory..."
	@rm -rf $(BUILD_DIR)
	@echo "Clean complete."

clean-logs:
	@echo "Removing build logs..."
	@rm -rf $(LOG_DIR)
	@echo "Logs cleaned."

clean-bins:
	@echo "Cleaning temporyary files from image directory..."
	@rm -f Gen3Images/*-TEMP.bin Gen3Images/*-TEMP.elf Gen3Images/*-TEMP.hex
	@echo "Images cleaned."

clean-all: clean clean-logs clean-bins
	@echo "All clean complete."

rebuild: clean-all setup all

# ------------------------------------------------------------------
# Help
# ------------------------------------------------------------------
help:
	@echo "Workflow options:"
	@echo "  make setup                → Configure project"
	@echo "  cd _build && ninja        → Recommended: fast native builds"
	@echo ""
	@echo "Top-level shortcuts:"
	@echo "  make OppGen3              → Build application"
	@echo "  make Booty                → Build bootloader"
	@echo "  make all                  → Build both"
	@echo "  make lint                 → Run lint"
	@echo "  make clean                → Remove build directory"
	@echo "  make rebuild              → full clean and rebuild"
	@echo "  make clean-logs           → Remove build logs"
	@echo "  make clean-bins           → Remove temporary image files"
	@echo "  make clean-all            → clean build directory, logs, and temp image files"
	@echo ""
	@echo "Override build type: make OppGen3 BUILD_TYPE=Release"

.PHONY: all OppGen3 Booty lint setup clean clean-logs clean-all rebuild help
