#!/bin/sh

set -eu

ISO=${1:?usage: test-x86_64.sh ISO GRUB_DIR}
GRUB_DIR=${2:?usage: test-x86_64.sh ISO GRUB_DIR}
ROOT=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
QEMU=${QEMU:-qemu-system-x86_64}
QEMU_FLAGS=${QEMU_FLAGS:--m 512M}
TMPDIR=$(mktemp -d "${TMPDIR:-/tmp}/meowkernel-x86_64-test.XXXXXX")

cleanup() {
    rm -rf "$TMPDIR"
}
trap cleanup EXIT INT TERM

require_marker() {
    log=$1
    marker=$2
    if ! grep -F "$marker" "$log" >/dev/null 2>&1; then
        echo "ERROR: missing marker '$marker' in $log"
        sed -n '1,160p' "$log" || true
        exit 1
    fi
}

run_normal() {
    log=$TMPDIR/normal.serial
    pts_before=
    for candidate in /dev/pts/[0-9]*; do
        [ -c "$candidate" ] && pts_before="$pts_before $candidate"
    done

    set +e
    timeout 10s "$QEMU" -cdrom "$ISO" $QEMU_FLAGS \
        -display none -serial "file:$log" -no-reboot \
        -monitor pty \
        -device isa-debug-exit,iobase=0xf4,iosize=0x04 &
    qemu_pid=$!
    set -e

    monitor=
    for attempt in $(seq 1 200); do
        for candidate in /dev/pts/[0-9]*; do
            case "$pts_before" in
                *" $candidate"*) continue ;;
            esac
            if [ -c "$candidate" ]; then
                monitor=$candidate
                break
            fi
        done
        [ -n "$monitor" ] && break
        sleep 0.01
    done
    sleep 0.25
    if [ -n "$monitor" ]; then
        # QEMU's PTY monitor avoids filesystem socket restrictions and stays
        # in the same process context as the QEMU child.
        exec 3<>"$monitor"
        printf 'sendkey a\r' >&3
    else
        echo "ERROR: QEMU monitor PTY unavailable"
        kill "$qemu_pid" 2>/dev/null || true
        exit 1
    fi

    set +e
    wait "$qemu_pid"
    status=$?
    set -e
    exec 3>&- 3<&-
    if [ "$status" -ne 1 ]; then
        echo "ERROR: normal x86_64 test exited with status $status"
        sed -n '1,160p' "$log" || true
        exit 1
    fi

    require_marker "$log" 'MEOWTEST:BOOT:PASS'
    require_marker "$log" 'MEOWTEST:IDT:PASS'
    require_marker "$log" 'MEOWTEST:PMM:PASS'
    require_marker "$log" 'MEOWTEST:VM:PASS'
    require_marker "$log" 'MEOWTEST:HEAP:PASS'
    require_marker "$log" 'MEOWTEST:CTX:PASS'
    require_marker "$log" 'MEOWTEST:PIC:PASS'
    require_marker "$log" 'MEOWTEST:PIT:PASS'
    require_marker "$log" 'MEOWTEST:IRQ0:PASS'
    require_marker "$log" 'MEOWTEST:IRQ1:PASS'
    require_marker "$log" 'MEOWTEST:PASS'
    echo "x86_64 interrupt smoke test: PASS"
}

run_exception() {
    vector=$1
    variant="test-exception-$vector"
    exception_iso="build/x86_64-$variant/meowkernel-x86_64.iso"
    log="$TMPDIR/exception-$vector.serial"

    echo "Building x86_64 exception test vector $vector..."
    make --no-print-directory -C "$ROOT" ARCH=x86_64 \
        BUILD_VARIANT="$variant" \
        CFLAGS_EXTRA="-DMEOW_TEST_MODE -DX86_64_TEST_EXCEPTION_VECTOR=$vector" \
        GRUB_DIR="$GRUB_DIR" iso

    set +e
    timeout 10s "$QEMU" -cdrom "$ROOT/$exception_iso" $QEMU_FLAGS \
        -display none -serial "file:$log" -no-reboot \
        -monitor none -device isa-debug-exit,iobase=0xf4,iosize=0x04
    status=$?
    set -e
    if [ "$status" -ne 1 ]; then
        echo "ERROR: exception vector $vector exited with status $status"
        sed -n '1,160p' "$log" || true
        exit 1
    fi
    require_marker "$log" "MEOWTEST:EXCEPTION:$vector:PASS"
    echo "x86_64 exception vector $vector: PASS"
}

run_normal
for vector in 0 3 6 13 14; do
    run_exception "$vector"
done

echo "x86_64 deterministic test suite: PASS"
