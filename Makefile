# MeowKernel Main Makefile
# This file orchestrates the build system

# Project information
PROJECT_NAME = MeowKernel
VERSION = 0.1.0
BUILD_DATE = $(shell date '+%Y-%m-%d %H:%M:%S')

# Default architecture
ARCH ?= x86
BUILD_VARIANT ?=
BUILD_SUFFIX = $(if $(strip $(BUILD_VARIANT)),-$(BUILD_VARIANT),)

# These paths are needed while parsing architecture-specific target rules.
BUILDDIR = build/$(ARCH)$(BUILD_SUFFIX)
OBJDIR = $(BUILDDIR)/obj
BINDIR = $(BUILDDIR)/bin
ISODIR = $(BUILDDIR)/iso
KERNEL_BIN = $(BINDIR)/meowkernel.bin
KERNEL_ISO = $(BUILDDIR)/meowkernel-$(ARCH).iso

# Architecture rules must be parsed before common link prerequisites are
# expanded, so architecture-specific objects are included in the dependency
# graph.
include build-system/arch-$(ARCH).mk
include build-system/common.mk

# Show help by default if no target specified
.DEFAULT_GOAL := help

# Architecture shortcuts
.PHONY: x86 x86_64 arm64 rpi clean-all help info

x86:
	@$(MAKE) ARCH=x86 all

x86_64:
	@$(MAKE) ARCH=x86_64 all

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
	@echo "  make x86_64           - Build explicit x86_64 kernel"
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

# About MeowKernel
about:
	@echo "🐱 ABOUT MEOWKERNEL"
	@echo "==================="
	@echo "MeowKernel - The Purr-fect Operating System!"
	@echo ""
	@echo "A cat-themed kernel built from scratch"
	@echo "with love for felines and systems programming."
	@echo ""
	@echo "Version: $(VERSION)"
	@echo "Build Date: $(BUILD_DATE)"
	@echo ""
	@echo "🐾 Happy coding with cats!"
