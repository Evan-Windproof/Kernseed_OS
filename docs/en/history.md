# Development history

## Phase 1 — Boot

[Goals, implementation, difficulties, executed checks, limits and dependencies](../reports/phase-1.md).

## Phase 2 — Interrupts

[Goals, implementation, difficulties, executed checks, limits and dependencies](../reports/phase-2.md).

## Phase 3 — PMM/VMM

[Goals, implementation, difficulties, executed checks, limits and dependencies](../phase3-test-report.md).

## Phase 4 — Preemption

[Goals, implementation, difficulties, executed checks, limits and dependencies](../phase4-test-report.md).

## Phase 5 — Shell

[Goals, implementation, difficulties, executed checks, limits and dependencies](../phase5-test-report.md).

## Phase 6 — Ring3/syscalls

[Goals, implementation, difficulties, executed checks, limits and dependencies](../phase6-test-report.md).

## Phase 7 — Stabilization

[Goals, implementation, difficulties, executed checks, limits and dependencies](../phase7-test-report.md).

Each phase builds on the previous one: logging enables exception diagnosis, memory enables stacks, scheduling enables Shell, and descriptor/page protections enable Ring3. Reports preserve failed experiments rather than reconstructing unrecorded dates.

[Index](index.md)

## Phase comparison

| Phase | Goal / implementation | Difficulty | Verification | Limitation | Next dependency |
|---|---|---|---|---|---|
| 1 Boot | Independent ELF, serial and framebuffer | Freestanding ABI and BSS | CPU entry + pixel checks | No interrupts yet | Diagnostic output |
| 2 Interrupts | GDT/TSS/IDT, PIC/PIT | Error frames, DF IST | Real CPU faults, GDB | No allocator yet | Fault diagnosis |
| 3 Memory | PMM + controlled VMM | Ownership, alignment, references | Four RAM sizes, exhaustion, permissions | No table reclamation | Independent stacks |
| 4 Scheduling | IRQ0 round-robin | Full register/frame restoration | Non-yield sentinels, exit/reap | Single CPU, no SIMD | Shell task |
| 5 Shell | PS/2, parser, registry | Overflow and display ownership | QMP keys, backpressure, reboot | ASCII, bounded input | Registered apps/run commands |
| 6 Ring3 | INT82, native built-in apps | TSS stack and user validation | Actual CPL, GP/PF, cleanup | No general ELF loader | Integration workload |
| 7 Stability | Audit and bounded fixes | Creation critical section, fixture timing | 120s scheduling, repeated apps, clean hashes | Finite testing, no production guarantee | Frozen teaching baseline |
