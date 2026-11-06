set pagination off
set confirm off
set language c
set remotetimeout 15
file /workspace/Kernseed_OS/build/fault-1/kernseed.elf
target remote /tmp/ks2-7wyug68z/gdb
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

hbreak *interrupt_dispatch
ignore 1 1
continue
python
entry_check()
check('((struct interrupt_frame *)$rdi)->vector == 0')
check('((struct interrupt_frame *)$rdi)->error == 0')
print('PASS fault C entry ABI vector 0')
end
delete breakpoints
hbreak cpu_halt
continue
python
check('irq_state.fatal == 1 && irq_state.nested == 0')
check('last_exception.vector == 0 && last_exception.error == 0')
check('last_exception.rip == (unsigned long)&fault_de_rip')
check('last_exception.cs == 8 && last_exception.ss == 0x10')
check('last_exception.rsp >= (unsigned long)&boot_stack_bottom && last_exception.rsp < (unsigned long)&boot_stack_top')
check('irq_state.ticks == 0 && irq_state.timer_eoi == 0')
check('irq_state.exception_cr2 == 0')
check('irq_state.entry_if_errors == 0')
check('irq_state.exception_stack >= (unsigned long)&boot_stack_bottom && irq_state.exception_stack < (unsigned long)&boot_stack_top')
if flags() & 0x200: raise gdb.GdbError('Fault handler enabled IRQs')
print('PASS real fault vector 0: error, RIP, interrupted RSP, CR2, boot stack')
end
p/x last_exception
p/x irq_state
delete breakpoints

detach
quit
