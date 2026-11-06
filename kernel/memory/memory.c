/* SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 */
#include <kernseed/memory.h>
#include <kernseed/runtime.h>
#include <limine.h>
/* Static metadata belongs to the ELF kernel allocation and is never free.
 * Teaching limit: reject usable RAM above 4 GiB, rather than truncate silently.
 * State bytes are a bitmap alternative: RESERVED/FREE/OWNED/TABLE, plus refs. */
#define FRAMES (1ULL << 20)
#define MASK 0x000ffffffffff000ULL
static uint8_t state[FRAMES];
static uint16_t refs[FRAMES];
static struct limine_memmap_entry regions[256];
static uint64_t region_count, ceiling, cursor=1;
volatile struct memory_stats memory_state;
volatile enum vmm_error_code vmm_error;
__attribute__((used,section(".limine_requests")))
static volatile struct limine_memmap_request map_request={.id=LIMINE_MEMMAP_REQUEST};
__attribute__((used,section(".limine_requests")))
static volatile struct limine_hhdm_request hhdm_request={.id=LIMINE_HHDM_REQUEST};
static uint64_t lock(void) { uint64_t f; __asm__ volatile("pushfq; popq %0; cli":"=r"(f)::"memory"); return f; }
static void unlock(uint64_t f) { __asm__ volatile("pushq %0; popfq"::"r"(f):"memory","cc"); }
static bool ram(uint64_t p) {
    if (p & 4095) return false;
    for (uint64_t i=0;i<region_count;i++) {
        struct limine_memmap_entry *r=&regions[i];
        if ((r->type==0 || r->type==5 || r->type==6) && p>=r->base &&
            r->length>=4096 && p-r->base<=r->length-4096) return true;
    }
    return false;
}
void *physical_pointer(uint64_t p) {
    if (!ram(p) || p>UINT64_MAX-memory_state.hhdm) return 0;
    return (void *)(uintptr_t)(p+memory_state.hhdm);
}
static uint64_t allocate(uint64_t n, uint8_t kind) {
    if (!n || n>memory_state.free) return 0;
    uint64_t run=0;
    for (uint64_t pass=0;pass<2;pass++) {
    run=0;
    uint64_t start=pass ? 1 : (n==1 ? cursor : 1);
    uint64_t end=pass ? (n==1 ? cursor : 1) : ceiling;
    for (uint64_t i=start;i<end;i++) {
        run=state[i]==1 ? run+1 : 0;
        if (run==n) {
            uint64_t first=i+1-n;
            for(uint64_t j=first;j<=i;j++) state[j]=kind;
            memory_state.free-=n;
            if(kind==2) memory_state.allocated+=n; else memory_state.tables+=n;
            cursor=i+1;return first*4096;
        }
    }
    }
    return 0;
}
uint64_t pmm_alloc(uint64_t n) { uint64_t f=lock(),p=allocate(n,2); unlock(f); return p; }
bool pmm_allocated(uint64_t p) { uint64_t f=lock(); bool b=!(p&4095)&&p/4096<ceiling&&state[p/4096]==2; unlock(f); return b; }
bool pmm_free(uint64_t p) {
    uint64_t f=lock(),i=p/4096;
    bool ok=!(p&4095)&&i<ceiling&&state[i]==2&&!refs[i];
    if(ok) {if(i<cursor)cursor=i;state[i]=1;memory_state.free++;memory_state.allocated--;}
    unlock(f);return ok;
}
static bool canonical(uint64_t v) { return v>>47==0 || v>>47==0x1ffff; }
bool vmm_query(uint64_t v,uint64_t *p,uint64_t *flags) {
    if(!canonical(v)||!p||!flags) return false;
    uint64_t f=lock(),pa=memory_state.cr3; bool writable=true,nx=false,user=true,ok=false;
    for(int level=3;level>=0;level--) {
        uint64_t *t=physical_pointer(pa); if(!t) break;
        uint64_t e=t[(v>>(12+level*9))&511]; if(!(e&1)) break;
        writable=writable&&((e&2)!=0); nx=nx||((e&VM_NX)!=0);user=user&&((e&4)!=0);
        if(level==0 || ((e&128)&&(level==1||level==2))) {
            uint64_t size=1ULL<<(12+9*level);
            *p=((e&MASK)&~(size-1))+(v&(size-1));
            *flags=1|(user?4:0)|(writable?2:0)|(nx?VM_NX:0);ok=true;break;
        }
        if(e&128) break;
        pa=e&MASK;
    }
    unlock(f);return ok;
}
bool vmm_map(uint64_t v,uint64_t p,uint64_t flags) {
    vmm_error=MEM_INVALID;
    if(v<VM_BASE || v-VM_BASE>=VM_SIZE || (v&4095) || (p&4095) ||
       flags&~(VM_WRITE|VM_NX|4ULL)) return false;
    if((flags&4)&&(v<0xffffc00001000000ULL||v>=0xffffc00001200000ULL))return false;
    uint64_t f=lock(),i=p/4096,pa=memory_state.cr3;bool ok=false;
    vmm_error=MEM_OWNERSHIP;
    if(i>=ceiling || state[i]!=2 || refs[i]==UINT16_MAX) goto done;
    for(int level=3;level>0;level--) {
        uint64_t *t=physical_pointer(pa); if(!t) goto done;
        uint64_t index=(v>>(12+9*level))&511,e=t[index];
        if(!(e&1)) {
            uint64_t next=allocate(1,3); if(!next) {vmm_error=MEM_OOM;goto done;}
            void *ptr=physical_pointer(next);
            if(!ptr) goto done; /* Initialization ensures every FREE page is HHDM RAM. */
            memset(ptr,0,4096);t[index]=next|3;e=t[index];
        }
        /* Never split huge mappings or enter bootloader-owned subtrees. */
        if(e&128) {vmm_error=MEM_HUGE;goto done;}
        if((e&MASK)/4096>=ceiling||state[(e&MASK)/4096]!=3) goto done;
        if(flags&4)t[index]|=4;
        pa=e&MASK;
    }
    uint64_t *t=physical_pointer(pa),index=(v>>12)&511;
    if(!t) goto done;
    if(t[index]&1) {vmm_error=MEM_CONFLICT;goto done;}
    t[index]=p|flags|1;refs[i]++;memory_state.mapped++;
    __asm__ volatile("invlpg (%0)"::"r"(v):"memory");ok=true;vmm_error=MEM_OK;
 done: unlock(f);return ok;
}
bool vmm_unmap(uint64_t v) {
    if(v<VM_BASE||v-VM_BASE>=VM_SIZE||(v&4095)) return false;
    uint64_t f=lock(),pa=memory_state.cr3;bool ok=false;
    for(int level=3;level>0;level--) {
        uint64_t *t=physical_pointer(pa);if(!t) goto done;
        uint64_t e=t[(v>>(12+9*level))&511],i=(e&MASK)/4096;
        if(!(e&1)||(e&128)||i>=ceiling||state[i]!=3) goto done;
        pa=e&MASK;
    }
    uint64_t *t=physical_pointer(pa),index=(v>>12)&511,e=t?t[index]:0,i=(e&MASK)/4096;
    if(!(e&1)||i>=ceiling||state[i]!=2||!refs[i]) goto done;
    t[index]=0;__asm__ volatile("invlpg (%0)"::"r"(v):"memory");
    refs[i]--;memory_state.mapped--;ok=true;
 done:unlock(f);return ok;
}
bool memory_init(void) {
    struct limine_memmap_response *m=map_request.response;
    if(!m||!m->entries||!m->entry_count||m->entry_count>256||!hhdm_request.response) return false;
    memory_state.hhdm=hhdm_request.response->offset;
    if(!canonical(memory_state.hhdm)) return false;
    uint64_t cr0;__asm__ volatile("mov %%cr0,%0":"=r"(cr0));
    if((cr0&0x80010000ULL)!=0x80010000ULL)return false;
    uint64_t cr4;__asm__ volatile("mov %%cr4,%0":"=r"(cr4));if(cr4&((1ULL<<12)|(1ULL<<17)))return false;
    uint32_t lo,hi;__asm__ volatile("rdmsr":"=a"(lo),"=d"(hi):"c"(0xc0000080));
    if(!(lo&(1U<<11)))return false;
    __asm__ volatile("mov %%cr3,%0":"=r"(memory_state.cr3));memory_state.cr3&=MASK;
    region_count=m->entry_count;
    for(uint64_t i=0;i<region_count;i++) {
        if(!m->entries[i])return false;
        regions[i]=*m->entries[i];struct limine_memmap_entry *r=&regions[i];
        if(r->length>UINT64_MAX-r->base)return false;
        if(r->type==0) {
            uint64_t first,end;
            if(!memory_span(r->base,r->length,false,&first,&end))return false;
            if(end>FRAMES||r->base>UINT64_MAX-4095)return false;
            if(end>ceiling)ceiling=end;
            for(uint64_t j=first;j<end;j++)state[j]=1;
        }
    }
    /* Non-usable overlaps take priority, round reservations outward. */
    for(uint64_t i=0;i<region_count;i++)if(regions[i].type!=0&&regions[i].length) {
        uint64_t first,last;
        if(!memory_span(regions[i].base,regions[i].length,true,&first,&last))return false;
        if(last>ceiling)last=ceiling;
        for(uint64_t j=first;j<last;j++)state[j]=0;
    }
    state[0]=0;
    for(uint64_t j=0;j<ceiling;j++)if(state[j]==1)memory_state.free++;
    memory_state.usable=memory_state.free;
    uint64_t *root=physical_pointer(memory_state.cr3);
    /* Own an initially unused PML4 slot, never edit inherited kernel/HHDM entries. */
    return root && !(root[(VM_BASE>>39)&511]&1) && memory_state.free>16;
}
bool memory_selftest(void) {
    if(memory_state.tests||memory_state.allocated||memory_state.mapped)return false;
    uint64_t before=memory_state.free,p=pmm_alloc(3),q,flags;
    if(!vmm_query((uint64_t)(uintptr_t)physical_pointer(p),&q,&flags)||q!=p)return false;
    if(pmm_free(memory_state.cr3)||vmm_query(0x800000000000ULL,&q,&flags))return false;
    if(!p||(p&4095)||!pmm_allocated(p)||pmm_free(0)||pmm_free(p+1))return false;
    if(!vmm_map(VM_BASE,p,VM_WRITE|VM_NX)||vmm_map(VM_BASE,p,0)||pmm_free(p))return false;
    if(!vmm_query(VM_BASE+17,&q,&flags)||q!=p+17||!(flags&VM_NX))return false;
    *(volatile uint64_t *)VM_BASE=0x123456789abcdef0ULL;
    if(*(uint64_t *)physical_pointer(p)!=0x123456789abcdef0ULL)return false;
    if(vmm_map(0x800000000000ULL,p,0)||vmm_map(VM_BASE+1,p,0))return false;
    if(!vmm_unmap(VM_BASE)||vmm_query(VM_BASE,&q,&flags)||vmm_unmap(VM_BASE))return false;
    if(!pmm_free(p)||pmm_free(p)||!pmm_free(p+4096)||!pmm_free(p+8192))return false;
    if(memory_state.free+memory_state.tables!=before)return false;
    /* Exhaust only owned FREE pages. No page data is overwritten. */
    uint64_t count=0;while(pmm_alloc(1))count++;
    if(memory_state.free||pmm_alloc(1)||pmm_alloc(UINT64_MAX))return false;
    for(uint64_t i=1;i<ceiling;i++)if(state[i]==2&&!pmm_free(i*4096))return false;
    if(memory_state.allocated||memory_state.mapped||memory_state.free+memory_state.tables!=before||!count)return false;
    memory_state.tests=1;return true;
}

/* Isolated fault probes are absent from normal builds. */
void memory_fault_test(void) {
#if KERNSEED_TEST_FAULT >= 6 && KERNSEED_TEST_FAULT <= 8
    uint64_t p=pmm_alloc(1);
    if(!p||!vmm_map(VM_BASE,p,VM_NX)) __asm__ volatile("ud2");
#if KERNSEED_TEST_FAULT == 6
    if(!vmm_unmap(VM_BASE)) __asm__ volatile("ud2");
    __asm__ volatile(".global memory_fault_site\nmemory_fault_site: mov (%0), %%rax"::"r"(VM_BASE):"rax","memory");
#elif KERNSEED_TEST_FAULT == 7
    __asm__ volatile(".global memory_fault_site\nmemory_fault_site: movq $1,(%0)"::"r"(VM_BASE):"memory");
#elif KERNSEED_TEST_FAULT == 8
    ((void (*)(void))VM_BASE)();
#endif
#endif
}
