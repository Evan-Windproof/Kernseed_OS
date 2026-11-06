set pagination off
set confirm off
set language c
set remotetimeout 15
file /workspace/Kernseed_OS/build/fault-6/kernseed.elf
target remote /tmp/ks2-vv6d5186/gdb
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

hbreak cpu_halt
continue
python
check('memory_state.tests == 1')
check('last_exception.vector == 14 && last_exception.error == 0')
check('irq_state.exception_cr2 == 0xffffc00000000000')
check('last_exception.rip == (unsigned long)&memory_fault_site')
check('irq_state.fatal == 1 && irq_state.nested == 0')
print('PASS hardware page permission fault 6')
end
p/x last_exception
p/x memory_state
delete breakpoints

detach
quit
