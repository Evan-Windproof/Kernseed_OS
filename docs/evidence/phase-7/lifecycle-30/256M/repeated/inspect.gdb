set pagination off
set confirm off
set language c
set remotetimeout 15
file /workspace/Kernseed_OS/build/kernseed.elf
target remote /tmp/ks5-m_dhs8pd/gdb
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
python
check('user_state.exits == 69 && user_state.active == 0 && user_state.faults == 4')
check('memory_state.allocated == 16 && memory_state.mapped == 16')
check('sched_state.errors == 0 && irq_state.fatal == 0 && register_errors == 0')
print('PASS repeated application lifecycle and resource consistency')
end
p/x user_state
p/x sched_state
p/x memory_state

detach
quit
