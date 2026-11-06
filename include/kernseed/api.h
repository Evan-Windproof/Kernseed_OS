/* SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 */
#ifndef KERNSEED_API_H
#define KERNSEED_API_H
#include <kernseed/memory.h>
#include <kernseed/scheduler.h>
void api_memory(struct memory_stats *out);
void api_tasks(struct task out[TASK_LIMIT]);
uint64_t api_ticks(void);
void api_write(const char *text);
_Noreturn void api_exit(void);
#endif
