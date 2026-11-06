/* SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 */
#include <kernseed/memory.h>
bool memory_span(uint64_t base,uint64_t length,bool reserved,uint64_t *first,uint64_t *last) {
    if(!first||!last||length>UINT64_MAX-base)return false;
    if(!length){*first=*last=0;return true;}
    uint64_t end=base+length;
    *first=base/4096+(!reserved&&(base%4096!=0));
    *last=end/4096+(reserved&&(end%4096!=0));
    if(*last<*first)*last=*first;
    return true;
}
