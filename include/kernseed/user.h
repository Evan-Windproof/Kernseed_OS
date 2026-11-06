/* SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 */
#ifndef KERNSEED_USER_H
#define KERNSEED_USER_H
#include <kernseed/interrupts.h>
#include <stdbool.h>
#define USER_CODE 0xffffc00001000000ULL
#define USER_STACK 0xffffc00001100000ULL
struct user_diagnostics {uint64_t slot,active,calls,last_cs,last_rsp,last_number,exits,faults,vector,error,cr2,invalid,preemptions,fault_rip;};
extern volatile struct user_diagnostics user_state;
bool user_rsp_valid(uint64_t rsp);
void user_reap(unsigned slot);
void syscall_dispatch(struct interrupt_frame *frame);
_Noreturn void user_fault(struct interrupt_frame *frame);
int user_run(const char *name);
#endif
