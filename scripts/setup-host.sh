#!/bin/sh
# SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0
set -eu
if [ "$(id -u)" != 0 ]; then
    echo 'Run on Debian/Ubuntu: sudo sh scripts/setup-host.sh' >&2
    exit 1
fi
apt-get update
DEBIAN_FRONTEND=noninteractive apt-get install -y --no-install-recommends \
    build-essential binutils git ca-certificates python3 qemu-system-x86 gdb xorriso
