#!/usr/bin/env python3
# SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0
"""Inject actual PC keyboard events via QMP, not serial shell shortcuts."""
import json,os,re,subprocess,tempfile,time,traceback
from pathlib import Path
from boot import QMP,wait_for,ROOT
from phase2 import gdb_run, GDB_HELPERS
OUT=ROOT/'build/phase6-results'/time.strftime('%Y%m%dT%H%M%SZ',time.gmtime())
OUT.mkdir(parents=True,exist_ok=False)
KEYS={' ':'spc','\n':'ret','\t':'tab','\b':'backspace',"'":'apostrophe','-':'minus','=':'equal','/':'slash','.':'dot',',':'comma',';':'semicolon','[':'bracket_left',']':'bracket_right','\\':'backslash','`':'grave_accent'}
SHIFT={'"':'apostrophe','!':'1','@':'2','#':'3','$':'4','%':'5','^':'6','&':'7','*':'8','(':'9',')':'0','_':'minus','+':'equal','?':'slash',':':'semicolon','<':'comma','>':'dot','{':'bracket_left','}':'bracket_right','|':'backslash','~':'grave_accent'}
results=[]
measurements=[]
def check_screen(path):
    with path.open('rb') as stream:
        assert stream.readline().strip()==b'P6'
        width,height=map(int,stream.readline().split())
        assert (width,height)==(800,600) and stream.readline().strip()==b'255'
        pixels=stream.read()
    assert len(pixels)==width*height*3
    for row,shape in [(0,[17,18,20,24,20,18,17]),(1,[15,16,16,14,1,1,30])]:
        for y in range(18):
            for x in range(12):
                lit=x<10 and y<14 and shape[y//2]&(1<<(4-x//2))
                at=((row*18+y)*width+x)*3
                assert pixels[at:at+3]==(b'\xdd\xee\xff' if lit else b'\0\0\0')
    assert not any(pixels[3*18*width*3:]),'clear left old screen data'
def run(memory):
    d=OUT/f'{memory}M';d.mkdir();serial=d/'serial.log'
    with tempfile.TemporaryDirectory(prefix='ks5-') as temp:
        qmpp=Path(temp)/'qmp';gdbp=Path(temp)/'gdb'
        args=[os.environ.get('QEMU','qemu-system-x86_64'),'-machine','pc,accel=tcg','-cpu','qemu64','-m',str(memory),'-smp','1','-vga','std','-display','none','-nic','none','-cdrom',str(ROOT/'build/kernseed.iso'),'-boot','d','-serial',f'file:{serial}','-monitor','none','-qmp',f'unix:{qmpp},server=on,wait=off','-gdb',f'unix:{gdbp},server=on,wait=off','-no-shutdown']
        (d/'command.json').write_text(json.dumps(args,indent=2))
        with (d/'qemu.stderr').open('w') as err:
            boot_started=started=time.monotonic()
            p=subprocess.Popen(args,stderr=err,stdout=subprocess.DEVNULL);q=None
            try:
                wait_for(qmpp.exists,p,'QMP socket');q=QMP(qmpp)
                wait_for(lambda:serial.exists() and 'ks> ' in serial.read_text(),p,'interactive shell')
                measurements.append({'memory':memory,'operation':'boot-to-prompt','seconds':time.monotonic()-started})
                def text(s):
                    for c in s:
                        shift=c.isupper() or c in SHIFT
                        code=SHIFT.get(c,KEYS.get(c,c.lower()))
                        keys=([{'type':'qcode','data':'shift'}] if shift else [])+[{'type':'qcode','data':code}]
                        q.command('send-key',{'keys':keys,'hold-time':10});time.sleep(.03)
                def command(s,expected=None):
                    started=time.monotonic()
                    at=len(serial.read_text());text(s+'\n')
                    wait_for(lambda:'ks> ' in serial.read_text()[at:],p,'prompt after '+s[:24])
                    response=serial.read_text()[at:]
                    (d/'transcript.log').open('a').write(response)
                    if expected is not None:assert expected in response,(s,response)
                    measurements.append({'memory':memory,'operation':s,'seconds':time.monotonic()-started})
                    return response
                command('apps','hello counter probe privileged kernelread')
                command('run missing','error: unavailable application')
                # Existing allocator fault-injection seam: fail task-stack mapping
                # after user pages have been mapped. Verify both nested rollbacks.
                rollback=d/'rollback';rollback.mkdir()
                script=rollback/'inspect.gdb'
                script.write_text(f"""set pagination off
set confirm off
set language c
file {ROOT/'build/kernseed.elf'}
target remote {gdbp}
python
{GDB_HELPERS}
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
""")
                with (rollback/'gdb.log').open('w') as log:
                    debugger=subprocess.Popen([os.environ.get('GDB','gdb'),'-nx','-batch','-x',str(script)],stdout=log,stderr=subprocess.STDOUT)
                    time.sleep(.3)
                    command('run hello','error: unavailable application')
                    assert debugger.wait(timeout=20)==0
                assert 'PASS user creation failure' in (rollback/'gdb.log').read_text()
                # Attach before launching, so both CPL3 execution and CPL0
                # syscall entry are observed on the actual CPU, not just text.
                trace=d/'trace.gdb'
                trace.write_text(f"""set pagination off
set confirm off
set language c
file {ROOT/'build/kernseed.elf'}
target remote {gdbp}
python
{GDB_HELPERS}
end
hbreak *0xffffc00001000000
continue
python
check('$cs == 0x2b && $ss == 0x33')
check('(unsigned long)$rsp >= 0xffffc00001100000 && (unsigned long)$rsp < 0xffffc00001101000')
print('PASS actual CPU CPL3 execution')
end
info registers cs ss rsp cr3
x/4i $pc
delete breakpoints
hbreak *syscall_dispatch
continue
python
entry_check()
check('$cs == 8 && ((struct interrupt_frame *)$rdi)->cs == 0x2b')
check('(unsigned long)$rsp >= tasks[sched_state.current].low && (unsigned long)$rsp < tasks[sched_state.current].high')
print('PASS actual CPL3 to CPL0 syscall and kernel stack')
end
info registers cs ss rsp
delete breakpoints
detach
quit
""")
                with (d/'trace.log').open('w') as log:
                    debugger=subprocess.Popen([os.environ.get('GDB','gdb'),'-nx','-batch','-x',str(trace)],stdout=log,stderr=subprocess.STDOUT)
                    time.sleep(.3)
                    command('run hello','Hello from Kernseed userspace!')
                    assert debugger.wait(timeout=20)==0
                assert 'PASS actual CPL3 to CPL0' in (d/'trace.log').read_text()

                command('run counter','Counter ticks: ')
                command('run probe','PROBE PASSED')
                command('run badflags','FLAGS SANITIZED')
                command('run badstack','User fault: task terminated')
                for app,vector,error in [('privileged',13,0),('kernelread',14,5),('kernelwrite',14,7)]:
                    command('run '+app,'User fault: task terminated')
                    q.command('stop');fd=d/app;fd.mkdir()
                    gdb_run(fd,gdbp,ROOT/'build/kernseed.elf',f'''python
check('user_state.vector == {vector} && user_state.error == {error} && user_state.active == 0')
check('memory_state.allocated == 16 && memory_state.mapped == 16 && irq_state.fatal == 0')
check('user_state.fault_rip >= 0xffffc00001000000 && user_state.fault_rip < 0xffffc00001001000')
check('user_state.last_rsp >= 0xffffc00001100000 && user_state.last_rsp < 0xffffc00001101000')
print('PASS user fault {app}: actual vector/error and safe cleanup')
end
p/x user_state
''',['PASS user fault'])
                    q.command('cont')
                command('run hello','Hello from Kernseed userspace!')
                command('meminfo','allocated=16')
                q.command('stop');gd=d/'gdb';gd.mkdir()
                gdb_run(gd,gdbp,ROOT/'build/kernseed.elf',"""python
check('user_state.preemptions > 0')
check('user_state.calls > 0 && user_state.last_cs == 0x2b')
check('user_state.active == 0 && user_state.exits == 9 && user_state.faults == 4')
check('user_state.vector == 14 && user_state.error & 4')
check('user_state.invalid == 4 && irq_state.fatal == 0 && sched_state.errors == 0')
check('memory_state.allocated == 16 && memory_state.mapped == 16')
print('PASS actual Ring3 calls, invalid pointers/numbers, faults and reclamation')
end
p/x user_state
p/x memory_state
""",['PASS actual Ring3'])
                q.command('cont')
                repeats=int(os.environ.get('KERNSEED_APP_REPEATS','0'))
                assert 0<=repeats<=1000
                for cycle in range(repeats):
                    command('run hello','Hello from Kernseed userspace!')
                    command('run counter','Counter ticks: ')
                    command('echo "repeat ok"','repeat ok')
                if repeats:
                    q.command('stop');rd=d/'repeated';rd.mkdir()
                    gdb_run(rd,gdbp,ROOT/'build/kernseed.elf',f"""python
check('user_state.exits == {9+2*repeats} && user_state.active == 0 && user_state.faults == 4')
check('memory_state.allocated == 16 && memory_state.mapped == 16')
check('sched_state.errors == 0 && irq_state.fatal == 0 && register_errors == 0')
print('PASS repeated application lifecycle and resource consistency')
end
p/x user_state
p/x sched_state
p/x memory_state
""",['PASS repeated'])
                    q.command('cont')
                measurements.append({'memory':memory,'operation':'whole-scenario','seconds':time.monotonic()-boot_started})
                q.command('quit');p.wait(timeout=5);assert p.returncode==0
            finally:
                if q:q.close()
                if p.poll() is None:p.terminate();p.wait(timeout=5)
    print('PASS userspace apps/syscalls/faults',memory,flush=True)
try:
    for memory in map(int,os.environ.get("KERNSEED_TEST_MEMORY","128,256").split(",")):run(memory);results.append({'memory':memory,'result':'passed'})
except Exception:
    (OUT/'failure.log').write_text(traceback.format_exc());raise
finally:
    (OUT/'measurements.json').write_text(json.dumps(measurements,indent=2))
    (OUT/'results.json').write_text(json.dumps(results,indent=2))
    print('Phase 6 evidence:',OUT,flush=True)
