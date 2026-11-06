set pagination off
set confirm off
set language c
set remotetimeout 15
file /workspace/Kernseed_OS/build/kernseed.elf
target remote /tmp/ks4-1if7wrc4/gdb
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
hbreak *scheduler_interrupt
continue
python
check('demo_a > 0 && demo_b > 0 && register_errors == 0 && sched_state.errors == 0')
check('sched_state.preemptions > 20 && sched_state.records > 3')
check('tasks[1].yields == 0 && tasks[2].yields == 0')
check('sched_state.yields >= 3 && sched_state.exits == 2 && sched_state.reaped == 2')
check('tasks[4].reclaimed == 1 && tasks[5].reclaimed == 1')
check('memory_state.allocated == 12 && memory_state.mapped == 12')
check('memory_state.free + memory_state.tables + memory_state.allocated == memory_state.usable')
if sum(n(f'tasks[{i}].switches') for i in range(8)) != n('sched_state.switches'):raise gdb.GdbError('Switch statistics disagree')
if sum(n(f'tasks[{i}].preemptions') for i in range(8)) != n('sched_state.preemptions'):raise gdb.GdbError('Preemption statistics disagree')
if sum(n(f'tasks[{i}].status == RUNNING') for i in range(8)) != 1:raise gdb.GdbError('RUNNING state not unique')
rt=sum(n(f'tasks[{i}].runtime_ticks') for i in range(8))
if rt + n('((struct interrupt_frame *)$rdi)->vector == 32') != n('irq_state.ticks'):raise gdb.GdbError('Runtime tick accounting disagrees')
check('irq_state.ticks == irq_state.timer_eoi && irq_state.entry_if_errors == 0')
check('sched_state.observed_a > 0 && sched_state.observed_b > 0')
for i in (1,2,3):
    check('tasks['+str(i)+'].switches > 0')
print('PASS sustained preemption, yield/exit/reap, counters, stack/register sentinels')
end
p/x sched_state
p/x tasks
p/x memory_state
set variable demo_stop=1

detach
quit
