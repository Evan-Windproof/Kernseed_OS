#!/usr/bin/env python3
# SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0
"""Final BIOS/optional OVMF cold boots and existing real Ring3 integration tests.
Measurements use host monotonic time, include emulation/polling overhead, and
are not isolated kernel operation latencies. No firmware is bundled here.
"""
import json, os, shutil, subprocess, tempfile, time, traceback
from pathlib import Path
from boot import ROOT, QMP, wait_for
from phase2 import gdb_run, GDB_HELPERS
OUT=ROOT/'build/phase7-results'/time.strftime('%Y%m%dT%H%M%SZ',time.gmtime())
OUT.mkdir(parents=True,exist_ok=False)
results=[]

def boot(name,uefi=False,panic=False):
    directory=OUT/name;directory.mkdir()
    with tempfile.TemporaryDirectory(prefix='ks7-') as tmp:
        tmp=Path(tmp);socket=tmp/'gdb';qmpp=tmp/'qmp';serial=directory/'serial.log'
        args=[os.environ.get('QEMU','qemu-system-x86_64'),'-machine','pc,accel=tcg','-cpu','qemu64','-m','256','-smp','1','-vga','std','-display','none','-nic','none','-cdrom',str(ROOT/'build/kernseed.iso'),'-boot','d','-serial',f'file:{serial}','-monitor','none','-qmp',f'unix:{qmpp},server=on,wait=off','-gdb',f'unix:{socket},server=on,wait=off','-no-reboot','-no-shutdown']
        if uefi:
            code=Path(os.environ['KERNSEED_OVMF_CODE']).resolve()
            template=Path(os.environ['KERNSEED_OVMF_VARS']).resolve()
            shutil.copyfile(template,tmp/'vars.fd')
            args+=['-drive',f'if=pflash,format=raw,unit=0,readonly=on,file={code}','-drive',f'if=pflash,format=raw,unit=1,file={tmp/"vars.fd"}']
        (directory/'command.json').write_text(json.dumps(args,indent=2))
        with (directory/'qemu.stderr').open('w') as err:
            started=time.monotonic();process=subprocess.Popen(args,stderr=err,stdout=subprocess.DEVNULL);q=None
            try:
                wait_for(qmpp.exists,process,'QMP');q=QMP(qmpp)
                wait_for(lambda:serial.exists() and 'ks> ' in serial.read_text(),process,'Shell prompt',seconds=40)
                elapsed=time.monotonic()-started
                assert 'PANIC' not in serial.read_text()
                q.command('stop')
                gdb_run(directory,socket,ROOT/'build/kernseed.elf','''hbreak *scheduler_interrupt if sched_state.reaped == 2
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
''',['PASS actual kernel'])
                if panic:
                    q.command('stop');pd=directory/'panic';pd.mkdir()
                    gdb_run(pd,socket,ROOT/'build/kernseed.elf','''set *(unsigned long long *)tasks[1].low = 0
hbreak cpu_halt
continue
python
check('sched_state.errors == 1')
if flags() & 0x200:raise gdb.GdbError('Panic left interrupts enabled')
print('PASS isolated corrupted-canary controlled scheduler panic')
end
delete breakpoints
''',['PASS isolated'])
                    wait_for(lambda:'PANIC: scheduler invariant failed' in serial.read_text(),process,'panic diagnostic')
                q.command('quit');process.wait(timeout=5);assert process.returncode==0
                results.append({'name':name,'result':'passed','boot_to_prompt_seconds':elapsed})
            finally:
                if q:q.close()
                if process.poll() is None:process.terminate();process.wait(timeout=5)
    print('PASS cold boot',name,flush=True)

try:
    for index in range(5):boot(f'bios-{index+1}')
    boot('bios-panic-diagnostic',panic=True)
    if os.environ.get('KERNSEED_OVMF_CODE') and os.environ.get('KERNSEED_OVMF_VARS'):
        boot('uefi-ovmf',True)
    else:results.append({'name':'uefi-ovmf','result':'not executed','reason':'Set KERNSEED_OVMF_CODE and KERNSEED_OVMF_VARS to firmware files'})
    env=dict(os.environ,KERNSEED_TEST_MEMORY='128,256,512,1024')
    with (OUT/'apps-matrix.log').open('w') as log:
        subprocess.run(['python3','tests/phase6.py'],cwd=ROOT,env=env,stdout=log,stderr=subprocess.STDOUT,check=True)
    results.append({'name':'Ring3 four-memory matrix','result':'passed'})
except Exception:
    (OUT/'failure.log').write_text(traceback.format_exc());raise
finally:
    (OUT/'results.json').write_text(json.dumps(results,indent=2)+'\n')
    print('Phase 7 evidence:',OUT,flush=True)
