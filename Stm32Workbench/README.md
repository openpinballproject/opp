# Open Pinball Project
STM32 Build Process for STM32F103C8 target

# Prequisite Tooling
## Toolchain
ARM GNU Toolchain version 15.2.Rel1

https://developer.arm.com/downloads/-/arm-gnu-toolchain-downloads

## Build Tools
CMake version 3.15 or greater

# Linux Build Procedure

- Navigate in a linux terminal to this directory
- Type _make help_ for a list of targets
    - _make OppGen3_
        - Build main application only, but does require bootloader ELF
    - _make Booty_
        - Build bootloader only
    - _make all_
        - build OPPGen3 and Booty targets
    - _make clean_
        - Removes OPPGen3 and bootloader artifacts
- Log files of build process are created in _\_logs_ directory
- Build artifacts can be found in _Gen3Images_ directory

# Build Structure
The Makefile in this directory is the entry-point for the build process. The build process is created through CMake files aside from the simple entry Makefile. CMake provides a way to create expansive and highly customizable build structures. It tends to be more powerful and easier to use than traditional Makefiles.

The entry-point Makefile creates the CMake build directory (named _\_build_) and generated the CMake project. From that point, CMake-generated files are used to build artifacts.


The overall build structure is:

```bash
  Stm32Workbench/
  ├── Makefile                  # Build flow entry point
  ├── CMakeLists.txt            # Top-level CMake file
  ├── arm-none-eabi-gcc.cmake   # Toolchain file
  ├── Booty/
  │   ├── CMakeLists.txt        # Adds bootloader target, target-specific options
  │   ├── Booty.cmake           # Bootloader-specific files and settings
  │   ├── src/
  │   ├── inc/
  │   └── stm32F103x6.ld        # Bootloader linker script
  ├── OppGen3/
  │   ├── CMakeLists.txt        # Adds app target, target-specific options
  │   ├── OppGen3.cmake         # App-specific files and settings
  │   ├── src/
  │   ├── Inc/
  │   ├── Drivers/
  │   ├── Middlewares/
  │   └── STM32F103C8Tx_FLASH.ld.template   # Template for build-generated linker script
  └── _logs/                    # build-generated logs
  └── _build/                   # build-generated artifacts
```
## Makefile
This file defines the entry-point targets. The main goal of this file is to create the CMake build directory and configure the CMake project. It also provides a simple and familiar user experience.

### Targets
Below is a list of targets available at this level. Any dependent targets are in parentheses after the description.

| Name | Description | Dependencies |
|------|-------------|--------------|
|_help_|displays help menu and a list of targets|none|
|_setup_|creates the CMake build structure|none|
|_Booty_|builds the bootloader|_setup_|
|_OppGen3_|builds the main application|_setup_, _Booty_|
|_clean_|removes the CMake build directory|none|
|_clean-logs_|removes the log directory|none|
|_clean-bins_|removes temporary build artifacts copied to the _Gen3Images_ directory|none|
|_clean-all_|removes build and log directories, and artifacts copied to the _Gen3Images_ directory|none|
|_rebuild_|full clean and rebuild of _Booty_ and _OppGen3_ targets|_clean-all_, _all_|
|_blinky_|test build (not supported)|_setup_|
|_lint_|run linter (not supported)|_setup_|

If _Booty_ or _OppGen3_ targets are successfully built, the artifacts are copied into the _Gen3Images_ directory as \<target-name\>-TEMP.elf/.hex/.bin. There, they can be renamed manually with current version information. This step could be scripted, but that has not been implemented as yet.

## CMakeLists.txt
This file is the entry-point for the CMake build structure. It's purpose is to define:
- CMake version to enforce
- Project name/type
- Project-wide compiler and linker options
- Project targets by including additional CMakeLists files

## Target-Specific CMake Files
There are two target-specific CMake files in the root of each target directory:
- CMakeLists.txt
- \<target-name\>.cmake

### \<target-name\>.cmake
This file defines sources for the target, including:
- target-specific defines
- target source file(s) (*.c, *.s)
- target linker file(s)
This file is designed to be included in the target _CMakeLists.txt_ file.

### CMakeLists.txt
This file declares all the build options for this target. These target-specific options include:
- name
- search directories
- compiler options
- linker options
- custom pre/post-build commands (i.e. copying binaries, creating HEX/BIN files, etc.)

# Notes
## Booty Bootloader Sizing and OppGen3 Linker Descriptor
The Booty bootloader is designed to fit within 4kB. Part of the main application build (OppGen3), the Booty biary is copied into the boot section of the linker descriptor (LD) file. The OppGen3 LD is dynamically created from _OppGen3/STM32F103C8Tx_FLASH.ld.template_, which simply inserts the name and path of the Booty binary.

The section reserves 4kB for this binary. During the OppGen3 build process, be cognizant of the size of the BOOT_FLASH memory region. If Booty is too large, then it will not fit in this region and will fail to build correctly.

The OppGen3 linker descriptor file can be made to use an existing Booty binary, or a freshly built one. By default, the OppGen3 target uses the latest released bootloader, _Gen3Images/BootyStm32.0.0.bin_. Which bootloader binary used when building OppGen3 is defined by the variable
_BOOTLOADER\_BIN\_PATH_ in _OppGen3/OppGen3.cmake_.


# Example Build
```
user@buildbox:~/projects/opp/Stm32Workbench$ make help
Workflow options:
  make setup                → Configure project
  cd _build && ninja        → Recommended: fast native builds

Top-level shortcuts:
  make OppGen3              → Build application
  make Booty                → Build bootloader
  make blinky               → Build blinky test
  make all                  → Build both
  make lint                 → Run lint
  make clean                → Remove build directory
  make rebuild              → full clean and rebuild
  make clean-logs           → Remove build logs
  make clean-bins           → Remove temporary image files
  make clean-all            → clean build directory, logs, and temp image files

Override build type: make OppGen3 BUILD_TYPE=Release

user@buildbox:~/projects/opp/Stm32Workbench$ make Booty
Configuring CMake in _build...
-- The C compiler identification is GNU 15.2.1
-- The ASM compiler identification is GNU
-- Found assembler: /opt/arm-gnu-toolchain-15.2.rel1-x86_64-arm-none-eabi/bin/arm-none-eabi-gcc
-- Detecting C compiler ABI info
-- Detecting C compiler ABI info - done
-- Check for working C compiler: /opt/arm-gnu-toolchain-15.2.rel1-x86_64-arm-none-eabi/bin/arm-none-eabi-gcc - skipped
-- Detecting C compile features
-- Detecting C compile features - done
-- Configuring done
-- Generating done
-- Build files have been written to: /home/user/projects/opp/Stm32Workbench/_build
Configuration complete.
Project ready in _build/
Tip: cd _build && ninja   # for fastest workflow
Building Booty... (log: _logs/Booty-2026-01-14_22-39-52.log)
[1/8] Building ASM object Booty/CMakeFiles/Booty.elf.dir/src/startup.s.obj
[2/8] Building ASM object Booty/CMakeFiles/Booty.elf.dir/src/usbd_devfs_asm.S.obj
[3/8] Building C object Booty/CMakeFiles/Booty.elf.dir/src/stdlflash.c.obj
[4/8] Building C object Booty/CMakeFiles/Booty.elf.dir/src/cdc_loop.c.obj
[5/8] Building C object Booty/CMakeFiles/Booty.elf.dir/src/usbd_core.c.obj
[6/8] Building C object Booty/CMakeFiles/Booty.elf.dir/src/booty.c.obj
[7/8] Building C object Booty/CMakeFiles/Booty.elf.dir/src/usbd_devfs.c.obj
[8/8] Linking C executable Booty/Booty.elf
Memory region         Used Size  Region Size  %age Used
             ROM:        3760 B        64 KB      5.74%
             RAM:         348 B      20476 B      1.70%
   text	   data	    bss	    dec	    hex	filename
   3760	     32	    316	   4108	   100c	Booty.elf
Booty build complete → _build/Booty/Booty.elf

user@buildbox:~/projects/opp/Stm32Workbench$ make OppGen3
Project ready in _build/
Tip: cd _build && ninja   # for fastest workflow
Building OppGen3... (log: _logs/OppGen3-2026-01-14_22-40-07.log)
[1/35] Building C object OppGen3/CMakeFiles/OppGen3.elf.dir/Src/Common/debug.c.obj
[2/35] Building C object OppGen3/CMakeFiles/OppGen3.elf.dir/Src/Common/spiwing.c.obj
[3/35] Building C object OppGen3/CMakeFiles/OppGen3.elf.dir/Src/Common/lampMtrx.c.obj
[4/35] Building C object OppGen3/CMakeFiles/OppGen3.elf.dir/Src/Common/servo.c.obj
[5/35] Building C object OppGen3/CMakeFiles/OppGen3.elf.dir/Src/Common/neopxl.c.obj
[6/35] Building C object OppGen3/CMakeFiles/OppGen3.elf.dir/Src/Common/fade.c.obj
[7/35] Building C object OppGen3/CMakeFiles/OppGen3.elf.dir/Src/Common/incand.c.obj
[8/35] Building C object OppGen3/CMakeFiles/OppGen3.elf.dir/Src/Common/stdlflash.c.obj
[9/35] Building C object OppGen3/CMakeFiles/OppGen3.elf.dir/Src/Common/stdldigio.c.obj
[10/35] Building C object OppGen3/CMakeFiles/OppGen3.elf.dir/Src/Common/timer.c.obj
[11/35] Building C object OppGen3/CMakeFiles/OppGen3.elf.dir/Src/usbd_cdc_if.c.obj
[12/35] Building C object OppGen3/CMakeFiles/OppGen3.elf.dir/Src/Common/stdlser.c.obj
[13/35] Building C object OppGen3/CMakeFiles/OppGen3.elf.dir/Src/usbd_desc.c.obj
[14/35] Building C object OppGen3/CMakeFiles/OppGen3.elf.dir/Src/usb_device.c.obj
[15/35] Building C object OppGen3/CMakeFiles/OppGen3.elf.dir/Src/main.c.obj
[16/35] Building C object OppGen3/CMakeFiles/OppGen3.elf.dir/Src/usbd_conf.c.obj
[17/35] Building C object OppGen3/CMakeFiles/OppGen3.elf.dir/Src/Common/rs232proc.c.obj
[18/35] Building C object OppGen3/CMakeFiles/OppGen3.elf.dir/Src/Common/digital.c.obj
[19/35] Building C object OppGen3/CMakeFiles/OppGen3.elf.dir/Drivers/STM32F1xx_HAL_Driver/Src/stm32f1xx_hal.c.obj
[20/35] Building C object OppGen3/CMakeFiles/OppGen3.elf.dir/Middlewares/ST/STM32_USB_Device_Library/Core/Src/usbd_core.c.obj
[21/35] Building C object OppGen3/CMakeFiles/OppGen3.elf.dir/Middlewares/ST/STM32_USB_Device_Library/Core/Src/usbd_ioreq.c.obj
[22/35] Building C object OppGen3/CMakeFiles/OppGen3.elf.dir/Middlewares/ST/STM32_USB_Device_Library/Class/CDC/Src/usbd_cdc.c.obj
[23/35] Building C object OppGen3/CMakeFiles/OppGen3.elf.dir/Middlewares/ST/STM32_USB_Device_Library/Core/Src/usbd_ctlreq.c.obj
[24/35] Building ASM object OppGen3/CMakeFiles/OppGen3.elf.dir/startup/startup_stm32f103xb.S.obj
[25/35] Building C object OppGen3/CMakeFiles/OppGen3.elf.dir/Drivers/STM32F1xx_HAL_Driver/Src/stm32f1xx_hal_pcd_ex.c.obj
[26/35] Building C object OppGen3/CMakeFiles/OppGen3.elf.dir/Drivers/STM32F1xx_HAL_Driver/Src/stm32f1xx_hal_cortex.c.obj
[27/35] Building C object OppGen3/CMakeFiles/OppGen3.elf.dir/Drivers/STM32F1xx_HAL_Driver/Src/stm32f1xx_hal_gpio.c.obj
[28/35] Building C object OppGen3/CMakeFiles/OppGen3.elf.dir/Drivers/STM32F1xx_HAL_Driver/Src/stm32f1xx_hal_rcc_ex.c.obj
[29/35] Building C object OppGen3/CMakeFiles/OppGen3.elf.dir/Src/system_stm32f1xx.c.obj
[30/35] Building C object OppGen3/CMakeFiles/OppGen3.elf.dir/Src/stm32f1xx_it.c.obj
[31/35] Building C object OppGen3/CMakeFiles/OppGen3.elf.dir/Drivers/STM32F1xx_HAL_Driver/Src/stm32f1xx_hal_rcc.c.obj
[32/35] Building C object OppGen3/CMakeFiles/OppGen3.elf.dir/Src/stm32f1xx_hal_msp.c.obj
[33/35] Building C object OppGen3/CMakeFiles/OppGen3.elf.dir/Drivers/STM32F1xx_HAL_Driver/Src/stm32f1xx_hal_pcd.c.obj
[34/35] Building C object OppGen3/CMakeFiles/OppGen3.elf.dir/Drivers/STM32F1xx_HAL_Driver/Src/stm32f1xx_ll_usb.c.obj
[35/35] Linking C executable OppGen3/OppGen3.elf
/opt/arm-gnu-toolchain-15.2.rel1-x86_64-arm-none-eabi/bin/../lib/gcc/arm-none-eabi/15.2.1/../../../../arm-none-eabi/bin/ld: /opt/arm-gnu-toolchain-15.2.rel1-x86_64-arm-none-eabi/bin/../lib/gcc/arm-none-eabi/15.2.1/../../../../arm-none-eabi/lib/thumb/v7-m/nofp/libc_nano.a(libc_a-writer.o): note: the message above does not take linker garbage collection into account
/opt/arm-gnu-toolchain-15.2.rel1-x86_64-arm-none-eabi/bin/../lib/gcc/arm-none-eabi/15.2.1/../../../../arm-none-eabi/bin/ld: warning: OppGen3/OppGen3.elf has a LOAD segment with RWX permissions
Memory region         Used Size  Region Size  %age Used
             RAM:        7552 B        20 KB     36.88%
      BOOT_FLASH:        3792 B         4 KB     92.58%
           FLASH:       26312 B        60 KB     42.83%

Processing OppGen3.hex...
CRC addr located as: 0x080076c4
Table CRC calculated as: 0x39449ab8
Opening OppGen3.hex for writing
Original line ":0476C40000000000C2"
New crc32 record ":0476C400B89A4439F3"
Finished processing OppGen3.hex

   text	   data	    bss	    dec	    hex	filename
  25736	   4368	   6988	  37092	   90e4	OppGen3.elf
OppGen3 build complete → _build/OppGen3/OppGen3.elf

```
> [!NOTE]
> Compiler warnings were removed from the example output.