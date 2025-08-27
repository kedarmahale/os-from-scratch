# Description
This is a Minimal kernel that will boot on QEMU.

## Setting up the toolchain

### 1. Install Cross-Compiler (i686-elf-gcc)
You MUST use a cross-compiler for kernel development. 

```
# Install dependencies
sudo apt update
sudo apt install build-essential bison flex libgmp3-dev libmpc-dev libmpfr-dev texinfo

# Download and build binutils
wget https://ftp.gnu.org/gnu/binutils/binutils-2.42.tar.xz
tar -xf binutils-2.42.tar.xz
mkdir binutils-2.42-build
cd binutils-2.42-build
./configure --target=i686-elf --prefix="$HOME/cross" --disable-nls --disable-werror
make
make install
cd ..

# Download and build GCC
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

# Add to PATH
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

    boot.s - Assembly bootstrap code
    kernel.c - Main kernel implementation
    linker.ld - Linker script
    Makefile - Build system
    grub.cfg - GRUB configuration



