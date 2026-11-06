set pagination off
set confirm off
set language c
set remotetimeout 15
file /workspace/Kernseed_OS/build/fault-1/kernseed.elf
target remote /tmp/ks2-9_enmq8w/gdb
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
print('SAMPLE',n('irq_state.ticks'),n('irq_state.timer_eoi'),n('irq_state.other_irq'),n('irq_state.fatal'),flush=True)
end
delete breakpoints
detach
quit
