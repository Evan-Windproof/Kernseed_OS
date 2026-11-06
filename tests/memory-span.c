/* SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 */
#include <kernseed/memory.h>
#include <stdio.h>
int main(void){
    uint64_t a,b;
    if(!memory_span(4097,8190,false,&a,&b)||a!=2||b!=2)return 1;
    if(!memory_span(4097,8190,true,&a,&b)||a!=1||b!=3)return 2;
    if(memory_span(UINT64_MAX-10,11,false,&a,&b))return 3;
    if(!memory_span(4096,8192,false,&a,&b)||a!=1||b!=3)return 4;
    if(!memory_span(123,0,false,&a,&b)||a||b)return 5;
    if(!memory_span(UINT64_MAX-4095,4095,true,&a,&b)||b!=(1ULL<<52))return 6;
    puts("PASS memory span: unaligned usable/reserved, zero, boundaries, overflow");return 0;
}
