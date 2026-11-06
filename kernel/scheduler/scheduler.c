/* SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 */
#include <kernseed/scheduler.h>
#include <kernseed/memory.h>
#include <kernseed/runtime.h>
#include <kernseed/shell.h>
#define CANARY 0x6b65726e7374616bULL
struct task tasks[TASK_LIMIT];
volatile struct scheduler_stats sched_state;
volatile uint64_t demo_a,demo_b,demo_stop,register_errors;
static uint64_t next_id=1,request,sleep_ticks;
static int fail_after=-1;
extern unsigned char boot_stack_bottom[],boot_stack_top[];
extern void counter_task_a(void),counter_task_b(void);
static uint64_t irq_lock(void) {uint64_t f;__asm__ volatile("pushfq; popq %0; cli":"=r"(f)::"memory");return f;}
static void irq_unlock(uint64_t f){__asm__ volatile("pushq %0; popfq"::"r"(f):"memory","cc");}
static _Noreturn void bad(void){sched_state.errors++;cpu_halt();}
static _Noreturn void trampoline(void){
    tasks[sched_state.current].entry();
    task_exit();
}
int task_create(const char *name,void (*entry)(void)) {
    uint64_t flags=irq_lock();int slot=-1;
    if(!entry||!name)goto failed;
    for(int i=1;i<TASK_LIMIT;i++)if(tasks[i].status==UNUSED ||
        (tasks[i].status==TERMINATED&&tasks[i].reclaimed)) {slot=i;break;}
    if(slot<0)goto failed;
    uint64_t p=pmm_alloc(STACK_PAGES),mapped=0;
    if(!p)goto failed;
    uint64_t low=TASK_STACK_BASE+(uint64_t)slot*TASK_STRIDE+4096;
    for(;mapped<STACK_PAGES;mapped++) {
        if((int)mapped==fail_after||!vmm_map(low+mapped*4096,p+mapped*4096,VM_WRITE|VM_NX))goto rollback;
        memset((void *)(uintptr_t)(low+mapped*4096),0,4096);
    }
    uint64_t high=low+STACK_PAGES*4096;
    *(uint64_t *)(uintptr_t)low=CANARY;
    struct interrupt_frame *frame=(void *)(uintptr_t)(high-8-sizeof(*frame));
    memset(frame,0,sizeof(*frame));
    frame->rip=(uintptr_t)trampoline;frame->cs=8;frame->ss=0x10;
    frame->rflags=0x202;frame->rsp=high-8;
    tasks[slot]=(struct task){.id=next_id++,.name=name,.status=READY,.physical=p,
        .low=low,.high=high,.context=frame,.entry=entry};
    irq_unlock(flags);return slot;
rollback:
    for(uint64_t j=0;j<mapped;j++)if(!vmm_unmap(low+j*4096))bad();
    for(uint64_t j=0;j<STACK_PAGES;j++)if(!pmm_free(p+j*4096))bad();
failed:
    sched_state.create_failures++;irq_unlock(flags);return -1;
}
static void reap(void){
    for(unsigned i=1;i<TASK_LIMIT;i++) {
        struct task *t=&tasks[i];
        if(i==sched_state.current||t->status!=TERMINATED||t->reclaimed)continue;
        for(unsigned j=0;j<STACK_PAGES;j++)if(!vmm_unmap(t->low+j*4096)||!pmm_free(t->physical+j*4096))bad();
        t->physical=0;t->context=0;t->reclaimed=1;sched_state.reaped++;
    }
}
static void trap(uint64_t action,uint64_t ticks){
    uint64_t f=irq_lock();request=action;sleep_ticks=ticks;
    __asm__ volatile("int $0x81":::"memory","cc");
    irq_unlock(f);
}
void task_yield(void){trap(1,0);}
void task_sleep(uint64_t ticks){trap(2,ticks?ticks:1);}
_Noreturn void task_exit(void){trap(3,0);bad();}
static void short_task(void);
static void recorder(void){
    while(!demo_stop) {
        sched_state.observed_a=demo_a;sched_state.observed_b=demo_b;
        sched_state.observed_tick=irq_state.ticks;sched_state.records++;
        task_sleep(10);
    }
    if(KERNSEED_SHELL)return; /* Legacy lifecycle fixture runs without the shell task. */
    /* Explicit test stop is observed by all demo tasks; wait until A/B are reaped. */
    task_sleep(30);
    if(memory_state.allocated!=4||memory_state.mapped!=4)bad();
    for(unsigned cycle=0;cycle<16;cycle++) {
        if(task_create("recycled",short_task)<0)bad();
        task_yield();task_sleep(1);
        if(memory_state.allocated!=4||memory_state.mapped!=4)bad();
        sched_state.recycle_passed++;
    }
    for(unsigned i=0;i<6;i++)if(task_create("capacity",short_task)<0)bad();
    uint64_t before=memory_state.allocated,mapped=memory_state.mapped;
    if(task_create("full",short_task)!=-1||before!=memory_state.allocated||mapped!=memory_state.mapped)bad();
    sched_state.capacity_passed=1;
    task_yield();task_sleep(30);
    if(memory_state.allocated!=4||memory_state.mapped!=4)bad();
}
static void yielding(void){for(unsigned i=0;i<3;i++)task_yield();}
static void guard_write(void){*(volatile uint64_t *)(uintptr_t)(KERNSEED_TEST_FAULT==9?tasks[sched_state.current].low-4096:tasks[sched_state.current].high)=1;}
static void short_task(void){} /* Real return exercises trampoline -> exit. */
void scheduler_start(void){
    if(!KERNSEED_SCHEDULER)return;
    uint64_t f=irq_lock();
    tasks[0]=(struct task){.id=0,.name="idle",.status=RUNNING,
        .low=(uintptr_t)boot_stack_bottom,.high=(uintptr_t)boot_stack_top};
    /* Force failure after two actual mappings; verify owned data and refs roll back. */
    uint64_t allocated=memory_state.allocated,mapped=memory_state.mapped;
    fail_after=2;
    if(task_create("rollback",short_task)!=-1||allocated!=memory_state.allocated||mapped!=memory_state.mapped)bad();
    fail_after=-1;sched_state.rollback_passed=1;
    if(task_create("A",KERNSEED_TEST_FAULT>=9?guard_write:counter_task_a)!=1||task_create("B",counter_task_b)!=2||
       task_create("C",recorder)!=3||task_create("yield-exit",yielding)<0||task_create("return-exit",short_task)<0)bad();
    if(KERNSEED_SHELL&&task_create("shell",shell_task)<0)bad();
    sched_state.slice=5;sched_state.active=1;
    irq_unlock(f);
}
struct interrupt_frame *scheduler_interrupt(struct interrupt_frame *frame){
    if(!sched_state.active)return frame;
    if(frame->vector!=32&&frame->vector!=129)return frame;
    struct task *old=&tasks[sched_state.current];
    if((uintptr_t)frame<old->low||(uintptr_t)frame+sizeof(*frame)>old->high||
       frame->rsp<old->low+16||frame->rsp>old->high)bad();
    if(sched_state.current&&*(uint64_t *)(uintptr_t)old->low!=CANARY)bad();
    old->context=frame;reap();
    for(unsigned i=1;i<TASK_LIMIT;i++)if(tasks[i].status==BLOCKED&&irq_state.ticks>=tasks[i].wake_tick)tasks[i].status=READY;
    bool reschedule=false;
    if(frame->vector==32){
        old->runtime_ticks++;
        if(!sched_state.current)sched_state.idle_ticks++;
        if(sched_state.slice)sched_state.slice--;
        reschedule=!sched_state.current||!sched_state.slice;
    }else{
        uint64_t action=request;request=0;
        if(!sched_state.current)bad();
        if(action==1){old->yields++;sched_state.yields++;}
        else if(action==2){
            if(sleep_ticks>UINT64_MAX-irq_state.ticks)bad();
            old->status=BLOCKED;old->wake_tick=irq_state.ticks+sleep_ticks;
        }else if(action==3){old->status=TERMINATED;sched_state.exits++;}
        else bad();
        reschedule=true;
    }
    if(!reschedule)return frame;
    if(old->status==RUNNING)old->status=READY;
    unsigned chosen=0;
    for(unsigned n=1;n<=TASK_LIMIT;n++){
        unsigned i=(sched_state.current+n)%TASK_LIMIT;
        if(i&&tasks[i].status==READY){chosen=i;break;}
    }
    tasks[chosen].status=RUNNING;sched_state.slice=5;
    if(chosen!=sched_state.current){
        if(frame->vector==32&&sched_state.current){old->preemptions++;sched_state.preemptions++;}
        sched_state.switches++;tasks[chosen].switches++;
        sched_state.current=chosen;
    }
    if(!tasks[chosen].context)bad();
    return tasks[chosen].context;
}
