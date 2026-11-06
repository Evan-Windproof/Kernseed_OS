/* SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 */
static inline long call(long n,long a,long b){register long rax __asm__("rax")=n;__asm__ volatile("int $0x82":"+a"(rax):"D"(a),"S"(b):"memory","cc");return rax;}
static inline void write_text(const char *s){long n=0;while(s[n])n++;call(0,(long)s,n);}
static inline __attribute__((noreturn)) void finish(void){call(3,0,0);for(;;);}
