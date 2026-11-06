#!/bin/sh
# SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0
set -eu
cd "$(dirname "$0")/.."
# Keep existing test evidence and normal artifacts. Each iteration starts from
# an absent, dedicated output tree, at the SAME path (DWARF is path-sensitive).
repro_build=build/reproducible-clean
mkdir -p build
for pass in first second; do
    make BUILD="$repro_build" clean > "build/reproducible-$pass.log" 2>&1
    make BUILD="$repro_build" iso >> "build/reproducible-$pass.log" 2>&1
    sha256sum "$repro_build/kernseed.elf" "$repro_build/kernseed.iso" > "build/reproducible-$pass.sha256"
done
cmp build/reproducible-first.sha256 build/reproducible-second.sha256
echo 'PASS reproducibility: identical ELF and ISO across two clean output trees in this checkout/toolchain'
cat build/reproducible-second.sha256
