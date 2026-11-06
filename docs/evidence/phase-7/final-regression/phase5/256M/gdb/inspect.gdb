set pagination off
set confirm off
set language c
set remotetimeout 15
file /workspace/Kernseed_OS/build/kernseed.elf
target remote /tmp/ks5-s8mrwa1_/gdb
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
check('shell_state.ready == 1 && shell_state.commands >= 18 && shell_state.overlong == 1')
check('keyboard_state.ready == 1 && keyboard_state.irqs > 0 && keyboard_state.bytes > 0')
check('keyboard_state.dropped == 0 && keyboard_state.errors == 0')
check('irq_state.other_irq == keyboard_state.irqs')
check('sched_state.preemptions > 0 && register_errors == 0 && sched_state.errors == 0')
check('memory_state.allocated == 16 && memory_state.mapped == 16')
check('irq_state.pic_master_mask == 0xfc && irq_state.pic_slave_mask == 0xff')
print('PASS real IRQ1 input, bounded shell, preemption, memory and registry')
end
p/x keyboard_state
p/x shell_state
p/x memory_state
p/x sched_state
delete breakpoints

detach
quit
