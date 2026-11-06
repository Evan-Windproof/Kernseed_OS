/* SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 */
#include <kernseed/console.h>
#include <limine.h>
#include <stdio.h>

int main(void) {
    enum { W = 48, H = 36, STRIDE = 52, GUARD = 16 };
    uint32_t memory[GUARD + STRIDE * H + GUARD];
    for (unsigned i = 0; i < sizeof(memory) / sizeof(memory[0]); ++i) memory[i] = 0xdeadbeef;
    struct limine_framebuffer fb = {
        .address = memory + GUARD, .width = W, .height = H, .pitch = STRIDE * 4,
        .bpp = 32, .memory_model = 1, .red_mask_size = 8, .red_mask_shift = 0,
        .green_mask_size = 8, .green_mask_shift = 8, .blue_mask_size = 8, .blue_mask_shift = 16
    };
    if (!framebuffer_init(&fb)) return 1;
    const unsigned rows[7] = {17, 18, 20, 24, 20, 18, 17};
    framebuffer_putc('K');
    for (unsigned y = 0; y < 18; ++y)
        for (unsigned x = 0; x < 12; ++x) {
            int lit = x < 10 && y < 14 && (rows[y / 2] & (1U << (4 - x / 2)));
            if (memory[GUARD + y * STRIDE + x] != (lit ? 0xffeeddU : 0)) return 2;
        }
    framebuffer_backspace();
    for(unsigned y=0;y<H;y++)for(unsigned x=0;x<W;x++)
        if(memory[GUARD+y*STRIDE+x])return 7;
    framebuffer_putc('A');framebuffer_putc('\n');
    framebuffer_putc('B');framebuffer_putc('\n');
    const unsigned brow[7]={30,17,17,30,17,17,30};
    for(unsigned y=0;y<H;y++)for(unsigned x=0;x<W;x++){
        int lit=x<10&&y<14&&(brow[y/2]&(1U<<(4-x/2)));
        if(memory[GUARD+y*STRIDE+x]!=(lit?0xffeeddU:0))return 8;
    }
    framebuffer_clear();
    for(unsigned y=0;y<H;y++)for(unsigned x=0;x<W;x++)
        if(memory[GUARD+y*STRIDE+x])return 9;
    /* Exercise glyph fallback, row wrap, scrolling, CR and LF. */
    for (unsigned i = 0; i < 1000; ++i) framebuffer_putc((char)(i & 255));
    framebuffer_putc('\r'); framebuffer_putc('\n');
    for (unsigned i = 0; i < GUARD; ++i)
        if (memory[i] != 0xdeadbeef || memory[GUARD + STRIDE * H + i] != 0xdeadbeef) return 3;
    for (unsigned y = 0; y < H; ++y)
        for (unsigned x = W; x < STRIDE; ++x)
            if (memory[GUARD + y * STRIDE + x] != 0xdeadbeef) return 4;
    if (framebuffer_init(0)) return 5;
    for (unsigned test = 0; test < 10; ++test) {
        struct limine_framebuffer bad = fb;
        switch (test) {
            case 0: bad.address = 0; break;
            case 1: bad.width = 0; break;
            case 2: bad.height = 17; break;
            case 3: bad.pitch = W * 4 - 1; break;
            case 4: bad.bpp = 24; break;
            case 5: bad.memory_model = 0; break;
            case 6: bad.red_mask_shift = bad.blue_mask_shift; break;
            case 7: bad.green_mask_size = 5; break;
            case 8: bad.width = 8193; break;
            case 9: bad.address = (unsigned char *)fb.address + 1; break;
        }
        if (framebuffer_init(&bad)) return 6;
    }
    puts("PASS framebuffer: RGB order, glyph pixels, pitch/edge canaries, wrapping, 11 invalid layouts");
    return 0;
}
