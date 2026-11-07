# Limitations and safety

Single CPU; usable RAM below4GiB, at most256 memmap regions; fixed1GiB VMM window and conservative table retention. No FPU/SIMD/TLS context, SMP, general ELF loader, filesystem, dynamic linker, POSIX or Linux ABI. Built-in fixed images only, one active Ring3 task, one-page code/stack. Synchronous run has no external kill: a nonterminating application can hold the Shell although other tasks remain preemptible. Not production-safe or for important data; no comprehensive security audit or real-hardware guarantee.

[Source / 源码](../../docs/known-limitations.md) · [Index / 目录](index.md)
