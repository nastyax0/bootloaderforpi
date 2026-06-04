# Raspberry Pi 3 Bootloader

A bare-metal ARM64 bootloader for Raspberry Pi 3 developed as a low-level exploration of system startup, UART debugging, SD/eMMC communication, filesystem investigation, and kernel loading.

The project currently demonstrates a complete boot chain from storage to kernel execution and serves as a foundation for future experimentation with secure boot mechanisms and firmware security concepts.

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
* Kernel loading into RAM
* EL1 execution handoff
* Minimal and educational codebase
* Loads a kernel image from SD/eMMC using raw sector reads
* Demonstrates block-level storage access without relying on an operating system

---

## Future Work

### High Priority
- Interrupt handling
- DMA-based transfers
- Secure boot experiments
- Firmware security demonstrations

### Medium Priority
- GPT partition support

### Low Priority
- Full FAT32 file discovery
- Improved memory management
- Better QEMU compatibility
- Interactive bootloader interface

---

## Status

Current implementation demonstrates a complete boot chain:

SD/eMMC → Bootloader → RAM Loading → EL1 Transition → Kernel Execution

The project is intended as a learning-oriented exploration of bare-metal ARM64 development, boot processes, and low-level hardware interaction on Raspberry Pi 3.

But I have decided to shift the angle for security booting purposes more in next phase, for demonstration and educational purposes.

## Logs/ Issues

``` bash
Raspberry Pi Bootcode
Read File: config.txt, 159
Read File: start.elf, 3027872 (bytes)
MESS:00:00:01.222807:0: boot-part: 0 fs-type: 0
MESS:00:00:01.225633:0: boot-part: 0 fs-type: 3
MESS:00:00:01.233931:0: brfs: File read: /mfs/sd/config.txt
MESS:00:00:01.238039:0: board: boardrev 9020e0 otp 9020e0
MESS:00:00:01.243049:0: brfs: File read: 159 bytes
MESS:00:00:01.268565:0: brfs: File read: /mfs/sd/config.txt
MESS:00:00:01.499582:0: gpioman: gpioman_get_pin_num: pin DISPLAY_DSI_PORT not d                                                                                                             efined
MESS:00:00:01.506620:0: gpioman: gpioman_get_pin_num: pin DISPLAY_DSI_PORT not d                                                                                                             efined
MESS:00:00:01.531452:0: *** Restart logging
MESS:00:00:01.533947:0: brfs: File read: 159 bytes
MESS:00:00:01.539075:0: hdmi: HDMI:hdmi_get_state is deprecated, use hdmi_get_di                                                                                                             splay_state instead
MESS:00:00:01.547225:0: HDMI0: hdmi_pixel_encoding: 162000000
MESS:00:00:01.552957:0: vec: vec_middleware_power_on: vec_base: 0x7e806000 rev-i                                                                                                             d 0x00002708 @ vec: 0x7e806100 @ 0x00000420 enc: 0x7e806060 @ 0x00000220 cgmsae:                                                                                                              0x7e80605c @ 0x00000000
MESS:00:00:01.580116:0: dtb_file 'bcm2710-rpi-3-a-plus.dtb'
MESS:00:00:01.584579:0: dtb_file 'bcm2710-rpi-3-b-plus.dtb'
MESS:00:00:01.589870:0: dtb_file 'bcm2837-rpi-3-a-plus.dtb'
MESS:00:00:01.595167:0: dterror: Failed to load Device Tree file '?'
MESS:00:00:01.601257:0: Failed to open command line file 'cmdline.txt'
MESS:00:00:01.610269:0: brfs: File read: /mfs/sd/kernel8.img
MESS:00:00:01.614234:0: Loaded 'kernel8.img' to 0x0 size 0x25ea
MESS:00:00:01.619922:0: Kernel relocated to 0x80000
MESS:00:00:01.624513:0: gpioman: gpioman_get_pin_num: pin EMMC_ENABLE not define                                                                                                             d
MESS:00:00:01.635067:0: uart: Set PL011 baud rate to 103448.300000 Hz
MESS:00:00:01.641358:0: uart: Baud rate change done...
MESS:00:00:01.644787:0: uart: Baud rate▒

EMMC: GPIO set up
EMMC: reset OK
SD card clock Information:
dividor(d): 00000000000000000000000001111101

EMMC_CONTROL1: 000E7D07

EMMC_STATUS: 01FF0000

EMMC: SD clock set to desired frequency
EMMC: Support OK
CMDTM0: 00000000000000000000000000000000
                                        CMDTM8: 00001000000110100000000000000000

Response: 00000000000000000000000110101010

INTERRUPT: 00000001

sd_readblock lba 00000000
                          num 00000001

CMD55 successful
CMD55 succesful
ACMD41 resp: 40FF8000

CMD55 successful
CMD55 succesful
ACMD41 resp: 40FF8000

CMD55 successful
CMD55 succesful
ACMD41 resp: C0FF8000

!data line isnt busy!
!command line isnt busy!
CMDTM2: 00000010000110010000000000000000

!data line isnt busy!
!command line isnt busy!
CMDTM3: 00000011000110100000000000000000

!data line isnt busy!
!command line isnt busy!
RCA: AAAA0000

CMDTM7: 00000111000110110000000000000000

STATE after CMD7: 00000003

!data line isnt busy!
!command line isnt busy!
CMDTM: 00010010001110100000000000110110

STATE: 00000004

Response: 00000000000000000000100100000000

INTERRUPT: 00000020

=================================DEBUG=================================
EMMC_INTERRUPT: 00000020

EMMC_STATUS: 01FF0A06

INT_READ_RDY: 00000020

EMMC_CONTROL0: 00000000

Data is ready to be read
Reading data...
00200000
        : 40 01 00 58  01 09 80 52  01 00 00 B9  20 01 00 58  @..X...R.... ..X
00200010
        : 1F 00 00 91  20 01 00 58  00 00 1F D6  00 00 00 14  .... ..X........
00200020
        : C0 03 5F D6  00 00 00 00  00 10 20 3F  00 00 00 00  .._....... ?....
00200030
        : 00 00 40 00  00 00 00 00  30 03 20 00  00 00 00 00  ..@.....0. .....
00200040
        : FD 7B BE A9  FD 03 00 91  00 48 88 52  E0 01 A0 72  .{.......H.R...r
00200050
        : E0 1F 00 B9  1F 20 03 D5  00 03 82 D2  00 E4 A7 F2  ..... ..........
00200060
        : 00 00 40 B9  00 00 1D 12  1F 00 00 71  E0 00 00 54  ..@........q...T
00200070
        : E0 1F 40 B9  00 04 00 51  E0 1F 00 B9  E0 1F 40 B9  ..@....Q......@.
00200080
        : 21 FF FF 54  00 00 82 D2  00 E4 A7 F2  E1 7F 40 39  !..T..........@9
00200090
        : 01 00 00 B9  E0 7F 40 39  1F 20 00 71  61 03 00 54  ......@9. .qa..T
002000A0
        : 03 00 00 14  E0 7F 40 39  7E FF FF 97  00 03 82 D2  ......@9~.......
002000B0
        : 00 E4 A7 F2  00 00 40 B9  00 00 1B 12  1F 00 00 71  ......@........q
002000C0
        : 21 FF FF 54  00 00 82 D2  00 E4 A7 F2  01 04 80 52  !..T...........R
002000D0
        : 01 00 00 B9  03 00 00 14  E0 7F 40 39  71 FF FF 97  ..........@9q...
002000E0
        : 00 03 82 D2  00 E4 A7 F2  00 00 40 B9  00 00 1B 12  ..........@.....
002000F0
        : 1F 00 00 71  21 FF FF 54  00 00 82 D2  00 E4 A7 F2  ...q!..T........
00200100
        : 02 00 00 14  00 06 80 52  8B FF FF 97  E0 2F 40 B9  .......R...../@.
00200110
        : 00 04 00 51  E0 2F 00 B9  E0 2F 40 B9  1F 00 00 71  ...Q./.../@....q
00200120
        : EA FD FF 54  40 01 80 52  83 FF FF 97  1F 20 03 D5  ...T@..R..... ..
00200130
        : FD 7B C3 A8  C0 03 5F D6  FD 7B BE A9  FD 03 00 91  .{...._..{......
00200140
        : E0 0F 00 F9  0C 00 00 14  E0 0F 40 F9  00 00 40 39  ..........@...@9
00200150
        : 1F 28 00 71  61 00 00 54  A0 01 80 52  76 FF FF 97  .(.qa..T...Rv...
00200160
        : E0 0F 40 F9  01 04 00 91  E1 0F 00 F9  00 00 40 39  ..@...........@9
00200170
        : 71 FF FF 97  E0 0F 40 F9  00 00 40 39  1F 00 00 71  q.....@...@9...q
00200180
        : 41 FE FF 54  1F 20 03 D5  1F 20 03 D5  FD 7B C2 A8  A..T. ... ...{..
00200190
        : C0 03 5F D6  FF 43 00 D1  E0 0F 00 B9  1F 20 03 D5  .._..C....... ..
002001A0
        : 00 03 82 D2  00 E4 A7 F2  00 00 40 B9  00 00 1B 12  ..........@.....
002001B0
        : 1F 00 00 71  61 FF FF 54  00 00 82 D2  00 E4 A7 F2  ...qa..T........
002001C0
        : E1 0F 40 B9  01 00 00 B9  1F 20 03 D5  FF 43 00 91  ..@...... ...C..
002001D0
        : C0 03 5F D6  FD 7B BE A9  FD 03 00 91  00 00 00 90  .._..{..........
002001E0
        : 00 60 14 91  D5 FF FF 97  83 FF FF 97  E0 7F 00 39  .`.............9
002001F0
        : E0 7F 40 39  E3 FE FF 97  FC FF FF 17  00 00 00 00  ..@9............
Jumping to kernel...
EXCEPTION LEVEL: 0000000C

Instruction Address: 000019F4

Cache invalidated.
EL1J
    H
```
The above is for people to check out quickly common mistakes and related logs.

The author is in state of fixing the kernel file i.e kernel.bin and adding better debugging logs.

