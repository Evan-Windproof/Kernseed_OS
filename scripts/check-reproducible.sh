#!/bin/sh
# SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0
set -eu
cd "$(dirname "$0")/.."
mkdir -p build
make -B iso > build/reproducible-first.log 2>&1
sha256sum build/kernseed.elf build/kernseed.iso > build/reproducible-first.sha256
make -B iso > build/reproducible-second.log 2>&1
sha256sum build/kernseed.elf build/kernseed.iso > build/reproducible-second.sha256
cmp build/reproducible-first.sha256 build/reproducible-second.sha256
echo 'PASS reproducibility: identical ELF and ISO across two forced rebuilds in this checkout/toolchain'
cat build/reproducible-second.sha256
