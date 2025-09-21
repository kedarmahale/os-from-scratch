# ARM64-specific build configuration

# ARM64 toolchain
CC = aarch64-linux-gnu-gcc
AS = arch64-linux-gnu-as
LD = aarch64-linux-gnu-ld
OBJCOPY = aarch64-linux-gnu-objcopy

# ARM64 compiler flags
CFLAGS = $(CFLAGS_COMMON) -mcpu=cortex-a72 -mgeneral-regs-only
ASFLAGS = 
LDFLAGS = -nostdlib

# ARM64-specific source files
BOOT_SOURCES = boot/arm64/boot.S
ARCH_HAL_SOURCES = advanced/hal/arm64/hal_arm64.c
ASM_SOURCES = 

# Object files
BOOT_OBJECTS = $(BOOT_SOURCES:%.S=$(OBJDIR)/%.o)
ARCH_HAL_OBJECTS = $(ARCH_HAL_SOURCES:%.c=$(OBJDIR)/%.o) $(ASM_SOURCES:%.S=$(OBJDIR)/%.o)

# Linker script
LINKER_SCRIPT = scripts/arm64/linker.ld

# QEMU configuration for Raspberry Pi
QEMU = qemu-system-aarch64
QEMU_FLAGS = -M raspi3b -m 1G -serial stdio

# ARM64-specific targets (no ISO for now)
iso:
	@echo "❌ ISO creation not supported for ARM64 yet"
	@echo "Use 'make run' to test in QEMU"

# Run ARM64 kernel directly
run: $(KERNEL_BIN)
	@echo "🚀 Starting MeowKernel ARM64 in QEMU..."
	$(QEMU) $(QEMU_FLAGS) -kernel $(KERNEL_BIN)

# Debug ARM64
debug: $(KERNEL_BIN)
	@echo "🐛 Starting ARM64 debug session..."
	@echo "In another terminal, run: gdb-multiarch $(KERNEL_BIN)"
	@echo "Then in GDB: target remote localhost:1234"
	$(QEMU) $(QEMU_FLAGS) -kernel $(KERNEL_BIN) -s -S

# Check ARM64 dependencies
check-deps:
	@echo "Checking ARM64 build dependencies..."
	@which $(CC) > /dev/null && echo "  ✅ $(CC)" || echo "  ❌ $(CC) - run: sudo apt install gcc-aarch64-linux-gnu"
	@which $(QEMU) > /dev/null && echo "  ✅ $(QEMU)" || echo "  ❌ $(QEMU) - run: sudo apt install qemu-system-aarch64"a
