#!/usr/bin/env python3
# SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0
"""Real QEMU/TCG boots + QMP display capture + GDB CPU/memory inspection.

Only Phase 1 is tested here. This is not an interrupt, MMU allocator, task,
keyboard or shell test. Every subprocess status and timeout is significant.
"""
import json
import os
from pathlib import Path
import socket
import subprocess
import time
import traceback

ROOT = Path(__file__).resolve().parents[1]
os.chdir(ROOT)
OUT = ROOT / "build" / "test-results" / time.strftime("%Y%m%dT%H%M%SZ", time.gmtime())
OUT.mkdir(parents=True, exist_ok=False)

class QMP:
    def __init__(self, path):
        self.sock = socket.socket(socket.AF_UNIX)
        self.sock.settimeout(5)
        self.sock.connect(str(path))
        self.stream = self.sock.makefile("rwb", buffering=0)
        assert "QMP" in self.read()
        self.command("qmp_capabilities")

    def read(self):
        line = self.stream.readline()
        if not line:
            raise RuntimeError("QMP disconnected")
        return json.loads(line)

    def command(self, name, arguments=None):
        self.stream.write((json.dumps({"execute": name, "arguments": arguments or {}}) + "\n").encode())
        while True:
            response = self.read()
            if "error" in response:
                raise RuntimeError(response)
            if "return" in response:
                return response["return"]

    def close(self):
        self.stream.close()
        self.sock.close()

def wait_for(test, process, description, seconds=30):
    deadline = time.monotonic() + seconds
    while time.monotonic() < deadline:
        if process.poll() is not None:
            raise RuntimeError(f"QEMU exited {process.returncode} while waiting for {description}")
        if test():
            return
        time.sleep(0.05)
    raise TimeoutError(description)

def check_screen(path):
    # QMP PPM is the actual emulated VGA display, not a kernel-produced image.
    with path.open("rb") as stream:
        assert stream.readline().strip() == b"P6"
        width, height = map(int, stream.readline().split())
        assert stream.readline().strip() == b"255"
        pixels = stream.read()
    assert (width, height) == (800, 600)
    assert len(pixels) == width * height * 3
    # Independently assert the top-left rendered K, its 2x scale and black gaps.
    shape = ["#...#", "#..#.", "#.#..", "##...", "#.#..", "#..#.", "#...#"]
    for y in range(18):
        for x in range(12):
            lit = x < 10 and y < 14 and shape[y // 2][x // 2] == "#"
            at = (y * width + x) * 3
            assert pixels[at:at+3] == (b"\xdd\xee\xff" if lit else b"\x00\x00\x00"), (x, y)
    # Check output extends to the final status line, beyond just a title glyph.
    assert any(pixels[8 * 18 * width * 3:9 * 18 * width * 3])

def gdb_check(directory, gdb_socket):
    commands = directory / "check.gdb"
    commands.write_text(f"""set pagination off
set confirm off
set remotetimeout 15
file {ROOT / 'build/kernseed.elf'}
target remote {gdb_socket}
hbreak *kernel_main
continue
info registers rsp eflags
python
import gdb
import re
def number(expr):
    try: return int(gdb.parse_and_eval(expr))
    except gdb.error as exc: raise gdb.GdbError('Cannot evaluate ' + expr + ': ' + str(exc))
def require(expr):
    if not number(expr): raise gdb.GdbError('ASSERT FAILED: ' + expr)
require('((unsigned long)$rsp & 15) == 8')
# GDB exposes eflags as a special flags type, not a C integer.
flags_text = gdb.execute('info registers eflags', to_string=True)
flags = int(re.search(r'eflags\\s+0x([0-9a-fA-F]+)', flags_text).group(1), 16)
if flags & 0x600: raise gdb.GdbError('DF or IF set at C entry')
require('(unsigned long)$rsp >= (unsigned long)&boot_stack_bottom && (unsigned long)$rsp < (unsigned long)&boot_stack_top')
probe = gdb.selected_inferior().read_memory(number('&bss_probe'), 256).tobytes()
if any(probe): raise gdb.GdbError('BSS probe not zero')
require('boot_state.stage == 0')
print('PASS GDB entry: ABI stack, DF/IF, BSS')
end
delete breakpoints
hbreak kernel_idle
continue
python
require('boot_state.stage == 2')
require('boot_state.runtime_result == 0 && boot_state.bss_ok == 1 && boot_state.serial_ok == 1')
require('(boot_state.cr0 & 0x80000001) == 0x80000001')
require('(boot_state.cr4 & 0x20) != 0')
require('(boot_state.efer & 0x500) == 0x500')
require('(boot_state.rflags & 0x200) == 0')
require('boot_state.cr3 != 0 && (boot_state.cr3 & 4095) == 0')
require('boot_state.width == 800 && boot_state.height == 600 && boot_state.bpp == 32')
require('$cr3 == boot_state.cr3')
print('PASS GDB state: long mode, actual page table, runtime, framebuffer')
end
p/x boot_state
info registers rip rsp eflags cr0 cr3 cr4
delete breakpoints
hbreak kernel_idle_halt
continue
x/2i $pc
detach
quit
""")
    result = subprocess.run([os.environ.get("GDB", "gdb"), "-nx", "-batch", "-x", str(commands)],
                            capture_output=True, text=True, timeout=45)
    (directory / "gdb.log").write_text(result.stdout + result.stderr)
    assert result.returncode == 0, f"GDB failed: {result.stderr}"
    assert "PASS GDB entry" in result.stdout and "PASS GDB state" in result.stdout
    assert "ASSERT FAILED" not in result.stdout + result.stderr

def boot(name, memory, debug=False, no_display=False):
    directory = OUT / name
    directory.mkdir()
    # Keep UNIX paths short enough for sockaddr_un even in long checkout paths.
    import tempfile
    with tempfile.TemporaryDirectory(prefix="kernseed-") as sockets:
        qmp_path = Path(sockets) / "qmp"
        gdb_path = Path(sockets) / "gdb"
        args = [os.environ.get("QEMU", "qemu-system-x86_64"), "-machine", "pc,accel=tcg",
                "-cpu", "qemu64", "-m", str(memory), "-smp", "1", "-vga", "none" if no_display else "std",
                "-display", "none", "-cdrom", str(ROOT / "build/kernseed.iso"), "-boot", "d",
                "-nic", "none", "-serial", f"file:{directory / 'serial.log'}", "-monitor", "none",
                "-qmp", f"unix:{qmp_path},server=on,wait=off", "-no-reboot", "-no-shutdown"]
        if debug:
            args += ["-S", "-gdb", f"unix:{gdb_path},server=on,wait=off"]
        (directory / "command.json").write_text(json.dumps(args, indent=2))
        with (directory / "qemu.stderr").open("w") as err:
            process = subprocess.Popen(args, stdout=subprocess.DEVNULL, stderr=err)
            qmp = None
            try:
                wait_for(qmp_path.exists, process, "QMP socket")
                qmp = QMP(qmp_path)
                if debug:
                    wait_for(gdb_path.exists, process, "GDB socket")
                    gdb_check(directory, gdb_path)
                log = directory / "serial.log"
                marker = "PANIC: No framebuffer response" if no_display else "BOOT_READY: entering idle loop"
                wait_for(lambda: log.exists() and marker in log.read_text(), process, marker)
                text = log.read_text()
                if no_display:
                    assert "BOOT_READY" not in text
                else:
                    assert "PANIC" not in text
                    assert "C runtime: selftest passed" in text
                    assert "Framebuffer: 800x600x32" in text
                    time.sleep(2)  # Verify continued halt, rather than momentary output.
                    assert process.poll() is None
                    assert qmp.command("query-status")["status"] == "running"
                    registers = qmp.command("human-monitor-command", {"command-line": "info registers"})
                    (directory / "registers.log").write_text(registers)
                    assert "HLT=1" in registers, "CPU did not settle into HLT"
                    qmp.command("screendump", {"filename": str(directory / "screen.ppm")})
                    check_screen(directory / "screen.ppm")
                qmp.command("quit")
                process.wait(timeout=5)
                assert process.returncode == 0
            finally:
                if qmp:
                    qmp.close()
                if process.poll() is None:
                    process.terminate()
                    try:
                        process.wait(timeout=5)
                    except subprocess.TimeoutExpired:
                        process.kill()
                        process.wait()
    print(f"PASS {name}", flush=True)

results = []
try:
    for name, memory, debug, no_display in [
        ("cold-64M", 64, False, False), ("cold-128M", 128, False, False),
        ("cold-256M", 256, False, False), ("gdb-128M", 128, True, False),
        ("missing-framebuffer", 128, False, True),
    ]:
        boot(name, memory, debug, no_display)
        results.append({"name": name, "result": "passed"})
except Exception:
    (OUT / "failure.log").write_text(traceback.format_exc())
    results.append({"name": name, "result": "failed"})
    raise
finally:
    (OUT / "results.json").write_text(json.dumps(results, indent=2) + "\n")
    print(f"Evidence: {OUT.relative_to(ROOT)}", flush=True)
