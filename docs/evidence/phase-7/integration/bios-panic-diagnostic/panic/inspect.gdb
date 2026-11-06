set pagination off
set confirm off
set language c
set remotetimeout 15
file /workspace/Kernseed_OS/build/kernseed.elf
target remote /tmp/ks7-34xlciju/gdb
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
set *(unsigned long long *)tasks[1].low = 0
hbreak cpu_halt
continue
python
check('sched_state.errors == 1')
if flags() & 0x200:raise gdb.GdbError('Panic left interrupts enabled')
print('PASS isolated corrupted-canary controlled scheduler panic')
end
delete breakpoints

detach
quit
