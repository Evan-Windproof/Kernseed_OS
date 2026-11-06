/* SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 */
#include <kernseed/keyboard.h>
#include <kernseed/io.h>
#define RING_SIZE 256
static char ring[RING_SIZE];
static volatile unsigned head,tail;
static unsigned shifts,ctrl,caps,extended,pause_bytes;
static volatile unsigned discard,overflow_event;
volatile struct keyboard_diagnostics keyboard_state;
static const char plain[128]={
    [2]='1',[3]='2',[4]='3',[5]='4',[6]='5',[7]='6',[8]='7',[9]='8',[10]='9',[11]='0',
    [12]='-',[13]='=',[14]='\b',[15]='\t',[16]='q',[17]='w',[18]='e',[19]='r',[20]='t',
    [21]='y',[22]='u',[23]='i',[24]='o',[25]='p',[26]='[',[27]=']',[28]='\n',[30]='a',
    [31]='s',[32]='d',[33]='f',[34]='g',[35]='h',[36]='j',[37]='k',[38]='l',[39]=';',
    [40]='\'',[41]='`',[43]='\\',[44]='z',[45]='x',[46]='c',[47]='v',[48]='b',[49]='n',
    [50]='m',[51]=',',[52]='.',[53]='/',[57]=' '
};
static const char shifted[128]={
    [2]='!',[3]='@',[4]='#',[5]='$',[6]='%',[7]='^',[8]='&',[9]='*',[10]='(',[11]=')',
    [12]='_',[13]='+',[26]='{',[27]='}',[39]=':',[40]='"',[41]='~',[43]='|',
    [51]='<',[52]='>',[53]='?'
};
static bool wait_input(void){for(unsigned i=0;i<100000;i++)if(!(inb(0x64)&2))return true;return false;}
static bool command(uint8_t c){if(!wait_input())return false;outb(0x64,c);return true;}
static int read_output(void){for(unsigned i=0;i<100000;i++)if(inb(0x64)&1)return inb(0x60);return -1;}
bool keyboard_init(void){
    /* PC i8042 first port, translated set 1; no AUX device support. IF=0 here. */
    if(!command(0xad)||!command(0xa7))return false; /* Disable unused AUX clock too. */
    for(unsigned i=0;i<256&&(inb(0x64)&1);i++)(void)inb(0x60);
    if(!command(0x20))return false;
    int config=read_output();if(config<0)return false;
    if(!command(0x60)||!wait_input())return false;
    outb(0x60,(uint8_t)((config|0x41)&~0x12));
    if(!command(0xae)||!wait_input())return false;
    outb(0x60,0xf4);if(read_output()!=0xfa)return false;
    keyboard_state.ready=1;return true;
}
static void enqueue(char c){
    unsigned next=(head+1)%RING_SIZE;
    if(discard){
        if(c=='\n'&&next!=tail){ring[head]=c;__asm__ volatile("":::"memory");head=next;discard=0;}
        else keyboard_state.dropped++;
        return;
    }
    if(next==tail){keyboard_state.dropped++;discard=1;overflow_event=1;return;}
    ring[head]=c;__asm__ volatile("":::"memory");head=next;
}
void keyboard_irq(void){
    keyboard_state.irqs++;
    for(unsigned i=0;i<32;i++){
        uint8_t status=inb(0x64);if(!(status&1))break;
        uint8_t code=inb(0x60);keyboard_state.bytes++;
        if(status&0xe0){keyboard_state.errors++;continue;}
        if(pause_bytes){pause_bytes--;continue;}
        if(code==0xe1){pause_bytes=5;continue;}
        if(code==0xe0){extended=1;continue;}
        if(extended){extended=0;continue;}
        unsigned release=code&0x80;code&=0x7f;
        if(code==42||code==54){unsigned mask=code==42?1:2;if(release)shifts&=~mask;else shifts|=mask;continue;}
        if(code==29){ctrl=!release;continue;}
        if(release)continue;
        if(code==58){caps^=1;continue;}
        char c=plain[code];if(!c)continue;
        if(ctrl){if(c=='u')enqueue(21);continue;}
        if(c>='a'&&c<='z'){if((shifts!=0)^caps)c-='a'-'A';}
        else if(shifts&&shifted[code])c=shifted[code];
        enqueue(c);
    }
}
int keyboard_getchar(void){
    uint64_t flags;__asm__ volatile("pushfq; popq %0; cli":"=r"(flags)::"memory");
    int c=-1;if(overflow_event){tail=head;overflow_event=0;c=127;}else if(tail!=head){c=(unsigned char)ring[tail];tail=(tail+1)%RING_SIZE;}
    __asm__ volatile("pushq %0; popfq"::"r"(flags):"memory","cc");return c;
}
