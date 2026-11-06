set pagination off
set confirm off
set language c
set remotetimeout 15
file /workspace/Kernseed_OS/build/kernseed.elf
target remote /tmp/ks2-kokgy2eu/gdb
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

hbreak kernel_idle
continue
python
check('memory_state.tests == 1 && memory_state.allocated == 0 && memory_state.mapped == 0')
check('memory_state.free + memory_state.tables == memory_state.usable')
check('memory_state.cr3 == $cr3')
check('memory_state.tables == 3')
size=n('ceiling')
data=gdb.selected_inferior().read_memory(n('&state'),size).tobytes()
if data.count(1)!=n('memory_state.free') or data.count(3)!=3 or data.count(2):
    raise gdb.GdbError('PMM metadata/counters disagree')
# Independently walk the real four-level tree through the observed HHDM.
pa=n('memory_state.cr3'); va=0xffffc00000000000
for level in (3,2,1):
    at=n('memory_state.hhdm')+pa+((va>>(12+9*level))&511)*8
    e=struct.unpack('<Q',gdb.selected_inferior().read_memory(at,8).tobytes())[0]
    if not e&1 or e&128: raise gdb.GdbError('Expected owned 4K tree')
    pa=e&0x000ffffffffff000
leaf=gdb.selected_inferior().read_memory(n('memory_state.hhdm')+pa,4096).tobytes()
if any(leaf):raise gdb.GdbError('Unmapped leaf not zero')
print('PASS physical allocator counters and actual CR3/HHDM page tables')
end
p/x memory_state
delete breakpoints

detach
quit
