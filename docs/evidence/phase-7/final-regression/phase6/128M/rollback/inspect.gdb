set pagination off
set confirm off
set language c
file /workspace/Kernseed_OS/build/kernseed.elf
target remote /tmp/ks5-04k7xu2n/gdb
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
hbreak *user_run
continue
set variable fail_after=2
finish
python
check('(int)$eax == -1 && user_state.active == 0')
check('memory_state.allocated == 16 && memory_state.mapped == 16')
check('memory_state.free + memory_state.tables + memory_state.allocated == memory_state.usable')
print('PASS user creation failure restores user pages and partial kernel stack')
end
set variable fail_after=-1
delete breakpoints
detach
quit
