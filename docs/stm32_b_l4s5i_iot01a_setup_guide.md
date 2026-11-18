# STM32 B-L4S5I-IOT01A Setup Guide

This guide covers building, flashing, and debugging this repository's C++17
FreeRTOS firmware for the **STMicroelectronics B-L4S5I-IOT01A**
(`STM32L4S5VIT6`).

> **STM32CubeIDE and STM32CubeMX are not required.** The project uses CMake,
> Ninja, and the ARM GNU toolchain in a VS Code Dev Container. CMake downloads
> the STM32CubeL4 HAL/CMSIS source as a library dependency; the IDE and code
> generator are not used.

## What the firmware does

The application in `src/main.cpp`:

1. Initializes the STM32 HAL and configures the MSI/PLL clock tree for an
   80 MHz system clock (4 MHz MSI × 40 ÷ 2).
2. Configures the green LD2 LED on PB14 and USART1 on PB6 (TX) / PB7 (RX) for
   the ST-LINK virtual COM port at 115200 baud, 8-N-1.
3. Starts FreeRTOS with a task that toggles the LED and prints a heartbeat
   approximately once per second.

FreeRTOS owns SysTick at 1 kHz. Its tick hook calls `HAL_IncTick()` to keep the
HAL millisecond tick advancing. FreeRTOS allocation-failure and stack-overflow
hooks halt the system with interrupts disabled.

The board's ST-LINK USB connection provides both the debugger and virtual COM
port; no extra wiring is required.

## Project layout

```text
.
├── .devcontainer/
│   ├── devcontainer.json          # VS Code Dev Container configuration
│   └── Dockerfile                 # Debian toolchain and build tools
├── .vscode/
│   ├── launch.json                # Cortex-Debug configuration
│   └── tasks.json                 # Debug/release build and clean tasks
├── cmake/
│   └── arm-none-eabi.cmake        # ARM Cortex-M4F cross-compilation settings
├── config/
│   ├── FreeRTOSConfig.h           # Kernel settings, hooks, and IRQ priorities
│   └── stm32l4xx_hal_conf.h       # Enabled STM32L4 HAL modules
├── docs/
│   └── stm32_b_l4s5i_iot01a_setup_guide.md
├── src/
│   └── main.cpp                   # Clock, board peripherals, and application
├── CMakeLists.txt                 # Firmware target and fetched dependencies
├── STM32L4S5VITx_FLASH.ld         # 2 MB Flash / 640 KB SRAM linker layout
└── README.md
```

## Where the code and setup live

- `src/main.cpp` contains the clock and peripheral initialization, FreeRTOS
  hooks, and the blinking/logging task.
- `config/FreeRTOSConfig.h` controls the kernel tick, dynamic heap, hooks,
  interrupt priorities, and debugger task metadata.
- `config/stm32l4xx_hal_conf.h` selects HAL modules and sets HAL clock constants.
- `CMakeLists.txt` fetches the pinned HAL and FreeRTOS sources, chooses the
  Cortex-M4F FreeRTOS port and Heap 4 allocator, and defines firmware outputs.
- `cmake/arm-none-eabi.cmake` supplies the ARM compiler and Cortex-M4F flags;
  `STM32L4S5VITx_FLASH.ld` defines the MCU memory layout.
- `.devcontainer/` supplies the build/debug tools. The debugger connects from
  the container to host-side OpenOCD, which communicates with the board.

## Prerequisites

- Windows with Docker Desktop and Visual Studio Code
- VS Code **Dev Containers** extension
- A data-capable USB cable connected to the board's **CN7 (USB ST-LINK)**
- For flashing/debugging: Windows xPack OpenOCD and the ST-LINK USB driver
- For serial output: a terminal or VS Code serial monitor and the board's
  ST-LINK virtual COM port driver

The container is based on Debian Trixie and installs CMake, Ninja, Git,
`gcc-arm-none-eabi`, GDB, and the ARM binutils/newlib libraries. The CMake
project downloads STM32CubeL4 **v1.18.1** and FreeRTOS Kernel **V11.3.3**
through FetchContent into `.cache/deps`.

## Open the development container

1. Open this repository folder in VS Code.
2. Run **Dev Containers: Reopen in Container** and wait for the image build.
3. In the container, run **Tasks: Run Build Task** and select
   **Build STM32 Firmware (Debug)**.

The build task configures `build/debug` with Ninja and the ARM toolchain file,
then builds the firmware. The first configure downloads the dependencies and
may take longer than subsequent builds.

To build from the container terminal instead, run from the repository root:

```sh
cmake -B build/debug \
  -G Ninja \
  -DCMAKE_BUILD_TYPE=Debug \
  -DCMAKE_TOOLCHAIN_FILE=cmake/arm-none-eabi.cmake \
  -DFETCHCONTENT_BASE_DIR="${PWD}/.cache/deps"
cmake --build build/debug
```

For an optimized release build, use `build/release` and
`-DCMAKE_BUILD_TYPE=Release` in place of `build/debug` and `Debug`.
Debug and release builds have separate output directories.

The linker script defines 2048 KB Flash at `0x08000000` and 640 KB SRAM at
`0x20000000`. The firmware target is named `stm32l4s5-base.elf`.

## Build outputs

A successful build creates these files in the selected build directory:

- `stm32l4s5-base.elf` — ELF firmware image
- `stm32l4s5-base.bin` — raw binary image
- `stm32l4s5-base.hex` — Intel HEX image
- `stm32l4s5-base.map` — linker map
- `compile_commands.json` — compile commands used by the C/C++ tools

The build also prints the ELF's Flash and RAM usage.

## Flash the firmware

Connect the board to the Windows host over CN7. In a Windows PowerShell
terminal at the repository root, run OpenOCD with the ST-LINK interface and
STM32L4 target configuration. Adjust the executable path to match your
xPack OpenOCD installation:

```powershell
& "C:\path\to\openocd.exe" `
  -f interface/stlink.cfg `
  -f target/stm32l4x.cfg `
  -c "program build/debug/stm32l4s5-base.elf verify reset exit"
```

Use `build/release/stm32l4s5-base.elf` to flash the release build. Close any
existing OpenOCD process using the board before starting another one.

## Debug from VS Code

The included **Debug STM32L4S5 (Dev Container)** launch configuration uses an
external OpenOCD GDB server on the Windows host at
`host.docker.internal:3333`. Start OpenOCD on Windows and leave it running:

```powershell
& "C:\path\to\openocd.exe" `
  -f interface/stlink.cfg `
  -f target/stm32l4x.cfg `
  -c "bindto 0.0.0.0"
```

Then, in VS Code connected to the dev container, press **F5** and select
**Debug STM32L4S5 (Dev Container)**. The pre-launch task builds the debug
firmware; Cortex-Debug connects using `gdb-multiarch`, loads the ELF, and stops
at `main`.

OpenOCD's GDB server listens on the host network when using `bindto 0.0.0.0`.
Restrict access to trusted networks with your firewall, and stop the server
when debugging is finished.

## View serial output

Open the ST-LINK virtual COM port in a serial terminal with:

- Baud rate: 115200
- Data bits: 8
- Parity: none
- Stop bits: 1
- Flow control: none

After reset, output resembles:

```text

Hello Blinky from STM32L4S5!
Blink #1 (uptime: 1000 ms)
Blink #2 (uptime: 2000 ms)
```

## Clean build files

Use the VS Code tasks **Clean Debug Build** or **Clean Release Build**, or run
the corresponding command in the container:

```sh
cmake --build build/debug --target clean
cmake --build build/release --target clean
```

To remove generated build directories while keeping downloaded dependencies,
use **Clean All Builds (Keep FetchContent Cache)**. To remove the dependency
cache as well, use **Purge All Builds and FetchContent Cache**.
