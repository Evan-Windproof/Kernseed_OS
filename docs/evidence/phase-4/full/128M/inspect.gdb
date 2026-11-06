set pagination off
set confirm off
set language c
set remotetimeout 15
file /workspace/Kernseed_OS/build/kernseed.elf
target remote /tmp/ks4-zu57t8aa/gdb
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

hbreak *counter_task_a
continue
python
check('sched_state.current == 1 && tasks[1].status == RUNNING')
check('sched_state.rollback_passed == 1 && sched_state.create_failures == 1')
check('irq_state.ticks > 0 && irq_state.ticks == irq_state.timer_eoi')
check('((unsigned long)$rsp & 15) == 8')
print('PASS first real IRQ0 stack switch')
end
delete breakpoints
hbreak *counter_task_b
continue
python
check('demo_a > 0 && sched_state.preemptions > 0')
check('tasks[1].preemptions > 0 && tasks[1].status == READY')
check('tasks[1].context->rsp >= tasks[1].low && tasks[1].context->rsp < tasks[1].high')
check('tasks[1].physical != tasks[2].physical && tasks[1].low != tasks[2].low')
for i,r in enumerate(['rax','rbx','rcx','rdx','rbp','rdi','rsi','r8','r9','r10','r11','r12','r13','r14','r15']):
    check('tasks[1].context->'+r+' == '+str(0x4b53000000000201+i))
check('(tasks[1].context->rflags & 0x400) != 0')
print('PASS preempted A without yield; all GPRs and independent stack saved')
end
p/x sched_state
delete breakpoints

detach
quit
