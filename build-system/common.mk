# Common build rules for all architectures

# Common source files
KERNEL_SOURCES = kernel/meow_kernel_main.c kernel/meow_util.c kernel/meow_shell.c lib/runtime.c
HAL_SOURCES = advanced/hal/meow_hal_manager.c
MEM_SOURCES = advanced/mm/meow_memory_manager.c \
              advanced/mm/meow_memory_mapper.c \
              advanced/mm/meow_heap_allocator.c \
              advanced/mm/meow_physical_memory.c

# Phase 2 additional source files
DRIVER_SOURCES = advanced/drivers/keyboard/meow_keyboard.c \
                 advanced/drivers/serial/meow_serial.c
PROCESS_SOURCES = advanced/process/meow_task.c \
                  advanced/process/meow_scheduler.c
SYSCALL_SOURCES = advanced/syscalls/meow_syscall_table.c
FILESYSTEM_SOURCES = advanced/fs/vfs/meow_vfs.c \
                     advanced/fs/ramfs/meow_ramfs.c \
                     advanced/fs/devfs/meow_devfs.c

#BOOT_OBJECTS = 
KERNEL_OBJECTS = $(KERNEL_SOURCES:%.c=$(OBJDIR)/%.o)
HAL_OBJECTS = $(HAL_SOURCES:%.c=$(OBJDIR)/%.o)
MEM_OBJECTS = $(MEM_SOURCES:%.c=$(OBJDIR)/%.o)
DRIVER_OBJECTS = $(DRIVER_SOURCES:%.c=$(OBJDIR)/%.o)
PROCESS_OBJECTS = $(PROCESS_SOURCES:%.c=$(OBJDIR)/%.o)
SYSCALL_OBJECTS = $(SYSCALL_SOURCES:%.c=$(OBJDIR)/%.o)
FILESYSTEM_OBJECTS = $(FILESYSTEM_SOURCES:%.c=$(OBJDIR)/%.o)
ARCH_HAL_OBJECTS = $(ARCH_HAL_SOURCES:%.c=$(OBJDIR)/%.o) $(ASM_SOURCES:%.S=$(OBJDIR)/%.o) $(ARCH_ASM_SOURCES:%.S=$(OBJDIR)/%.o)

# Combined objects
ALL_OBJECTS = $(BOOT_OBJECTS) \
              $(KERNEL_OBJECTS) \
              $(HAL_OBJECTS) \
              $(ARCH_HAL_OBJECTS) \
              $(MEM_OBJECTS) \
              $(DRIVER_OBJECTS) \
              $(PROCESS_OBJECTS) \
              $(SYSCALL_OBJECTS) \
              $(FILESYSTEM_OBJECTS)

# Common compiler flags
CFLAGS_COMMON = -std=gnu99 -ffreestanding -O2 -Wall -Wextra
CFLAGS_COMMON += -nostdlib -fno-builtin -fno-stack-protector -fno-pic -fno-pie

# Include directories
INCLUDES = -Ikernel -Iadvanced/hal -Iadvanced

# Build directories
BUILDDIR = build/$(ARCH)
OBJDIR = $(BUILDDIR)/obj
BINDIR = $(BUILDDIR)/bin
ISODIR = $(BUILDDIR)/iso

# Target files
KERNEL_BIN = $(BINDIR)/meowkernel.bin

# Common build rules
.PHONY: all clean iso run debug dirs info check-deps

all: info dirs $(KERNEL_BIN)

# Create necessary directories
dirs:
	@mkdir -p $(OBJDIR)/boot/$(ARCH)
	@mkdir -p $(OBJDIR)/kernel
	@mkdir -p $(OBJDIR)/advanced/hal/$(ARCH)
	@mkdir -p $(OBJDIR)/advanced/mm
	@mkdir -p $(OBJDIR)/advanced/drivers/keyboard
	@mkdir -p $(OBJDIR)/advanced/drivers/serial
	@mkdir -p $(OBJDIR)/advanced/process/x86
	@mkdir -p $(OBJDIR)/advanced/syscalls/x86
	@mkdir -p $(OBJDIR)/advanced/fs/vfs
	@mkdir -p $(OBJDIR)/advanced/fs/ramfs
	@mkdir -p $(OBJDIR)/advanced/fs/devfs
	@mkdir -p $(BINDIR)
	@mkdir -p $(ISODIR)/boot/grub

# Compile C source files
$(OBJDIR)/%.o: %.c
	@echo "Compiling $< for $(ARCH)..."
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS_COMMON) $(CFLAGS) $(INCLUDES) -g -c $< -o $@

# Assemble ASM source files
$(OBJDIR)/%.o: %.S
	@echo "Assembling $< for $(ARCH)..."
	@mkdir -p $(dir $@)
	$(AS) $(ASFLAGS) $< -o $@

# Link kernel binary
$(KERNEL_BIN): $(ALL_OBJECTS) $(LINKER_SCRIPT) | dirs
	@echo "Linking MeowKernel for $(ARCH)..."
	$(LD) $(LDFLAGS) -T $(LINKER_SCRIPT) -o $@ $(ALL_OBJECTS)
	@echo "MeowKernel binary created: $(KERNEL_BIN)"

# Clean current architecture
clean:
	@echo "Cleaning $(ARCH) build..."
	rm -rf $(BUILDDIR)
	@echo "$(ARCH) build cleaned."

# Build information
info:
	@echo "=== MeowKernel Build Configuration ==="
	@echo "Architecture: $(ARCH)"
	@echo "Compiler: $(CC)"
	@echo "Assembler: $(AS)"
	@echo "Linker: $(LD)"
	@echo "Build Dir: $(BUILDDIR)"
	@echo "Kernel Binary: $(KERNEL_BIN)"
	@echo "Object files:" $(words $(ALL_OBJECTS))
