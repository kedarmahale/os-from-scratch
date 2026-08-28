# MeowKernel

MeowKernel is an educational operating-system kernel built from scratch. The
current development target is an explicit x86_64 kernel loaded by GRUB using
Multiboot v1. Legacy 32-bit x86 and ARM64 build configurations remain in the
repository, but they are not yet at x86_64 feature parity.

## Current working features

The x86_64 kernel currently provides:

- GRUB/Multiboot boot and a 32-bit-to-long-mode transition.
- Identity mapping of the first 1 GiB during early boot using 2 MiB pages.
- A 64-bit GDT and IDT with dedicated exception and IRQ entry stubs.
- One canonical ring-0 interrupt frame shared by assembly and C.
- Exception handling for vectors 0–31, including normalized error codes and `iretq` return.
- Remapped 8259 PIC vectors 32–47, IRQ registration, masking, and EOI handling.
- PIT channel 0 configured at 100 Hz, with working timer IRQ0 and keyboard IRQ1.
- A Multiboot memory-map-aware physical page allocator with reserved-region protection, allocation/free, reuse, exhaustion, invalid-free, and double-free checks.
- Initial x86_64 page-table mapping, unmapping, translation, and page-fault validation.
- A fixed 16 MiB x86_64 kernel heap backed by PMM frames and page mappings, with alignment, splitting/coalescing, realloc/calloc, invalid-free, double-free, and capacity checks.
- A cooperative x86_64 context switcher that saves/restores the ABI-required callee-saved registers, stack pointer, instruction pointer, and flags, with independent initial task stacks.
- VGA text output and a serial/debug test mode.

The x86_64 target is intentionally still ring-0 only. The legacy 32-bit heap,
full task management, scheduler, filesystem, syscall, and shell implementations
have not yet been ported to this target.

## Prerequisites

On Debian or Ubuntu, install the host tools and x86_64 cross-toolchain:

```bash
sudo apt update
sudo apt install build-essential make gcc-x86-64-linux-gnu binutils-x86-64-linux-gnu \
  qemu-system-x86 grub-pc-bin xorriso
```

The build uses these commands:

```text
x86_64-linux-gnu-gcc
x86_64-linux-gnu-as
x86_64-linux-gnu-ld
qemu-system-x86_64
grub-mkrescue
xorriso
```

If the host does not have GRUB BIOS modules at `/usr/lib/grub/i386-pc`, use a
project-local module directory:

```bash
sh scripts/setup-grub-x86.sh
```

Then pass:

```text
GRUB_DIR=build/tools/grub/usr/lib/grub/i386-pc
```

## Build and run

Run commands from the repository root:

```bash
# Show the selected toolchain and object list
make ARCH=x86_64 info

# Check tool availability
make ARCH=x86_64 check-deps

# Build a bootable x86_64 ISO
make ARCH=x86_64 \
  GRUB_DIR=build/tools/grub/usr/lib/grub/i386-pc \
  iso

# Launch the normal kernel in QEMU
make ARCH=x86_64 \
  GRUB_DIR=build/tools/grub/usr/lib/grub/i386-pc \
  run
```

The normal image displays:

```text
 /\_/\\
( o.o )
 > ^ <

MeowKernel
Architecture: x86_64
Long mode: enabled
Kernel entry: 64-bit
```

The normal kernel remains running in QEMU. Close the QEMU window to stop it.

## Deterministic tests

Use the x86_64 test target for the authoritative status check:

```bash
make ARCH=x86_64 \
  GRUB_DIR=build/tools/grub/usr/lib/grub/i386-pc \
  test
```

The test target creates isolated test builds and launches QEMU with serial
output plus the `isa-debug-exit` device. It checks:

- boot, IDT, PMM, and virtual-memory initialization;
- physical page allocation, free/reuse, invalid free, double free, exhaustion, and recovery;
- heap allocation, alignment, splitting/coalescing, realloc/calloc, invalid free, double free, and capacity failure;
- two independent kernel contexts alternating across separate stacks for repeated rounds;
- PIT timer IRQ0 and PS/2 keyboard IRQ1;
- divide error, breakpoint/`iretq`, invalid opcode, general protection fault, and page fault.

A successful run ends with:

```text
x86_64 deterministic test suite: PASS
```

The `char device redirected to /dev/pts/...` message from QEMU is expected.
Timeouts and unexpected QEMU exit statuses are failures. The test runner
selects the PTY created by QEMU, so it also works from an SSH or Raspberry Pi
terminal with an existing `/dev/pts/*` device.

## Source layout

```text
boot/x86_64/boot.S                 Multiboot entry and long-mode setup
boot/x86_64/interrupts.S           x86_64 exception and IRQ stubs
advanced/hal/x86_64/               x86_64 IDT, PIC, and PIT implementation
advanced/mm/x86_64_pmm.c           physical page-frame allocator
advanced/mm/x86_64_paging.c        page-table mapping and translation
advanced/mm/x86_64_heap.c          page-backed kernel heap allocator
advanced/process/x86_64/            saved-context ABI and cooperative switcher
kernel/meow_kernel64.c             x86_64 kernel entry and test scenarios
build-system/arch-x86_64.mk        x86_64 build configuration
scripts/test-x86_64.sh             deterministic QEMU test runner
scripts/x86_64/linker.ld            x86_64 linker script
```

The x86_64 sources are kept separate from the legacy 32-bit implementation so
old register layouts and architecture assumptions are not silently reused.

## Build variants and cleanup

Test-only compiler definitions are isolated using `BUILD_VARIANT`; for example,
the test target creates `build/x86_64-test` and exception-specific directories.

```bash
# Clean only the default x86_64 build
make ARCH=x86_64 clean

# Remove all generated build artifacts
make clean-all
```

Generated files are not source and can be rebuilt at any time.

## Development roadmap

Completed initial stabilization milestones:

1. x86_64 boot and long-mode entry.
2. Ring-0 interrupt foundation with 8259 PIC/PIT.
3. Physical page-frame allocator.
4. Initial x86_64 paging and virtual-memory APIs.
5. Page-backed x86_64 kernel heap.
6. Cooperative x86_64 task context switching.

Next work is integrating the context switcher with task objects and the
scheduler. Preemptive scheduling, user mode, SMP, networking, ARM64 parity, and
higher-level phase2 services remain deferred until the foundations are complete.

See [meowkernel_phase3_plan.md](../meowkernel_phase3_plan.md) for the detailed
implementation plan and milestone status.
