#!/usr/bin/env python3
# SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0
"""Real CPU faults in separate images, descriptor inspection and IRQ0 progress.
Fault images deliberately halt; a successful process start or text alone cannot pass.
"""
import hashlib
import json
import os
import re
from pathlib import Path
import subprocess
import tempfile
import time
import traceback
from boot import QMP, wait_for, ROOT

OUT = ROOT / 'build' / 'phase2-results' / time.strftime('%Y%m%dT%H%M%SZ', time.gmtime())
if __name__ == "__main__":
    OUT.mkdir(parents=True, exist_ok=False)

GDB_HELPERS = r'''
import gdb, re, struct
def n(expression): return int(gdb.parse_and_eval(expression))
def check(expression):
    if not n(expression): raise gdb.GdbError('ASSERT FAILED: ' + expression)
def flags():
    return int(re.search(r'eflags\s+0x([0-9a-fA-F]+)', gdb.execute('info registers eflags', to_string=True)).group(1),16)
def entry_check():
    check('((unsigned long)$rsp & 15) == 8')
    if flags() & 0x600: raise gdb.GdbError('IF/DF set at ISR C entry')
'''

def gdb_run(directory, sock, elf, body, required):
    script = directory / 'inspect.gdb'
    script.write_text(f'''set pagination off
set confirm off
set language c
set remotetimeout 15
file {elf}
target remote {sock}
python
{GDB_HELPERS}
end
{body}
detach
quit
''')
    p = subprocess.run([os.environ.get('GDB','gdb'), '-nx','-batch','-x',str(script)],
                       capture_output=True,text=True,timeout=45)
    (directory/'gdb.log').write_text(p.stdout+p.stderr)
    assert p.returncode == 0, p.stdout+p.stderr
    for marker in required: assert marker in p.stdout, marker

NORMAL = r'''
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
'''

def fault_body(vector, error, rip, df=False):
    return f'''
hbreak *interrupt_dispatch
ignore 1 1
continue
python
entry_check()
check('((struct interrupt_frame *)$rdi)->vector == {vector}')
check('((struct interrupt_frame *)$rdi)->error == {error}')
print('PASS fault C entry ABI vector {vector}')
end
delete breakpoints
hbreak cpu_halt
continue
python
check('irq_state.fatal == 1 && irq_state.nested == 0')
check('last_exception.vector == {vector} && last_exception.error == {error}')
check('last_exception.rip == (unsigned long)&{rip}')
check('last_exception.cs == 8 && last_exception.ss == 0x10')
check('last_exception.rsp >= (unsigned long)&boot_stack_bottom && last_exception.rsp < (unsigned long)&boot_stack_top')
check('irq_state.ticks == 0 && irq_state.timer_eoi == 0')
check('irq_state.exception_cr2 == {"0x400000000000" if vector == 14 else "0"}')
check('irq_state.entry_if_errors == 0')
check('{"irq_state.exception_stack >= (unsigned long)&df_stack && irq_state.exception_stack + sizeof(struct interrupt_frame) <= (unsigned long)&df_stack + 16384" if df else "irq_state.exception_stack >= (unsigned long)&boot_stack_bottom && irq_state.exception_stack < (unsigned long)&boot_stack_top"}')
if flags() & 0x200: raise gdb.GdbError('Fault handler enabled IRQs')
print('PASS real fault vector {vector}: error, RIP, interrupted RSP, CR2, {"DF IST" if df else "boot stack"}')
end
p/x last_exception
p/x irq_state
delete breakpoints
'''

def boot(name, image, elf, body, markers, normal=False, memory=128):
    directory=OUT/name; directory.mkdir()
    with tempfile.TemporaryDirectory(prefix='ks2-') as temp:
        qmp_path=Path(temp)/'qmp'; gdb_path=Path(temp)/'gdb'
        args=[os.environ.get('QEMU','qemu-system-x86_64'),'-machine','pc,accel=tcg','-cpu','qemu64',
              '-m',str(memory),'-smp','1','-vga','std','-display','none','-nic','none',
              '-cdrom',str(image),'-boot','d','-serial',f'file:{directory/"serial.log"}',
              '-monitor','none','-qmp',f'unix:{qmp_path},server=on,wait=off',
              '-gdb',f'unix:{gdb_path},server=on,wait=off','-S','-no-reboot','-no-shutdown',
              '-d','guest_errors','-D',str(directory/'qemu-diagnostic.log')]
        (directory/'command.json').write_text(json.dumps(args,indent=2))
        qmp=None
        with (directory/'qemu.stderr').open('w') as err:
            p=subprocess.Popen(args,stdout=subprocess.DEVNULL,stderr=err)
            try:
                wait_for(lambda:qmp_path.exists() and gdb_path.exists(),p,'debug sockets')
                qmp=QMP(qmp_path)
                gdb_run(directory,gdb_path,elf,body,markers)
                log=directory/'serial.log'
                wait_for(lambda:log.exists() and ('BOOT_READY' if normal else 'EXCEPTION_HALTED') in log.read_text(),p,'boot/fault completion')
                # Use QMP stop + GDB attach to read actual counters twice while
                # detached execution between samples delivers hardware IRQs.
                samples=[]
                for i in range(2):
                    time.sleep(1.1)
                    qmp.command('stop')
                    sample_script=('hbreak kernel_idle_halt\ncontinue\n' if normal else '')+'''python
print('SAMPLE',n('irq_state.ticks'),n('irq_state.timer_eoi'),n('irq_state.other_irq'),n('irq_state.fatal'),flush=True)
end
delete breakpoints'''
                    d=directory/f'sample-{i}';d.mkdir()
                    gdb_run(d,gdb_path,elf,sample_script,['SAMPLE'])
                    qmp.command('cont')
                    match=re.search(r'SAMPLE (\d+) (\d+) (\d+) (\d+)',(d/'gdb.log').read_text())
                    samples.append(tuple(map(int,match.groups())))
                (directory/'counter-samples.json').write_text(json.dumps(samples,indent=2))
                if normal:
                    assert samples[1][0]>samples[0][0]>0,samples
                    for ticks,eoi,other,fatal in samples: assert ticks==eoi and other==0 and fatal==0,samples
                    assert 'EXCEPTION' not in log.read_text()
                else:
                    assert 'BOOT_READY' not in log.read_text()
                    assert samples==[(0,0,0,1),(0,0,0,1)],samples
                registers=qmp.command('human-monitor-command',{'command-line':'info registers'})
                (directory/'registers.log').write_text(registers)
                if not normal:
                    assert 'HLT=1' in registers,'Fatal exception did not enter controlled HLT'
                    rflags=int(re.search(r'RFL=([0-9a-fA-F]+)',registers).group(1),16)
                    assert not rflags & 0x200,'Fatal exception halt has IF=1'
                pic=qmp.command('human-monitor-command',{'command-line':'info pic'})
                (directory/'pic.log').write_text(pic)
                if normal:
                    # A snapshot may coincide with IRQ0 in service; only
                    # assert stable hardware configuration, not ISR timing.
                    assert re.search(r'pic0:.*imr=fe.*irq_base=20',pic), pic
                    assert re.search(r'pic1:.*imr=ff.*irq_base=28',pic), pic
                assert qmp.command('query-status')['status']=='running'
                qmp.command('quit');p.wait(timeout=5);assert p.returncode==0
            finally:
                if qmp:qmp.close()
                if p.poll() is None:
                    p.terminate()
                    try:p.wait(timeout=5)
                    except subprocess.TimeoutExpired:p.kill();p.wait()
    print('PASS',name,flush=True)

def main():
    results=[]
    normal_image=ROOT/'build/kernseed.iso'
    original_hash=hashlib.sha256(normal_image.read_bytes()).hexdigest()
    try:
        symbols = subprocess.check_output(['nm',str(ROOT/'build/kernseed.elf')],text=True)
        assert not re.search(r'\bfault_(de|ud|gp|pf)(_|\b)',symbols),'Normal ELF contains fault injection instructions'
        boot('descriptors-timer',normal_image,ROOT/'build/kernseed.elf',NORMAL,
             ['PASS software INT','PASS all 256','PASS hardware IRQ0','PASS IRQ0 IRETQ'],normal=True)
        results.append({'name':'descriptors-timer','result':'passed'})
        for kind,vector,error,rip in [(1,0,0,'fault_de_rip'),(2,6,0,'fault_ud_rip'),
                                      (3,13,40,'fault_gp_rip'),(4,14,0,'fault_pf_rip'),(5,8,0,'fault_gp_rip')]:
            name=f'fault-{kind}-vector-{vector}'
            directory=OUT/name;directory.mkdir()
            with (directory/'build.log').open('w') as log:
                subprocess.run(['make','--no-print-directory',f'BUILD=build/fault-{kind}',f'TEST_FAULT={kind}','-j2','iso'],
                               stdout=log,stderr=subprocess.STDOUT,check=True,timeout=60)
            # boot() owns its directory; keep the build evidence beside it.
            boot(name+'-run',ROOT/f'build/fault-{kind}/kernseed.iso',ROOT/f'build/fault-{kind}/kernseed.elf',
                 fault_body(vector,error,rip,kind==5),[f'PASS real fault vector {vector}'])
            results.append({'name':name,'result':'passed'})
        assert hashlib.sha256(normal_image.read_bytes()).hexdigest()==original_hash,'Fault builds replaced normal ISO'
    except Exception:
        (OUT/'failure.log').write_text(traceback.format_exc())
        results.append({'name':locals().get('name','descriptors-timer'),'result':'failed'})
        raise
    finally:
        (OUT/'results.json').write_text(json.dumps(results,indent=2)+'\n')
        print('Phase 2 evidence:',OUT.relative_to(ROOT),flush=True)

if __name__ == "__main__":
    main()
