set pagination off
set confirm off
set language c
set remotetimeout 15
file /workspace/Kernseed_OS/build/fault-9/kernseed.elf
target remote /tmp/ks2-06ismwjw/gdb
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
check('irq_state.fatal == 1 && last_exception.vector == 14 && last_exception.error == 2')
check('irq_state.exception_cr2 == tasks[1].low-4096')
check('last_exception.rsp >= tasks[1].low && last_exception.rsp < tasks[1].high')
check('sched_state.current == 1 && sched_state.errors == 0')
print('PASS actual task stack guard fault 9')
end
p/x last_exception
p/x sched_state
delete breakpoints

detach
quit
