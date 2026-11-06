set pagination off
set confirm off
set language c
set remotetimeout 15
file /workspace/Kernseed_OS/build/kernseed.elf
target remote /tmp/ks5-zg2mir7l/gdb
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
check('tasks[6].status == BLOCKED && keyboard_state.dropped > 0')
check('keyboard_state.errors == 0 && sched_state.errors == 0')
print('PASS actual keyboard FIFO overflow detected')
end
set variable tasks[6].wake_tick=0

detach
quit
