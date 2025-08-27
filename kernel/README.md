https://www.perplexity.ai/spaces/os-from-scrach-w0kFjvlnQYOsR2Wr2i4iMA

# Minimal Kernel implementation

## Overwiew

This is not a recommended skeleton for project structure, but rather as an example of a minimal kernel. 
In this simple case, you need three input files:

    boot.s - kernel entry point that sets up the processor environment
    kernel.c - your actual kernel routines
    linker.ld - for linking the above files
	
## Booting the Operating System

To start the operating system, an existing piece of software will be needed to load it. 
This is called the bootloader, and in this tutorial, you will be using GRUB. 

The kernel is passed a very minimal environment, in which the stack is not set up yet, and virtual memory 
is not yet enabled, hardware is not initialized, and so on. 
 
The first task you will address is how the bootloader initiates the kernel. OSDevers are lucky because 
there exists a Multiboot Standard, which describes an easy interface between the bootloader and the
operating system kernel. It works by putting a few magic values in some global variables (known as a multiboot header), 
which is searched for by the bootloader. When it sees these values, it recognizes the kernel as multiboot compatible 
and it knows how to load us, and it can even forward us important information such as memory maps.

Since there is no stack yet and you need to make sure the global variables are set correctly, you will do this in assembly. 

### Bootstrap Assembly

We are using the GNU assembler (we could use NASM, too).
The very most important piece to create is the multiboot header, as it must be very early in the kernel binary,
or the bootloader will fail to recognize us. 

Detailed notes here: https://github.com/kedarmahale/os-from-scratch/blob/main/kernel/boot.s

## Implementing the Kernel

### Freestanding and Hosted Environments

In C or C++ programming in user-space, you have used a so-called Hosted Environment. Hosted means that there is a C 
standard library and other useful runtime features. 

Alternatively, there is the Freestanding version, which is what you are using here. Freestanding means that there is 
No C standard library, only what you provide yourself. 

However, some header files are actually not part of the C standard library, but rather the compiler. These remain 
available even in freestanding C source code. In this case, you use <stdbool.h> to get the bool datatype, <stddef.h> 
to get size_t and NULL, and <stdint.h> to get the intx_t and uintx_t datatypes which are invaluable for operating systems 
development, where you need to make sure that the variable is of an exact size 
(if you used a short instead of uint16_t and the size of short changed, your VGA driver here would break!). 

Additionally, you can access the <float.h>, <iso646.h>, <limits.h>, and <stdarg.h> headers, as they are also freestanding. 
GCC actually ships a few more headers, but these are special-purpose. 

### Writing a kernel in C

The following shows how to create a simple kernel in C. This kernel uses the VGA text mode buffer (located at 0xB8000) 
as the output device. It sets up a simple driver that remembers the location of the next character in this buffer and provides 
a primitive for adding a new character. 

Notably, there is no support for line breaks ('\n') (and writing that character will show some VGA-specific character instead) 
and no support for scrolling when the screen is filled up. 
These are added here: https://github.com/kedarmahale/os-from-scratch/blob/main/kernel/kernel.c

IMPORTANT NOTE: The VGA text mode (as well as the BIOS) is deprecated on newer machines, and UEFI only supports pixel buffers.

## Linking the Kernel

You can now assemble boot.s and compile kernel.c. This produces two object files that each contain part of the kernel. 
To create the full and final kernel, you will have to link these object files into the final kernel program, usable by the 
bootloader. When developing user-space programs, your toolchain ships with default scripts for linking such programs. 
However, these are unsuitable for kernel development, and you need to provide your own customized linker script

## Setting up the toolchain

### 1. Install Cross-Compiler (i686-elf-gcc)
You MUST use a cross-compiler for kernel development. 

#### Install dependencies
```
sudo apt update
sudo apt install build-essential bison flex libgmp3-dev libmpc-dev libmpfr-dev texinfo
```

#### Download and build binutils
```
wget https://ftp.gnu.org/gnu/binutils/binutils-2.42.tar.xz
tar -xf binutils-2.42.tar.xz
mkdir binutils-2.42-build
cd binutils-2.42-build
./configure --target=i686-elf --prefix="$HOME/cross" --disable-nls --disable-werror
make
make install
cd ..
```
#### Download and build GCC
```
wget https://ftp.gnu.org/gnu/gcc/gcc-14.1.0/gcc-14.1.0.tar.xz
tar -xzf gcc-14.1.0.tar.xz
mkdir gcc-14.1.0-build
cd gcc-14.1.0-build
./configure --target=i686-elf --prefix="$HOME/cross" --disable-nls --enable-languages=c,c++ --without-headers
make all-gcc
make all-target-libgcc
make install-gcc
make install-target-libgcc
cd ..
```
#### Add to PATH
```
export PATH="$HOME/cross/bin:$PATH"
# Add this line to your ~/.bashrc or ~/.profile to make it permanent
echo 'export PATH="$HOME/cross/bin:$PATH"' >> ~/.bashrc
```

## Minimal x86 Kernel

### Features
  1. Multiboot-compliant (GRUB compatible)
  2. VGA text mode output with colors
  3. Serial port debugging support
  4. Basic terminal with scrolling
  5. Cross-platform build system
  6. QEMU testing integration
  7. GDB debugging support

### Quick Start

  1. Install QEMU: sudo apt install qemu-system-x86
  2. Build: make
  3. Run: 
        ```
      make run        # Run in QEMU
      make run-serial # Run with serial debugging
      make debug      # Debug with GDB
      make iso        # Create bootable ISO
      make clean      # Clean build files
        ```

### Files
  1. boot.s - Assembly bootstrap code
  2. kernel.c - Main kernel implementation
  3. linker.ld - Linker script
  4. Makefile - Build system
  5. grub.cfg - GRUB configuration

## What is happening here?




