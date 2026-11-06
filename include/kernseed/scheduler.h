/* SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 */
#ifndef KERNSEED_SCHEDULER_H
#define KERNSEED_SCHEDULER_H
#include <kernseed/interrupts.h>
#include <stdbool.h>
#define TASK_LIMIT 8
#define STACK_PAGES 4
#define TASK_STACK_BASE (0xffffc00000200000ULL)
#define TASK_STRIDE (6ULL*4096)
enum task_status { UNUSED, RUNNING, READY, BLOCKED, TERMINATED };
struct task {
    uint64_t id;
    const char *name;
    enum task_status status;
    uint64_t physical, low, high;
    struct interrupt_frame *context;
    void (*entry)(void);
    uint64_t switches, preemptions, runtime_ticks, yields, wake_tick;
    uint64_t reclaimed;
};
struct scheduler_stats {
    uint64_t active,current,switches,preemptions,yields,exits,reaped,errors;
    uint64_t slice,create_failures,rollback_passed,records,idle_ticks;
    uint64_t observed_a,observed_b,observed_tick;
    uint64_t capacity_passed,recycle_passed;
};
extern struct task tasks[TASK_LIMIT];
extern volatile struct scheduler_stats sched_state;
extern volatile uint64_t demo_a,demo_b,demo_stop,register_errors;
void scheduler_start(void);
struct interrupt_frame *scheduler_interrupt(struct interrupt_frame *frame);
int task_create(const char *name,void (*entry)(void));
void task_yield(void);
void task_sleep(uint64_t ticks);
_Noreturn void task_exit(void);
#endif
