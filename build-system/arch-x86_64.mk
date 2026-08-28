# Explicit x86_64 Phase 3 target.  It intentionally remains separate from the
# legacy 32-bit kernel until the architecture reaches feature parity.
CC = x86_64-linux-gnu-gcc
AS = x86_64-linux-gnu-as
LD = x86_64-linux-gnu-ld
OBJCOPY = x86_64-linux-gnu-objcopy
KERNEL_SOURCES = kernel/meow_kernel64.c
HAL_SOURCES =
MEM_SOURCES = advanced/mm/x86_64_pmm.c \
              advanced/mm/x86_64_paging.c \
              advanced/mm/x86_64_heap.c
DRIVER_SOURCES =
PROCESS_SOURCES = advanced/process/x86_64/x86_64_context.c \
                  advanced/process/x86_64/x86_64_scheduler.c
SYSCALL_SOURCES =
FILESYSTEM_SOURCES =
CFLAGS = $(CFLAGS_COMMON) -m64 -mcmodel=kernel
ASFLAGS = --64
LDFLAGS = -nostdlib -m elf_x86_64
BOOT_SOURCES = boot/x86_64/boot.S
BOOT_OBJECTS = $(BOOT_SOURCES:%.S=$(OBJDIR)/%.o)
ARCH_HAL_SOURCES = advanced/hal/x86_64/x86_64_idt.c \
                   advanced/hal/x86_64/x86_64_pic_pit.c
ASM_SOURCES = boot/x86_64/interrupts.S
ARCH_ASM_SOURCES = advanced/process/x86_64/x86_64_context_switch.S
LINKER_SCRIPT = scripts/x86_64/linker.ld
QEMU = qemu-system-x86_64
QEMU_FLAGS = -m 512M
GRUB_DIR ?= /usr/lib/grub/i386-pc
TEST_RUNNER = scripts/test-x86_64.sh
KERNEL_ISO = $(BUILDDIR)/meowkernel-x86_64.iso

iso: $(KERNEL_ISO)
$(KERNEL_ISO): $(KERNEL_BIN)
	@echo "Creating bootable x86_64 ISO..."
	@mkdir -p $(ISODIR)/boot/grub
	cp $(KERNEL_BIN) $(ISODIR)/boot/meowkernel.bin
	echo 'set timeout=0' > $(ISODIR)/boot/grub/grub.cfg
	echo 'set timeout_style=hidden' >> $(ISODIR)/boot/grub/grub.cfg
	echo 'menuentry "MeowKernel x86_64" {' >> $(ISODIR)/boot/grub/grub.cfg
	echo '    multiboot /boot/meowkernel.bin' >> $(ISODIR)/boot/grub/grub.cfg
	echo '}' >> $(ISODIR)/boot/grub/grub.cfg
	@test -d "$(GRUB_DIR)" || { echo "ERROR: missing x86 GRUB modules at $(GRUB_DIR)"; exit 1; }
	grub-mkrescue -d "$(GRUB_DIR)" -o $(KERNEL_ISO) $(ISODIR)
	@xorriso -indev $(KERNEL_ISO) -report_el_torito plain 2>&1 | grep -q 'El Torito catalog' || { echo "ERROR: ISO has no El Torito boot record"; exit 1; }

run: $(KERNEL_ISO)
	$(QEMU) -cdrom $(KERNEL_ISO) $(QEMU_FLAGS)

check-deps:
	@which $(CC) >/dev/null || echo "Missing $(CC) (install gcc-x86-64-linux-gnu)"
	@which $(AS) >/dev/null || echo "Missing $(AS) (install binutils-x86-64-linux-gnu)"
	@which $(LD) >/dev/null || echo "Missing $(LD) (install binutils-x86-64-linux-gnu)"
	@which $(QEMU) >/dev/null || echo "Missing $(QEMU) (install qemu-system-x86)"
