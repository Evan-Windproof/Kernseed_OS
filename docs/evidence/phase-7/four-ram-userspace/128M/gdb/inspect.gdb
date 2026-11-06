set pagination off
set confirm off
set language c
set remotetimeout 15
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
python
check('user_state.preemptions > 0')
check('user_state.calls > 0 && user_state.last_cs == 0x2b')
check('user_state.active == 0 && user_state.exits == 9 && user_state.faults == 4')
check('user_state.vector == 14 && user_state.error & 4')
check('user_state.invalid == 4 && irq_state.fatal == 0 && sched_state.errors == 0')
check('memory_state.allocated == 16 && memory_state.mapped == 16')
print('PASS actual Ring3 calls, invalid pointers/numbers, faults and reclamation')
end
p/x user_state
p/x memory_state

detach
quit
