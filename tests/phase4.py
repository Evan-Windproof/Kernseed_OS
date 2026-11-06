#!/usr/bin/env python3
# SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0
import json,os,subprocess,tempfile,time,traceback
from pathlib import Path
from boot import QMP,wait_for,ROOT
import phase2 as q
OUT=ROOT/'build/phase4-results'/time.strftime('%Y%m%dT%H%M%SZ',time.gmtime())
OUT.mkdir(parents=True,exist_ok=False)
START='''
hbreak *counter_task_a
continue
python
check('sched_state.current == 1 && tasks[1].status == RUNNING')
check('sched_state.rollback_passed == 1 && sched_state.create_failures == 1')
check('irq_state.ticks > 0 && irq_state.ticks == irq_state.timer_eoi')
check('((unsigned long)$rsp & 15) == 8')
print('PASS first real IRQ0 stack switch')
end
delete breakpoints
hbreak *counter_task_b
continue
python
check('demo_a > 0 && sched_state.preemptions > 0')
check('tasks[1].preemptions > 0 && tasks[1].status == READY')
check('tasks[1].context->rsp >= tasks[1].low && tasks[1].context->rsp < tasks[1].high')
check('tasks[1].physical != tasks[2].physical && tasks[1].low != tasks[2].low')
for i,r in enumerate(['rax','rbx','rcx','rdx','rbp','rdi','rsi','r8','r9','r10','r11','r12','r13','r14','r15']):
    check('tasks[1].context->'+r+' == '+str(0x4b53000000000201+i))
check('(tasks[1].context->rflags & 0x400) != 0')
print('PASS preempted A without yield; all GPRs and independent stack saved')
end
p/x sched_state
delete breakpoints
'''
def run(memory,seconds=5):
    d=OUT/f'{memory}M';d.mkdir()
    with tempfile.TemporaryDirectory(prefix='ks4-') as tmp:
        sock=Path(tmp)/'gdb';qmpp=Path(tmp)/'qmp'
        args=[os.environ.get('QEMU','qemu-system-x86_64'),'-machine','pc,accel=tcg','-cpu','qemu64','-m',str(memory),'-smp','1','-vga','std','-display','none','-nic','none','-cdrom',str(ROOT/'build/kernseed.iso'),'-boot','d','-serial',f'file:{d/"serial.log"}','-monitor','none','-qmp',f'unix:{qmpp},server=on,wait=off','-gdb',f'unix:{sock},server=on,wait=off','-S','-no-reboot','-no-shutdown']
        (d/'command.json').write_text(json.dumps(args,indent=2))
        with (d/'qemu.stderr').open('w') as err:
            p=subprocess.Popen(args,stderr=err,stdout=subprocess.DEVNULL);monitor=None
            try:
                wait_for(lambda:sock.exists() and qmpp.exists(),p,'sockets');monitor=QMP(qmpp)
                q.gdb_run(d,sock,ROOT/'build/kernseed.elf',START,['PASS first real','PASS preempted'])
                time.sleep(seconds)
                monitor.command('stop')
                sample=d/'stress';sample.mkdir()
                q.gdb_run(sample,sock,ROOT/'build/kernseed.elf','''hbreak *scheduler_interrupt
continue
python
check('demo_a > 0 && demo_b > 0 && register_errors == 0 && sched_state.errors == 0')
check('sched_state.preemptions > 20 && sched_state.records > 3')
check('tasks[1].yields == 0 && tasks[2].yields == 0')
check('sched_state.yields >= 3 && sched_state.exits == 2 && sched_state.reaped == 2')
check('tasks[4].reclaimed == 1 && tasks[5].reclaimed == 1')
check('memory_state.allocated == 12 && memory_state.mapped == 12')
check('memory_state.free + memory_state.tables + memory_state.allocated == memory_state.usable')
if sum(n(f'tasks[{i}].switches') for i in range(8)) != n('sched_state.switches'):raise gdb.GdbError('Switch statistics disagree')
if sum(n(f'tasks[{i}].preemptions') for i in range(8)) != n('sched_state.preemptions'):raise gdb.GdbError('Preemption statistics disagree')
if sum(n(f'tasks[{i}].status == RUNNING') for i in range(8)) != 1:raise gdb.GdbError('RUNNING state not unique')
rt=sum(n(f'tasks[{i}].runtime_ticks') for i in range(8))
if rt + n('((struct interrupt_frame *)$rdi)->vector == 32') != n('irq_state.ticks'):raise gdb.GdbError('Runtime tick accounting disagrees')
check('irq_state.ticks == irq_state.timer_eoi && irq_state.entry_if_errors == 0')
check('sched_state.observed_a > 0 && sched_state.observed_b > 0')
for i in (1,2,3):
    check('tasks['+str(i)+'].switches > 0')
print('PASS sustained preemption, yield/exit/reap, counters, stack/register sentinels')
end
p/x sched_state
p/x tasks
p/x memory_state
set variable demo_stop=1
''',['PASS sustained'])
                monitor.command('cont');time.sleep(1)
                monitor.command('stop');sample=d/'idle';sample.mkdir()
                q.gdb_run(sample,sock,ROOT/'build/kernseed.elf','''hbreak kernel_idle_halt if sched_state.reaped == 27
continue
python
check('sched_state.current == 0 && sched_state.exits == 27 && sched_state.reaped == 27')
check('memory_state.allocated == 0 && memory_state.mapped == 0')
check('memory_state.free + memory_state.tables == memory_state.usable')
if not flags() & 0x200:raise gdb.GdbError('Idle disabled IRQs')
check('sched_state.recycle_passed == 16 && sched_state.capacity_passed == 1 && sched_state.create_failures == 2')
check('sched_state.idle_ticks > 0 && register_errors == 0 && sched_state.errors == 0')
for i in (1,2,3,4,5):check('tasks['+str(i)+'].status == TERMINATED && tasks['+str(i)+'].reclaimed == 1')
print('PASS empty ready queue, real idle and complete data-stack reclamation')
end
p/x sched_state
p/x memory_state
delete breakpoints
''',['PASS empty ready'])
                monitor.command('quit');p.wait(timeout=5);assert p.returncode==0
            finally:
                if monitor:monitor.close()
                if p.poll() is None:p.terminate();p.wait(timeout=5)
    print('PASS scheduler',memory,flush=True)
results=[]
try:
    duration=int(os.environ.get('KERNSEED_STRESS_SECONDS','120'))
    assert duration>=5,'Stress interval must be at least 5 seconds'
    for memory,seconds in [(128,5),(256,duration),(512,5)]:
        run(memory,seconds);results.append({'memory':memory,'seconds':seconds,'result':'passed'})
    q.OUT=OUT
    for kind in (9,10):
        d=OUT/f'guard-{kind}';d.mkdir()
        with (d/'build.log').open('w') as log:
            subprocess.run(['make',f'BUILD=build/fault-{kind}',f'TEST_FAULT={kind}','SCHEDULER=1','CONSOLE_SHELL=0','-j2','iso'],stdout=log,stderr=subprocess.STDOUT,check=True,timeout=60)
        guard='tasks[1].low-4096' if kind==9 else 'tasks[1].high'
        body=f"""
hbreak cpu_halt
continue
python
check('irq_state.fatal == 1 && last_exception.vector == 14 && last_exception.error == 2')
check('irq_state.exception_cr2 == {guard}')
check('last_exception.rsp >= tasks[1].low && last_exception.rsp < tasks[1].high')
check('sched_state.current == 1 && sched_state.errors == 0')
print('PASS actual task stack guard fault {kind}')
end
p/x last_exception
p/x sched_state
delete breakpoints
"""
        q.boot(f'guard-{kind}-run',ROOT/f'build/fault-{kind}/kernseed.iso',ROOT/f'build/fault-{kind}/kernseed.elf',body,['PASS actual task stack guard'],fault_after_timer=True)
        results.append({'guard':kind,'result':'passed'})
except Exception:
    (OUT/'failure.log').write_text(traceback.format_exc());raise
finally:
    (OUT/'results.json').write_text(json.dumps(results,indent=2))
    print('Phase 4 evidence:',OUT,flush=True)
