# MeowKernel - The Purr-fect Operating System Kernel 🐱

 Kernel that will power **CatOS** - built from scratch with love for cats!

## Features Implemented
- **Hardware Abstraction Layer (HAL)**
- **Cross-compiler Build System** 
- **QEMU Testing Environment** 
- **VGA Text Output** 

## Coming Soon 
- Device Drivers (purr-fect hardware control)
- Raspberry Pi Port (tiny but mighty, like a kitten!)

## Building MeowKernel

make clean && make iso && make run 
### Watch it purr!

## Directory structure

meowkernel/
├── Makefile                    # Main orchestrator   \
├── build-system/               # Split build system   \
│   ├── common.mk               # Common build rules \
│   ├── arch-x86.mk             # x86 build configuration \
│   └── arch-arm64.mk           # ARM64 build configuration \
├── boot/ \
│   ├── x86/boot.S              # x86 boot code \
│   └── arm64/boot.S            # ARM64 boot code \
├── advanced/hal/ \
│   ├── hal.h & hal.c           # Architecture-independent HAL \
│   ├── x86/                    # Complete x86 implementation \
│   │   ├── hal_x86.h & hal_x86.c \
│   │   ├── gdt.c & gdt_flush.S \
│   │   ├── idt.c & interrupts.S \
│   │   ├── pic.c & pit.c \
│   └── arm64/                  # ARM64 infrastructure (stubs) \
│       ├── hal_arm64.h & hal_arm64.c \
└── scripts/ \
    ├── x86/linker.ld           # x86 linker script \
    └── arm64/linker.ld         # ARM64 linker script \

