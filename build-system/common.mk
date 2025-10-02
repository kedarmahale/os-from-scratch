# Common build rules for all architectures

# Common source files
KERNEL_SOURCES = kernel/meow_kernel_main.c kernel/meow_util.c lib/runtime.c
HAL_SOURCES = advanced/hal/meow_hal_manager.c
MEM_SOURCES = advanced/mm/meow_memory_manager.c \
	    advanced/mm/meow_memory_mapper.c \
        advanced/mm/meow_heap_allocator.c \
	    advanced/mm/meow_physical_memory.c

KERNEL_OBJECTS = $(KERNEL_SOURCES:%.c=$(OBJDIR)/%.o)
HAL_OBJECTS = $(HAL_SOURCES:%.c=$(OBJDIR)/%.o)
MEM_OBJECTS = $(MEM_SOURCES:%.c=$(OBJDIR)/%.o)

# Combined objects
ALL_OBJECTS = $(BOOT_OBJECTS) \
	      $(KERNEL_OBJECTS) \
	      $(HAL_OBJECTS) \
	      $(ARCH_HAL_OBJECTS) \
	      $(MEM_OBJECTS)

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
	@mkdir -p $(BINDIR)
	@mkdir -p $(ISODIR)/boot/grub

# Compile C source files
$(OBJDIR)/%.o: %.c
	@echo "Compiling $< for $(ARCH)..."
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $(INCLUDES) -g -c $< -o $@

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
	@echo "Sources: $(words $(ALL_OBJECTS)) object files"
	
