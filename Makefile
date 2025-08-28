# Compiler and tools
CC = i686-elf-gcc
LD = i686-elf-ld
AS = i686-elf-as

# Compiler flags
CFLAGS = -std=gnu99 -ffreestanding -O2 -Wall -Wextra
CFLAGS += -nostdlib -fno-builtin -fno-stack-protector
CFLAGS += -fno-pic -fno-pie -m32

# Assembler flags (GAS/AT&T syntax)
ASFLAGS = --32

# Linker flags
LDFLAGS = -m elf_i386 -nostdlib

# Directories
SRCDIR = .
BUILDDIR = build
OBJDIR = $(BUILDDIR)/obj
BINDIR = $(BUILDDIR)/bin
ISODIR = $(BUILDDIR)/iso

# Include directories
INCLUDES = -Ikernel -Iadvanced/hal -Iadvanced

# Source files
BOOT_SOURCES = boot/boot.S
KERNEL_SOURCES = kernel/kernel.c \
                 advanced/hal/hal.c \
                 advanced/hal/x86/hal_x86.c

# Object files (maintaining directory structure in obj/)
BOOT_OBJECTS = $(BOOT_SOURCES:%.S=$(OBJDIR)/%.o)
KERNEL_OBJECTS = $(KERNEL_SOURCES:%.c=$(OBJDIR)/%.o)
ALL_OBJECTS = $(BOOT_OBJECTS) $(KERNEL_OBJECTS)

# Target files
KERNEL_BIN = $(BINDIR)/kernel.bin
KERNEL_ISO = $(BUILDDIR)/meowkernel.iso

# Default target
.PHONY: all clean iso run debug help dirs

all: dirs $(KERNEL_BIN)

# Create necessary directories
dirs:
	@mkdir -p $(OBJDIR)/boot
	@mkdir -p $(OBJDIR)/kernel
	@mkdir -p $(OBJDIR)/advanced/hal/x86
	@mkdir -p $(BINDIR)
	@mkdir -p $(ISODIR)/boot/grub

# Compile C source files
$(OBJDIR)/%.o: %.c
	@echo "Compiling $<..."
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

# Assemble ASM source files with GAS
$(OBJDIR)/%.o: %.S
	@echo "Assembling $< with GAS..."
	@mkdir -p $(dir $@)
	$(AS) $(ASFLAGS) $< -o $@

# Link kernel binary
$(KERNEL_BIN): $(ALL_OBJECTS) scripts/linker.ld | dirs
	@echo "Linking kernel binary..."
	$(LD) $(LDFLAGS) -T scripts/linker.ld -o $@ $(ALL_OBJECTS)
	@echo "Kernel binary created: $(KERNEL_BIN)"

# Create bootable ISO
iso: $(KERNEL_ISO)

$(KERNEL_ISO): $(KERNEL_BIN)
	@echo "Creating bootable ISO..."
	@mkdir -p $(ISODIR)/boot/grub
	cp $(KERNEL_BIN) $(ISODIR)/boot/kernel.bin
	echo 'menuentry "Meow Kernel" {' > $(ISODIR)/boot/grub/grub.cfg
	echo '    multiboot /boot/kernel.bin' >> $(ISODIR)/boot/grub/grub.cfg
	echo '}' >> $(ISODIR)/boot/grub/grub.cfg
	grub-mkrescue -o $(KERNEL_ISO) $(ISODIR)
	@echo "ISO created: $(KERNEL_ISO)"

# Run kernel in QEMU
run: $(KERNEL_ISO)
	@echo "Starting QEMU..."
	qemu-system-i386 -cdrom $(KERNEL_ISO) -m 512M

# Debug kernel with GDB
debug: $(KERNEL_ISO)
	@echo "Starting QEMU with GDB server..."
	@echo "In another terminal, run: gdb $(KERNEL_BIN)"
	@echo "Then in GDB: target remote localhost:1234"
	qemu-system-i386 -cdrom $(KERNEL_ISO) -m 512M -s -S

# Clean all build artifacts
clean:
	@echo "Cleaning build directory..."
	rm -rf $(BUILDDIR)
	@echo "Clean complete."

# Show build information
info:
	@echo "=== Build Information ==="
	@echo "Assembler: GNU Assembler (GAS)"
	@echo "Syntax: AT&T"
	@echo "Build directory:  $(BUILDDIR)"
	@echo "Target binary: $(KERNEL_BIN)"
	@echo "Target ISO:    $(KERNEL_ISO)"

# Help target
help:
	@echo "Available targets:"
	@echo "  all     - Build kernel binary (default)"
	@echo "  iso     - Create bootable ISO image"
	@echo "  run     - Run kernel in QEMU"
	@echo "  debug   - Run kernel with GDB debugging"
	@echo "  clean   - Remove all build artifacts"
	@echo "  info    - Show build configuration"
	@echo "  help    - Show this help message"
	@echo ""
	@echo "Using GNU Assembler (GAS) with AT&T syntax"

# Tool verification
check-tools:
	@echo "Checking build tools..."
	@which $(CC) > /dev/null && echo "  ✓ GCC found" || echo "  ✗ GCC not found"
	@which $(AS) > /dev/null && echo "  ✓ GAS found" || echo "  ✗ GAS not found"
	@which $(LD) > /dev/null && echo "  ✓ LD found" || echo "  ✗ LD not found"
	@which qemu-system-i386 > /dev/null && echo "  ✓ QEMU found" || echo "  ✗ QEMU not found"

.PHONY: info check-tools

