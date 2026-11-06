set pagination off
set confirm off
set language c
set remotetimeout 15
file /workspace/Kernseed_OS/build/kernseed.elf
target remote /tmp/ks7-p5tdfclm/gdb
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
hbreak *scheduler_interrupt if sched_state.reaped == 2
continue
python
check('boot_state.stage == 2 && boot_state.bss_ok == 1 && boot_state.runtime_result == 0')
check('irq_state.initialized == 1 && irq_state.ticks > 0 && irq_state.ticks == irq_state.timer_eoi')
check('irq_state.fatal == 0 && irq_state.entry_if_errors == 0 && sched_state.errors == 0 && register_errors == 0')
check('irq_state.pic_master_mask == 0xfc && irq_state.pic_slave_mask == 0xff')
check('memory_state.tests == 1 && memory_state.allocated == 16 && memory_state.mapped == 16')
check('memory_state.free + memory_state.tables + memory_state.allocated == memory_state.usable')
check('shell_state.ready == 1 && user_state.active == 0')
# Read actual CR3 page tables, including inherited large leaves.
def permissions(address):
    table=n('memory_state.cr3');hhdm=n('memory_state.hhdm')
    writable=True;user=True;nx=False
    for level in (3,2,1,0):
        offset=((address>>(12+9*level))&511)*8
        entry=int.from_bytes(bytes(gdb.selected_inferior().read_memory(hhdm+table+offset,8)),'little')
        if not entry&1:raise gdb.GdbError('Absent kernel page')
        writable=writable and bool(entry&2);user=user and bool(entry&4);nx=nx or bool(entry&(1<<63))
        if level==0 or (level in (1,2) and entry&128):return writable,user,nx
        table=entry&0x000ffffffffff000
text=permissions(n('(unsigned long)&_start'))
stack=permissions(n('(unsigned long)&boot_stack_bottom'))
if text!=(False,False,False) or stack!=(True,False,True):raise gdb.GdbError('Unexpected kernel text/stack permissions')
print('PASS actual kernel RX supervisor text, RW NX supervisor stack, CPU and resource invariants')
end
p/x boot_state
p/x irq_state
p/x sched_state
p/x memory_state
delete breakpoints

detach
quit
