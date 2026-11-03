#!/bin/sh
# SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0
set -eu
cd "$(dirname "$0")/.."
commit=aad3edd370955449717a334f0289dee10e2c5f01
dest=.cache/limine
if [ ! -d "$dest" ]; then
    mkdir -p .cache
    temp=$(mktemp -d .cache/limine-fetch.XXXXXX)
    trap 'rm -rf "$temp"' EXIT HUP INT TERM
    git -C "$temp" init -q
    git -C "$temp" remote add origin https://github.com/limine-bootloader/limine.git
    git -C "$temp" fetch --depth=1 origin "$commit"
    git -C "$temp" checkout --detach FETCH_HEAD
    mv "$temp" "$dest"
    trap - EXIT HUP INT TERM
fi
test "$(git -C "$dest" rev-parse HEAD)" = "$commit"
(cd "$dest" && sha256sum -c ../../third_party/limine/SHA256SUMS)
cmp third_party/limine/limine.h "$dest/limine.h"
make -C "$dest" CC="${HOST_CC:-cc}" limine
