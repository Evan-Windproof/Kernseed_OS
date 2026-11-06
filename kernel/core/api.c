/* SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 */
#include <kernseed/api.h>
#include <kernseed/terminal.h>
static uint64_t lock(void){uint64_t f;__asm__ volatile("pushfq; popq %0; cli":"=r"(f)::"memory");return f;}
static void unlock(uint64_t f){__asm__ volatile("pushq %0; popfq"::"r"(f):"memory","cc");}
void api_memory(struct memory_stats *out){uint64_t f=lock();*out=memory_state;unlock(f);}
void api_tasks(struct task out[TASK_LIMIT]){uint64_t f=lock();for(unsigned i=0;i<TASK_LIMIT;i++)out[i]=tasks[i];unlock(f);}
uint64_t api_ticks(void){return irq_state.ticks;}
void api_write(const char *s){terminal_write(s);}
_Noreturn void api_exit(void){task_exit();}
