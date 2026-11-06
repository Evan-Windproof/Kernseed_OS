#!/bin/sh
# SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0
set -eu
cd "$(dirname "$0")/.."
mode=${1:-run}
if [ "$#" -gt 0 ]; then shift; fi
case "$mode" in
    run) set -- -display "${QEMU_DISPLAY:-gtk}" "$@" ;;
    headless) set -- -display none "$@" ;;
    debug) set -- -display none -S -gdb tcp:127.0.0.1:1234 "$@" ;;
    *) echo 'Usage: scripts/run.sh [run|headless|debug] [QEMU options]' >&2; exit 2 ;;
esac
exec "${QEMU:-qemu-system-x86_64}" -machine pc,accel=tcg -cpu qemu64 \
    -m 128M -smp 1 -vga std -cdrom "${KERNSEED_BUILD:-build}/kernseed.iso" -boot d \
    -nic none -serial stdio -monitor none -no-reboot -no-shutdown "$@"
