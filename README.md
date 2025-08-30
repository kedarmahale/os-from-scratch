# MeowKernel - The Purr-fect Operating System Kernel 🐱

Educational OS kernel that will power **CatOS** - built from scratch with love for cats!

## Features Implemented ✅
- **Hardware Abstraction Layer (HAL)** 🔧
- **Cross-compiler Build System** ⚙️
- **QEMU Testing Environment** 🖥️
- **VGA Text Output** 📺

## Coming Soon 🚧
- Interrupt Handling (no more cat-astrophic crashes!)
- Memory Management (smart as a cat!)
- Device Drivers (purr-fect hardware control)
- Raspberry Pi Port (tiny but mighty, like a kitten!)

## Building MeowKernel

make clean && make iso && make run 
### Watch it purr!

## Directory structure

my-kernel/
├── boot/                    # Source files
│   └── boot.S
├── kernel/
│   └── kernel.c
├── advanced/
│   └── hal/
├── scripts/
│   └── linker.ld
├── build/                   # All build artifacts (safe to delete)
│   ├── obj/                 # Object files (mirrors source structure)
│   │   ├── boot/
│   │   │   └── boot.o
│   │   ├── kernel/
│   │   │   └── kernel.o
│   │   └── advanced/
│   │       └── hal/
│   │           ├── hal.o
│   │           └── x86/
│   │               └── hal_x86.o
│   ├── bin/                 # Final binaries
│   │   └── kernel.bin
│   ├── iso/                 # ISO building directory
│   │   └── boot/
│   │       ├── kernel.bin
│   │       └── grub/
│   │           └── grub.cfg
│   └── mykernel.iso         # Bootable ISO file
└── Makefile

