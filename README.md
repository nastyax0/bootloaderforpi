# Raspberry Pi 3 Bootloader

A minimal bare-metal bootloader for the Raspberry Pi 3 that initializes hardware, loads a kernel image from SD/eMMC storage, and transfers execution to it.

---

## Overview

This project demonstrates a minimal bare-metal boot process on the Raspberry Pi 3:

- Configures MMIO-based peripherals (GPIO, UART, eMMC/SD)
- Initializes the ARM Cortex-A53 execution environment
- Loads a kernel image from storage into RAM
- Transitions to EL1 and transfers execution to the loaded kernel
- Provides UART-based debugging output

---

## Boot Flow

ROM Boot → GPU Firmware → `bootcode.bin` → `start.elf` → `kernel8.img` (bootloader) → Custom Kernel

### Bootloader Responsibilities

1. Initialize UART for debugging
2. Initialize SD/eMMC controller
3. Read kernel image from storage (LBA-based in current version)
4. Copy kernel image to `0x200000`
5. Set up stack and execution environment
6. Transition to EL1
7. Branch to the kernel entry point

---

## Build

### Build the Bootloader (`kernel8.img`)

```bash
make clean
make
````

### Build the Test Kernel (`kernel.bin`)

```bash
aarch64-none-elf-gcc -c boot.S
aarch64-none-elf-gcc -c kernel.c -ffreestanding
aarch64-none-elf-ld -T linker.ld boot.o kernel.o -o kernel.elf
aarch64-none-elf-objcopy -O binary kernel.elf kernel.bin
```

---

## Current Limitations

* Kernel location is currently hardcoded using LBA offsets
* No complete FAT32 file lookup implementation
* No dynamic memory allocator
* No interrupt handling
* No DMA support
* No interactive boot shell
* Limited error handling
* Does not currently run under QEMU due to implementation differences
* Test kernel is intentionally minimal

---

## Features

* Runs successfully on Raspberry Pi 3 hardware
* UART-based debugging support
* SD/eMMC block-level reads
* Kernel loading into RAM
* EL1 execution handoff
* Minimal and educational codebase

---

## Future Work

> priority: high/low

* Interrupt handling (high)
* DMA-based transfers (high)
* Full FAT32 file discovery (low)
* GPT partition support (medium)
* Improved memory management (low)
* Better QEMU compatibility (low)
* Interactive bootloader interface (low)
* Secure boot and firmware integrity experiments (high)
* Firmware Security Analysis Demonstration (high)

---

## Status

Current implementation demonstrates a complete boot chain:

SD/eMMC → Bootloader → RAM Loading → EL1 Transition → Kernel Execution

The project is intended as a learning-oriented exploration of bare-metal ARM64 development, boot processes, and low-level hardware interaction on Raspberry Pi 3.

But I have decided to shift the angle for security booting purposes more in next phase, for demonstration and educational purposes.
```
