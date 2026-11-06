/* SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 */
#include "abi.h"
void _start(void){
    unsigned long t=call(1,0,0);
    do {for(volatile unsigned long i=0;i<100000;i++){} }while((unsigned long)call(1,0,0)-t<50);
    char b[32];int at=31;b[at]=0;do{b[--at]='0'+t%10;t/=10;}while(t);
    write_text("Counter ticks: ");write_text(b+at);write_text("\n");call(2,0,0);finish();
}
