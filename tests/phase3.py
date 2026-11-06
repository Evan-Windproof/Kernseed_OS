#!/usr/bin/env python3
# SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0
import json, subprocess, time, traceback
from pathlib import Path
import phase2 as q
q.OUT=q.ROOT/'build/phase3-results'/time.strftime('%Y%m%dT%H%M%SZ',time.gmtime())
q.OUT.mkdir(parents=True,exist_ok=False)
body='''
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
'''
results=[]
try:
    for mib in (128,256,512,1024):
        name=f'memory-{mib}M'
        q.boot(name,q.ROOT/'build/kernseed.iso',q.ROOT/'build/kernseed.elf',body,
               ['PASS physical allocator'],normal=True,memory=mib)
        results.append({'name':name,'result':'passed'})
    for fault,error in ((6,0),(7,3),(8,17)):
        name=f'page-fault-{fault}'
        d=q.OUT/name;d.mkdir()
        with (d/'build.log').open('w') as log:
            subprocess.run(['make',f'BUILD=build/fault-{fault}',f'TEST_FAULT={fault}','-j2','iso'],stdout=log,stderr=subprocess.STDOUT,check=True,timeout=60)
        expected_rip='0xffffc00000000000' if fault==8 else '(unsigned long)&memory_fault_site'
        script=f'''
hbreak cpu_halt
continue
python
check('memory_state.tests == 1')
check('last_exception.vector == 14 && last_exception.error == {error}')
check('irq_state.exception_cr2 == 0xffffc00000000000')
check('last_exception.rip == {expected_rip}')
check('irq_state.fatal == 1 && irq_state.nested == 0')
print('PASS hardware page permission fault {fault}')
end
p/x last_exception
p/x memory_state
delete breakpoints
'''
        q.boot(name+'-run',q.ROOT/f'build/fault-{fault}/kernseed.iso',q.ROOT/f'build/fault-{fault}/kernseed.elf',script,['PASS hardware page permission'])
        results.append({'name':name,'result':'passed'})
except Exception:
    (q.OUT/'failure.log').write_text(traceback.format_exc());raise
finally:
    (q.OUT/'results.json').write_text(json.dumps(results,indent=2)+'\n')
    print('Phase 3 evidence:',q.OUT,flush=True)
