/* SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 */
#include <kernseed/user.h>
#include <kernseed/api.h>
#include <kernseed/shell.h>
#include <kernseed/runtime.h>
#include <kernseed/console.h>
volatile struct user_diagnostics user_state;
static uint64_t code_page,stack_page;
#define APP(n) extern const unsigned char app_##n##_start[],app_##n##_end[]
APP(hello);APP(counter);APP(probe);APP(privileged);APP(kernelread);APP(kernelwrite);APP(badflags);APP(badstack);
extern _Noreturn void user_enter(uint64_t rip,uint64_t rsp);
static _Noreturn void start(void){kernel_tss.rsp[0]=tasks[sched_state.current].high;user_enter(USER_CODE,USER_STACK+4096-8);}
bool user_rsp_valid(uint64_t rsp){return user_state.active&&user_state.slot==sched_state.current&&rsp>=USER_STACK&&rsp<USER_STACK+4096;}
void user_reap(unsigned slot){
    if(!user_state.active||slot!=user_state.slot)return;
    if(!vmm_unmap(USER_CODE)||!vmm_unmap(USER_STACK)||!pmm_free(code_page)||!pmm_free(stack_page)){
        const char *s="PANIC: user resource reclamation failed\n";while(*s)serial_putc(*s++);cpu_halt();
    }
    code_page=stack_page=0;
    user_state.active=0;user_state.exits++;
}
int user_run(const char *name){
    /* Single CPU: serialize reservation through publication, including rollback.
     * Nested PMM/VMM/task locks preserve the already-disabled IF state. */
    uint64_t flags;__asm__ volatile("pushfq; popq %0; cli":"=r"(flags)::"memory");
    if(!KERNSEED_USERMODE||!name||user_state.active||!sched_state.active||!sched_state.current){
        __asm__ volatile("pushq %0; popfq"::"r"(flags):"memory","cc");return -1;
    }
    const unsigned char *begin=0,*end=0;const char *stable_name=0;
#define PICK(n) if(shell_equal(name,#n)){begin=app_##n##_start;end=app_##n##_end;stable_name=#n;}
    PICK(hello) PICK(counter) PICK(probe) PICK(privileged) PICK(kernelread) PICK(kernelwrite) PICK(badflags) PICK(badstack)
    uintptr_t first=(uintptr_t)begin,last=(uintptr_t)end;
    if(!begin||last<=first||last-first>4096){
        __asm__ volatile("pushq %0; popfq"::"r"(flags):"memory","cc");return -1;
    }
    code_page=pmm_alloc(1);stack_page=pmm_alloc(1);
    if(!code_page||!stack_page)goto failed;
    memset(physical_pointer(code_page),0,4096);memcpy(physical_pointer(code_page),begin,(size_t)(last-first));
    memset(physical_pointer(stack_page),0,4096);
    if(!vmm_map(USER_CODE,code_page,4))goto failed;
    if(!vmm_map(USER_STACK,stack_page,4|VM_WRITE|VM_NX)){vmm_unmap(USER_CODE);goto failed;}
    int slot=task_create(stable_name,start);
    if(slot<0){vmm_unmap(USER_CODE);vmm_unmap(USER_STACK);goto failed;}
    user_state.slot=(uint64_t)slot;user_state.active=1;
    __asm__ volatile("pushq %0; popfq"::"r"(flags):"memory","cc");
    while(user_state.active)task_sleep(1);
    return 0;
failed:
    if(code_page)pmm_free(code_page);
    if(stack_page)pmm_free(stack_page);
    code_page=stack_page=0;
    __asm__ volatile("pushq %0; popfq"::"r"(flags):"memory","cc");
    return -1;
}
static bool copy_user(char *out,uint64_t address,uint64_t size){
    if(size>256||size>UINT64_MAX-address)return false;
    for(uint64_t i=0;i<size;i++){
        uint64_t a=address+i,p,flags;
        if(!((a>=USER_CODE&&a<USER_CODE+4096)||(a>=USER_STACK&&a<USER_STACK+4096)))return false;
        if(!vmm_query(a,&p,&flags)||!(flags&4))return false;
        uint64_t expected=a>=USER_STACK?stack_page:code_page;
        if((p&~4095ULL)!=expected)return false;
        const char *base=physical_pointer(p&~4095ULL);if(!base)return false;
        out[i]=base[p&4095];
    }
    out[size]=0;return true;
}
void syscall_dispatch(struct interrupt_frame *f){
    if((f->cs&3)!=3||!user_state.active||sched_state.current!=user_state.slot){f->rax=(uint64_t)-1;return;}
    if(!user_rsp_valid(f->rsp)||f->rip<USER_CODE||f->rip>=USER_CODE+4096)user_fault(f);
    user_state.calls++;user_state.last_cs=f->cs;user_state.last_rsp=f->rsp;user_state.last_number=f->rax;
    switch(f->rax){
    case 0:{char text[257];if(!copy_user(text,f->rdi,f->rsi)){f->rax=(uint64_t)-14;user_state.invalid++;return;}
        api_write(text);f->rax=f->rsi;return;}
    case 1:f->rax=api_ticks();return;
    case 2:task_yield();f->rax=0;return;
    case 3:api_exit();
    default:f->rax=(uint64_t)-38;user_state.invalid++;return;
    }
}
_Noreturn void user_fault(struct interrupt_frame *f){
    user_state.fault_rip=f->rip;user_state.last_rsp=f->rsp;
    user_state.faults++;user_state.vector=f->vector;user_state.error=f->error;
    __asm__ volatile("mov %%cr2,%0":"=r"(user_state.cr2));
    api_write("User fault: task terminated\n");api_exit();
}
