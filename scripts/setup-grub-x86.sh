#!/bin/sh
set -eu

# Install only the architecture-independent GRUB BIOS modules needed to build
# an x86 ISO. This avoids installing amd64 libc and other foreign libraries on
# ARM64 hosts such as Raspberry Pi.
DEST=${1:-build/tools/grub}
TMP=$(mktemp -d)
trap 'rm -rf "$TMP"' EXIT

mkdir -p "$DEST"
if [ -d "$DEST/usr/lib/grub/i386-pc" ]; then
    echo "x86 GRUB modules already available at $DEST/usr/lib/grub/i386-pc"
    exit 0
fi

echo "Downloading GRUB x86 BIOS modules (no system install)..."
(cd "$TMP" && apt-get download grub-pc-bin:amd64)
DEB=$(find "$TMP" -maxdepth 1 -name 'grub-pc-bin_*.deb' -print -quit)
if [ -z "$DEB" ]; then
    echo "ERROR: could not download grub-pc-bin:amd64" >&2
    exit 1
fi
dpkg-deb -x "$DEB" "$DEST"
echo "x86 GRUB modules extracted to $DEST/usr/lib/grub/i386-pc"
echo "Build with: make test GRUB_DIR=$DEST/usr/lib/grub/i386-pc"
