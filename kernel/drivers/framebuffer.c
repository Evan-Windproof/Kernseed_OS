/* SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 */
#include <kernseed/console.h>
#include <limine.h>

/* Original 5x7 bitmap alphabet. Lowercase is displayed as uppercase.
 * Each row occupies five low bits, leftmost pixel in bit 4. No font dependency. */
static const uint8_t font[128][7] = {
    ['A']={14,17,17,31,17,17,17}, ['B']={30,17,17,30,17,17,30},
    ['C']={14,17,16,16,16,17,14}, ['D']={30,17,17,17,17,17,30},
    ['E']={31,16,16,30,16,16,31}, ['F']={31,16,16,30,16,16,16},
    ['G']={14,17,16,23,17,17,15}, ['H']={17,17,17,31,17,17,17},
    ['I']={14,4,4,4,4,4,14}, ['J']={7,2,2,2,18,18,12},
    ['K']={17,18,20,24,20,18,17}, ['L']={16,16,16,16,16,16,31},
    ['M']={17,27,21,21,17,17,17}, ['N']={17,25,21,19,17,17,17},
    ['O']={14,17,17,17,17,17,14}, ['P']={30,17,17,30,16,16,16},
    ['Q']={14,17,17,17,21,18,13}, ['R']={30,17,17,30,20,18,17},
    ['S']={15,16,16,14,1,1,30}, ['T']={31,4,4,4,4,4,4},
    ['U']={17,17,17,17,17,17,14}, ['V']={17,17,17,17,17,10,4},
    ['W']={17,17,17,21,21,21,10}, ['X']={17,17,10,4,10,17,17},
    ['Y']={17,17,10,4,4,4,4}, ['Z']={31,1,2,4,8,16,31},
    ['0']={14,17,19,21,25,17,14}, ['1']={4,12,4,4,4,4,14},
    ['2']={14,17,1,2,4,8,31}, ['3']={30,1,1,14,1,1,30},
    ['4']={2,6,10,18,31,2,2}, ['5']={31,16,16,30,1,1,30},
    ['6']={14,16,16,30,17,17,14}, ['7']={31,1,2,4,8,8,8},
    ['8']={14,17,17,14,17,17,14}, ['9']={14,17,17,15,1,1,14},
    ['.']={0,0,0,0,0,6,6}, [':']={0,6,6,0,6,6,0},
    ['-']={0,0,0,31,0,0,0}, ['/']={1,2,2,4,8,8,16},
    ['[']={14,8,8,8,8,8,14}, [']']={14,2,2,2,2,2,14},
    ['=']={0,0,31,0,31,0,0}, ['?']={14,17,1,2,4,0,4},
    ['_']={0,0,0,0,0,0,31}, ['(']={2,4,8,8,8,4,2},
    [')']={8,4,2,2,2,4,8}, ['!']={4,4,4,4,4,0,4},
    ['>']={16,8,4,2,4,8,16}, ['<']={1,2,4,8,4,2,1},
    ['\'']={4,4,0,0,0,0,0}, ['\"']={10,10,0,0,0,0,0},
    [',']={0,0,0,0,6,4,8}, [';']={0,6,6,0,6,4,8},
    ['+']={0,4,4,31,4,4,0}, ['\\']={16,8,8,4,2,2,1}
};
static struct limine_framebuffer *screen;
static uint64_t column, row;
static uint32_t foreground;
enum { CELL_W = 12, CELL_H = 18 };

static void pixel(uint64_t x, uint64_t y, uint32_t color) {
    volatile uint32_t *p = (volatile uint32_t *)((uint8_t *)screen->address
                                               + y * screen->pitch + x * 4);
    *p = color;
}
static void clear(void) {
    for (uint64_t y = 0; y < screen->height; ++y)
        for (uint64_t x = 0; x < screen->width; ++x) pixel(x, y, 0);
    column = row = 0;
}
bool framebuffer_init(struct limine_framebuffer *fb) {
    /* Phase 1 supports RGB 32bpp, byte-aligned 8-bit channels. */
    if (!fb || !fb->address || ((uintptr_t)fb->address & 3) || fb->bpp != 32 || fb->memory_model != 1 ||
        fb->red_mask_size != 8 || fb->green_mask_size != 8 || fb->blue_mask_size != 8 ||
        fb->red_mask_shift > 24 || fb->green_mask_shift > 24 || fb->blue_mask_shift > 24 ||
        fb->red_mask_shift % 8 || fb->green_mask_shift % 8 || fb->blue_mask_shift % 8 ||
        fb->red_mask_shift == fb->green_mask_shift || fb->red_mask_shift == fb->blue_mask_shift ||
        fb->green_mask_shift == fb->blue_mask_shift || fb->width < CELL_W ||
        fb->height < CELL_H || fb->width > 8192 || fb->height > 8192 ||
        fb->pitch < fb->width * 4 || fb->pitch > 65536 || fb->pitch % 4) return false;
    screen = fb;
    foreground = (0xddU << fb->red_mask_shift) | (0xeeU << fb->green_mask_shift)
               | (0xffU << fb->blue_mask_shift);
    clear();
    return true;
}
void framebuffer_putc(char c) {
    if (!screen) return;
    if (c == '\r') { column = 0; return; }
    if (c == '\n') { column = 0; ++row; }
    else {
        if (c >= 'a' && c <= 'z') c -= 'a' - 'A';
        if ((unsigned char)c >= 128 || (c < ' ')) c = '?';
        uint64_t x = column * CELL_W, y = row * CELL_H;
        for (unsigned dy = 0; dy < CELL_H; ++dy)
            for (unsigned dx = 0; dx < CELL_W; ++dx) {
                bool lit = dx < 10 && dy < 14 &&
                           (font[(unsigned char)c][dy / 2] & (1U << (4 - dx / 2)));
                pixel(x + dx, y + dy, lit ? foreground : 0);
            }
        if (++column == screen->width / CELL_W) { column = 0; ++row; }
    }
    if (row == screen->height / CELL_H) {
        uint64_t text_height=(screen->height/CELL_H)*CELL_H;
        for(uint64_t y=0;y<text_height-CELL_H;y++)
            for(uint64_t x=0;x<screen->width;x++){
                volatile uint32_t *source=(volatile uint32_t *)((uint8_t *)screen->address+(y+CELL_H)*screen->pitch+x*4);
                pixel(x,y,*source);
            }
        for(uint64_t y=text_height-CELL_H;y<screen->height;y++)
            for(uint64_t x=0;x<screen->width;x++)pixel(x,y,0);
        row--;
    }
}

void framebuffer_clear(void){if(screen)clear();}
void framebuffer_backspace(void){
    if(!screen)return;
    if(column)column--;
    else if(row){row--;column=screen->width/CELL_W-1;}
    else return;
    for(unsigned y=0;y<CELL_H;y++)for(unsigned x=0;x<CELL_W;x++)pixel(column*CELL_W+x,row*CELL_H+y,0);
}
