# MeowKernel Main Makefile
# This file orchestrates the build system

# Default architecture
ARCH ?= x86

# Include architecture-specific build rules
include build-system/arch-$(ARCH).mk
include build-system/common.mk

# Show help by default if no target specified
.DEFAULT_GOAL := help

# Architecture shortcuts
.PHONY: x86 arm64 rpi clean-all help info

x86:
	@$(MAKE) ARCH=x86 all

arm64:
	@$(MAKE) ARCH=arm64 all

rpi: arm64

clean-all:
	@echo "Cleaning all architectures..."
	rm -rf build
	@echo "All build artifacts cleaned."

help:
	@echo "MeowKernel Build System"
	@echo ""
	@echo "Architecture Selection:"
	@echo "  make x86               - Build for x86"
	@echo "  make arm64             - Build for ARM64/Raspberry Pi 5"
	@echo "  make rpi               - Alias for arm64"
	@echo ""
	@echo "Build Targets:"
	@echo "  make ARCH=x86          - Build kernel for x86"
	@echo "  make ARCH=arm64        - Build kernel for ARM64"
	@echo "  make iso               - Create bootable ISO (x86 only)"
	@echo "  make run               - Run in QEMU"
	@echo "  make debug             - Debug with GDB"
	@echo ""
	@echo "Utilities:"
	@echo "  make clean             - Clean current architecture"
	@echo "  make clean-all         - Clean all architectures"
	@echo "  make info              - Show build configuration"
	@echo "  make check-deps        - Check build dependencies"

info:
	@echo "=== MeowKernel Build Information ==="
	@echo "Current Architecture: $(ARCH)"
	@echo "Build Directory: $(BUILDDIR)"
	@echo "Compiler: $(CC)"
	@echo "Kernel Binary: $(KERNEL_BIN)"

