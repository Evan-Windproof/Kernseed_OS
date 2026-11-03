#!/usr/bin/env python3
# SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0
"""Check the linked image, rather than assuming compiler flags were honored."""
import subprocess
import sys

def output(*args):
    return subprocess.check_output(args, text=True)

elf = sys.argv[1]
header = output("readelf", "-hW", elf)
assert "ELF64" in header and "EXEC (Executable file)" in header
assert "Advanced Micro Devices X86-64" in header
assert not output("nm", "-u", elf).strip(), "unresolved symbols"
segments = output("readelf", "-lW", elf)
assert "INTERP" not in segments and "DYNAMIC" not in segments
assert "RWE" not in segments, "writable executable segment"
assert "GNU_STACK" in segments
symbols = output("nm", "-n", elf)
assert " _start\n" in symbols and " kernel_idle_halt\n" in symbols
print("PASS ELF: static x86-64, no unresolved symbols, no RWX segment")
