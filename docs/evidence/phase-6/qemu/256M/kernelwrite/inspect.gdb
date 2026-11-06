set pagination off
set confirm off
set language c
set remotetimeout 15
file /workspace/Kernseed_OS/build/kernseed.elf
target remote /tmp/ks5-6jhagmoa/gdb
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
check('user_state.vector == 14 && user_state.error == 7 && user_state.active == 0')
check('memory_state.allocated == 16 && memory_state.mapped == 16 && irq_state.fatal == 0')
check('user_state.fault_rip >= 0xffffc00001000000 && user_state.fault_rip < 0xffffc00001001000')
check('user_state.last_rsp >= 0xffffc00001100000 && user_state.last_rsp < 0xffffc00001101000')
print('PASS user fault kernelwrite: actual vector/error and safe cleanup')
end
p/x user_state

detach
quit
