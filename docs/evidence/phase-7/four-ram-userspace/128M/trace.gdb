set pagination off
set confirm off
set language c
file /workspace/Kernseed_OS/build/kernseed.elf
target remote /tmp/ks5-zg_r8r7b/gdb
python

import gdb, re, struct
def n(expression): return int(gdb.parse_and_eval(expression))
def check(expression):
    if not n(expression): raise gdb.GdbError('ASSERT FAILED: ' + expression)
def flags():
    return int(re.search(r'eflags\s+0x([0-9a-fA-F]+)', gdb.execute('info registers eflags', to_string=True)).group(1),16)
def entry_check():
    check('((unsigned long)$rsp & 15) == 8')
    if flags() & 0x600: raise gdb.GdbError('IF/DF set at ISR C entry')

end
hbreak *0xffffc00001000000
continue
python
check('$cs == 0x2b && $ss == 0x33')
check('(unsigned long)$rsp >= 0xffffc00001100000 && (unsigned long)$rsp < 0xffffc00001101000')
print('PASS actual CPU CPL3 execution')
end
info registers cs ss rsp cr3
x/4i $pc
delete breakpoints
hbreak *syscall_dispatch
continue
python
entry_check()
check('$cs == 8 && ((struct interrupt_frame *)$rdi)->cs == 0x2b')
check('(unsigned long)$rsp >= tasks[sched_state.current].low && (unsigned long)$rsp < tasks[sched_state.current].high')
print('PASS actual CPL3 to CPL0 syscall and kernel stack')
end
info registers cs ss rsp
delete breakpoints
detach
quit
