# x86-specific build configuration

# x86 toolchain
CC = i686-elf-gcc
AS = i686-elf-as
LD= i686-elf-ld
OBJCOPY = i686-elf-objcopy

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

# x86-specific targets
KERNEL_ISO = $(BUILDDIR)/meowkernel-x86.iso

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
	grub-mkrescue -o $(KERNEL_ISO) $(ISODIR)
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
	@which $(CC) > /dev/null && echo "   $(CC)" || echo "  $(CC) - run: sudo apt install gcc-multilib"
	@which $(QEMU) > /dev/null && echo "   $(QEMU)" || echo "   $(QEMU) - run: sudo apt install qemu-system-x86"
	@which grub-mkrescue > /dev/null && echo "   grub-mkrescue" || echo "   grub-mkrescue - run: sudo apt install grub-pc-bin"
	@which xorriso > /dev/null && echo "   xorriso" || echo "   xorriso - run: sudo apt install xorriso" 
