set pagination off
set confirm off
set language c
set remotetimeout 15
file /workspace/Kernseed_OS/build/kernseed.elf
target remote /tmp/ks4-6i4mhpl9/gdb
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
hbreak kernel_idle_halt if sched_state.reaped == 27
continue
python
check('sched_state.current == 0 && sched_state.exits == 27 && sched_state.reaped == 27')
check('memory_state.allocated == 0 && memory_state.mapped == 0')
check('sched_state.recycle_passed == 16 && sched_state.capacity_passed == 1 && sched_state.create_failures == 2')
check('sched_state.idle_ticks > 0 && register_errors == 0 && sched_state.errors == 0')
for i in (1,2,3,4,5):check('tasks['+str(i)+'].status == TERMINATED && tasks['+str(i)+'].reclaimed == 1')
print('PASS empty ready queue, real idle and complete data-stack reclamation')
end
p/x sched_state
p/x memory_state
delete breakpoints

detach
quit
