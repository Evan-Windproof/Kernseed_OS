set pagination off
set confirm off
set remotetimeout 15
file /workspace/Kernseed_OS/build/kernseed.elf
target remote /tmp/kernseed-tdrhevrv/gdb
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
flags = int(re.search(r'eflags\s+0x([0-9a-fA-F]+)', flags_text).group(1), 16)
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
