set pagination off
set confirm off
set language c
set remotetimeout 15
file /workspace/Kernseed_OS/build/kernseed.elf
target remote /tmp/ks2-zm2rejoi/gdb
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
continue
python
entry_check()
check('((struct interrupt_frame *)$rdi)->vector == 128')
check('((struct interrupt_frame *)$rdi)->error == 0')
check('(((struct interrupt_frame *)$rdi)->rflags & 0x400) != 0')
print('PASS software INT: normalized frame and aligned C ABI, DF cleared')
end
delete breakpoints
hbreak kernel_idle
continue
python
check('irq_state.initialized == 1 && irq_state.register_probe_result == 0')
check('irq_state.software_count == 1 && irq_state.entry_if_errors == 0')
check('irq_state.gdtr.base == (unsigned long)&gdt && irq_state.gdtr.limit == 39')
check('irq_state.idtr.base == (unsigned long)&idt && irq_state.idtr.limit == 4095')
check('irq_state.tr == 0x18')
check('kernel_tss.ist[0] == (unsigned long)&df_stack + 16384')
check('kernel_tss.rsp[0] == (unsigned long)&boot_stack_top && kernel_tss.iomap == 104')
check('irq_state.pit_divisor == 11932')
check('irq_state.pic_master_mask == 0xfe && irq_state.pic_slave_mask == 0xff')
memory = gdb.selected_inferior()
raw = memory.read_memory(n('&idt'),4096).tobytes()
stubs = struct.unpack('<256Q', memory.read_memory(n('&isr_table'),2048).tobytes())
for i in range(256):
    lo,sel,ist,attr,mid,hi,res = struct.unpack_from('<HHBBHII',raw,i*16)
    if (lo|(mid<<16)|(hi<<32)) != stubs[i] or sel != 8 or attr != 0x8e or res or ist != (1 if i==8 else 0):
        raise gdb.GdbError('Invalid IDT gate '+str(i))
words = struct.unpack('<5Q',memory.read_memory(n('&gdt'),40).tobytes())
if words[0] or (words[1] & ~(1<<40)) != 0x00af9a000000ffff or (words[2] & ~(1<<40)) != 0x00cf92000000ffff:
    raise gdb.GdbError('Invalid code/data GDT')
tss_base = ((words[3]>>16)&0xffffff)|((words[3]>>32)&0xff000000)|(words[4]<<32)
if tss_base != n('&kernel_tss') or ((words[3]>>40)&255) != 0x8b or (words[3]&65535)!=103:
    raise gdb.GdbError('Invalid busy 64-bit TSS descriptor')
print('PASS all 256 IDT gates, GDTR/IDTR/TR, GDT and DF IST')
end
p/x irq_state
delete breakpoints
hbreak *interrupt_dispatch
continue
python
entry_check()
check('((struct interrupt_frame *)$rdi)->vector == 32')
check('((struct interrupt_frame *)$rdi)->error == 0')
check('(((struct interrupt_frame *)$rdi)->rflags & 0x200) != 0')
check('((struct interrupt_frame *)$rdi)->cs == 8')
check('((struct interrupt_frame *)$rdi)->ss == 0x10')
print('PASS hardware IRQ0 entry and saved IF')
end
delete breakpoints
hbreak kernel_idle_halt
continue
python
check('irq_state.ticks > 0 && irq_state.ticks == irq_state.timer_eoi')
check('irq_state.other_irq == 0 && irq_state.entry_if_errors == 0')
if not flags() & 0x200: raise gdb.GdbError('IRETQ lost IF')
print('PASS IRQ0 IRETQ and EOI')
end
delete breakpoints

detach
quit
