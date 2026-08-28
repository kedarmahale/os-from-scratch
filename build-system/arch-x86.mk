# x86-specific build configuration

# x86 toolchain. Prefer the freestanding cross-toolchain documented by the
# project, but support Debian's equivalent i686-linux-gnu toolchain as well.
ifeq ($(shell command -v i686-elf-gcc 2>/dev/null),)
X86_TOOLCHAIN_PREFIX = i686-linux-gnu
else
X86_TOOLCHAIN_PREFIX = i686-elf
endif
CC = $(X86_TOOLCHAIN_PREFIX)-gcc
AS = $(X86_TOOLCHAIN_PREFIX)-as
LD = $(X86_TOOLCHAIN_PREFIX)-ld
OBJCOPY = $(X86_TOOLCHAIN_PREFIX)-objcopy

# x86 compiler flags
CFLAGS = $(CFLAGS_COMMON) -m32
ASFLAGS = --32
LDFLAGS = -nostdlib -m elf_i386

# x86-specific source files
BOOT_SOURCES = boot/x86/boot.S
ARCH_HAL_SOURCES = advanced/hal/x86/x86_meow_hal.c \
                   advanced/hal/x86/x86_descriptor_tables.c \
                   advanced/hal/x86/x86_interrupt_tables.c \
                   advanced/hal/x86/x86_interrupt_controller.c \
                   advanced/hal/x86/x86_system_timer.c \
				   advanced/hal/x86/x86_platform_support.c
ASM_SOURCES = advanced/hal/x86/x86_gdt_flush.S \
              advanced/hal/x86/x86_interrupt_handlers.S \
			  advanced/hal/x86/x86_assembly_functions.S \
			  advanced/process/x86/x86_context_switch.S \
			  advanced/syscalls/x86/x86_syscall_entry.S

# Object files
BOOT_OBJECTS = $(BOOT_SOURCES:%.S=$(OBJDIR)/%.o)
ARCH_HAL_OBJECTS = $(ARCH_HAL_SOURCES:%.c=$(OBJDIR)/%.o) $(ASM_SOURCES:%.S=$(OBJDIR)/%.o)

# Linker script
LINKER_SCRIPT = scripts/x86/linker.ld

# QEMU configuration
QEMU = qemu-system-i386
QEMU_FLAGS = -m 512M
GRUB_DIR ?= /usr/lib/grub/i386-pc

# Create bootable ISO (x86 only)
iso: $(KERNEL_ISO)

$(KERNEL_ISO): $(KERNEL_BIN)
	@echo "============================================================="
	@echo "Creating bootable ISO for x86....."
	@mkdir -p $(ISODIR)/boot/grub
	cp $(KERNEL_BIN) $(ISODIR)/boot/meowkernel.bin
	echo 'menuentry "MeowKernel x86" {' > $(ISODIR)/boot/grub/grub.cfg
	echo '    multiboot /boot/meowkernel.bin' >> $(ISODIR)/boot/grub/grub.cfg
	echo '}' >> $(ISODIR)/boot/grub/grub.cfg
	@test -d "$(GRUB_DIR)" || { echo "ERROR: missing x86 GRUB modules at $(GRUB_DIR)"; echo "Run scripts/setup-grub-x86.sh or set GRUB_DIR"; exit 1; }
	grub-mkrescue -d "$(GRUB_DIR)" -o $(KERNEL_ISO) $(ISODIR)
	@xorriso -indev $(KERNEL_ISO) -report_el_torito plain 2>&1 | grep -q 'El Torito catalog' || { echo "ERROR: ISO has no El Torito boot record (install grub-pc-bin)"; exit 1; }
	@echo " ISO created: $(KERNEL_ISO)"
	@echo "============================================================="

# Run in QEMU
run: $(KERNEL_ISO)
	@echo "============================================================="
	@echo " Starting MeowKernel x86 in QEMU..."
	@echo "============================================================="
	$(QEMU) -cdrom $(KERNEL_ISO) $(QEMU_FLAGS)

# Debug with GDB
debug: $(KERNEL_ISO)
	@echo "============================================================="
	@echo " Starting x86 debug session..."
	@echo "============================================================="
	@echo "In another terminal, run: gdb $(KERNEL_BIN)"
	@echo "Then in GDB: target remote localhost:1234"
	@echo "============================================================="
	$(QEMU) -cdrom $(KERNEL_ISO) $(QEMU_FLAGS) -s -S

# Check x86 dependencies
check-deps:
	@echo "Checking x86 build dependencies..."
	@which $(CC) > /dev/null && echo "   $(CC)" || echo "  $(CC) - install an i686 cross-compiler"
	@which $(AS) > /dev/null && echo "   $(AS)" || echo "  $(AS) - install i686 binutils"
	@which $(LD) > /dev/null && echo "   $(LD)" || echo "  $(LD) - install i686 binutils"
	@which $(QEMU) > /dev/null && echo "   $(QEMU)" || echo "   $(QEMU) - run: sudo apt install qemu-system-x86"
	@which grub-mkrescue > /dev/null && echo "   grub-mkrescue" || echo "   grub-mkrescue - run: sudo apt install grub-pc-bin"
	@which xorriso > /dev/null && echo "   xorriso" || echo "   xorriso - run: sudo apt install xorriso"
	@which grub-file > /dev/null && echo "   grub-file" || echo "   grub-file - install GRUB utilities"
