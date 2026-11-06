#!/usr/bin/env python3
# SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0
"""Inject actual PC keyboard events via QMP, not serial shell shortcuts."""
import json,os,re,subprocess,tempfile,time,traceback
from pathlib import Path
from boot import QMP,wait_for,ROOT
from phase2 import gdb_run
OUT=ROOT/'build/phase5-results'/time.strftime('%Y%m%dT%H%M%SZ',time.gmtime())
OUT.mkdir(parents=True,exist_ok=False)
KEYS={' ':'spc','\n':'ret','\t':'tab','\b':'backspace',"'":'apostrophe','-':'minus','=':'equal','/':'slash','.':'dot',',':'comma',';':'semicolon','[':'bracket_left',']':'bracket_right','\\':'backslash','`':'grave_accent'}
SHIFT={'"':'apostrophe','!':'1','@':'2','#':'3','$':'4','%':'5','^':'6','&':'7','*':'8','(':'9',')':'0','_':'minus','+':'equal','?':'slash',':':'semicolon','<':'comma','>':'dot','{':'bracket_left','}':'bracket_right','|':'backslash','~':'grave_accent'}
results=[]
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
            p=subprocess.Popen(args,stderr=err,stdout=subprocess.DEVNULL);q=None
            try:
                wait_for(qmpp.exists,p,'QMP socket');q=QMP(qmpp)
                wait_for(lambda:serial.exists() and 'ks> ' in serial.read_text(),p,'interactive shell')
                def text(s):
                    for c in s:
                        shift=c.isupper() or c in SHIFT
                        code=SHIFT.get(c,KEYS.get(c,c.lower()))
                        keys=([{'type':'qcode','data':'shift'}] if shift else [])+[{'type':'qcode','data':code}]
                        q.command('send-key',{'keys':keys,'hold-time':10});time.sleep(.03)
                def command(s,expected=None):
                    at=len(serial.read_text());text(s+'\n')
                    wait_for(lambda:'ks> ' in serial.read_text()[at:],p,'prompt after '+s[:24])
                    response=serial.read_text()[at:]
                    (d/'transcript.log').open('a').write(response)
                    if expected is not None:assert expected in response,(s,response)
                    return response
                response=command('help','echo - Echo parsed arguments')
                for name in ('help','clear','about','meminfo','tasks','uptime','reboot','echo'):assert re.search(r'\b'+name+r' - ',response),name
                command('help echo','echo [text ...]')
                command('echo "alpha beta" tail','alpha beta tail\n')
                command("echo '' tail",'\n tail\n')
                command('echo Mixed_CASE','\nMixed_CASE\n')
                command('abouX\bt','Kernseed OS Phase 5')
                command('  \t  ')
                command('not-a-command','error: unknown command')
                command('about extra','usage: about')
                command('help missing','error: no such command')
                command('echo a b c d e f g h','error: too many arguments')
                command('echo "unclosed','error: malformed quote or escape')
                command('a'*140,'error: line too long; discarded')
                command('echo recovered','\nrecovered\n')
                q.command('input-send-event',{'events':[{'type':'rel','data':{'axis':'x','value':20}},{'type':'rel','data':{'axis':'y','value':10}}]})
                command('echo mouse-safe','\nmouse-safe\n')
                m=command('meminfo','pages: usable=')
                stats=tuple(map(int,re.search(r'usable=(\d+) free=(\d+) allocated=(\d+) tables=(\d+) mapped=(\d+)',m).groups()))
                assert stats[0]==sum(stats[1:4]) and stats[2]==stats[4]==16,stats
                t=command('tasks','shell RUNNING')
                for name in ('idle','A','B','C','shell'):assert name in t
                u1=command('uptime','uptime: ticks=');time.sleep(.2);u2=command('uptime','uptime: ticks=')
                ticks=lambda s:int(re.search(r'uptime: ticks=(\d+)',s).group(1))
                assert ticks(u2)>ticks(u1)>0
                command('clear','\x1b[2J\x1b[H')
                command('echo screencheck','\nscreencheck\n')
                q.command('screendump',{'filename':str(d/'screen.ppm')})
                check_screen(d/'screen.ppm')
                q.command('stop');gd=d/'gdb';gd.mkdir()
                gdb_run(gd,gdbp,ROOT/'build/kernseed.elf','''hbreak *scheduler_interrupt
continue
python
check('shell_state.ready == 1 && shell_state.commands >= 18 && shell_state.overlong == 1')
check('keyboard_state.ready == 1 && keyboard_state.irqs > 0 && keyboard_state.bytes > 0')
check('keyboard_state.dropped == 0 && keyboard_state.errors == 0')
check('irq_state.other_irq == keyboard_state.irqs')
check('sched_state.preemptions > 0 && register_errors == 0 && sched_state.errors == 0')
check('memory_state.allocated == 16 && memory_state.mapped == 16')
check('irq_state.pic_master_mask == 0xfc && irq_state.pic_slave_mask == 0xff')
print('PASS real IRQ1 input, bounded shell, preemption, memory and registry')
end
p/x keyboard_state
p/x shell_state
p/x memory_state
p/x sched_state
delete breakpoints
''',['PASS real IRQ1'])
                q.command('cont')
                # Force reader backpressure through its real sleep request;
                # IRQ1 continues to fill the actual bounded keyboard ring.
                gd=d/'backpressure';gd.mkdir()
                gdb_run(gd,gdbp,ROOT/'build/kernseed.elf','''hbreak *scheduler_interrupt if sched_state.current == 6 && ((struct interrupt_frame *)$rdi)->vector == 129
continue
python
check('request == 2')
print('PASS shell sleeping for FIFO backpressure test')
end
set variable sleep_ticks=1000000
delete breakpoints
''',['PASS shell sleeping'])
                text('a'*280)
                q.command('stop');gd=d/'overflow';gd.mkdir()
                gdb_run(gd,gdbp,ROOT/'build/kernseed.elf','''python
check('tasks[6].status == BLOCKED && keyboard_state.dropped > 0')
check('keyboard_state.errors == 0 && sched_state.errors == 0')
print('PASS actual keyboard FIFO overflow detected')
end
set variable tasks[6].wake_tick=0
''',['PASS actual keyboard FIFO'])
                q.command('cont')
                wait_for(lambda:'error: input overflow; press Enter to discard' in serial.read_text(),p,'overflow discard notification')
                command('','error: line too long; discarded')
                command('echo fifo-recovered','\nfifo-recovered\n')
                before=serial.read_text().count('Kernseed Shell: type help')
                text('reboot\n')
                wait_for(lambda:serial.read_text().count('Kernseed Shell: type help')>before,p,'hardware reboot and new shell',seconds=40)
                command('about','Kernseed OS Phase 5')
                q.command('quit');p.wait(timeout=5);assert p.returncode==0
            finally:
                if q:q.close()
                if p.poll() is None:p.terminate();p.wait(timeout=5)
    print('PASS shell keyboard/commands/reboot',memory,flush=True)
try:
    for memory in (128,256):run(memory);results.append({'memory':memory,'result':'passed'})
except Exception:
    (OUT/'failure.log').write_text(traceback.format_exc());raise
finally:
    (OUT/'results.json').write_text(json.dumps(results,indent=2))
    print('Phase 5 evidence:',OUT,flush=True)
