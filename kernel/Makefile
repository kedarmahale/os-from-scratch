# Makefile for minimal kernel

# Toolchain
CC = i686-elf-gcc
AS = i686-elf-as
LD = i686-elf-gcc

# Flags
CFLAGS = -std=gnu99 -ffreestanding -O2 -Wall -Wextra
LDFLAGS = -ffreestanding -O2 -nostdlib
LIBS = -lgcc

# Files
KERNEL_SOURCES = kernel.c
KERNEL_OBJECTS = boot.o kernel.o
KERNEL_BINARY = myos.bin
ISO_IMAGE = myos.iso

# Default target
all: $(KERNEL_BINARY)

# Compile C files
%.o: %.c
	$(CC) -c $< -o $@ $(CFLAGS)

# Assemble assembly files
%.o: %.s
	$(AS) $< -o $@

# Link kernel
$(KERNEL_BINARY): $(KERNEL_OBJECTS)
	$(LD) -T linker.ld -o $@ $(LDFLAGS) $(KERNEL_OBJECTS) $(LIBS)

# Verify multiboot header
verify: $(KERNEL_BINARY)
	grub-file --is-x86-multiboot $(KERNEL_BINARY) && echo "Multiboot verified" || echo "Not multiboot compliant"

# Create bootable ISO
iso: $(KERNEL_BINARY)
	mkdir -p isodir/boot/grub
	cp $(KERNEL_BINARY) isodir/boot/$(KERNEL_BINARY)
	cp grub.cfg isodir/boot/grub/grub.cfg
	grub-mkrescue -o $(ISO_IMAGE) isodir

# Run in QEMU
run: $(KERNEL_BINARY)
	qemu-system-i386 -kernel $(KERNEL_BINARY)

# Run in QEMU with serial output to terminal
run-serial: $(KERNEL_BINARY)
	qemu-system-i386 -kernel $(KERNEL_BINARY) -serial stdio

# Run in QEMU with serial logging
run-log: $(KERNEL_BINARY)
	qemu-system-i386 -kernel $(KERNEL_BINARY) -serial file:serial.log

# Run bootable ISO in QEMU
run-iso: iso
	qemu-system-i386 -cdrom $(ISO_IMAGE)

# Debug with GDB
debug: $(KERNEL_BINARY)
	qemu-system-i386 -kernel $(KERNEL_BINARY) -s -S &
	@echo "QEMU started with GDB server on port 1234"
	@echo "In another terminal, run:"
	@echo "gdb"
	@echo "(gdb) file $(KERNEL_BINARY)"
	@echo "(gdb) target remote localhost:1234"
	@echo "(gdb) break kernel_main"
	@echo "(gdb) continue"

# Clean build files
clean:
	rm -f *.o $(KERNEL_BINARY) serial.log
	rm -rf isodir $(ISO_IMAGE)

.PHONY: all verify iso run run-serial run-log run-iso debug clean
