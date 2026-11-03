/* SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 */
#include <kernseed/runtime.h>
#include <stdio.h>

int main(void) {
    if (runtime_selftest()) return 1;
    unsigned cases = 0;
    /* Exhaust both overlap directions, self-copy, zero length, and boundaries.
     * The oracle reads from a separate snapshot; it never uses memmove. */
    for (size_t from = 0; from < 32; ++from)
        for (size_t to = 0; to < 32; ++to)
            for (size_t n = 0; n <= 32 - from && n <= 32 - to; ++n) {
                unsigned char actual[34], expected[34], snapshot[34];
                for (size_t i = 0; i < 34; ++i)
                    actual[i] = expected[i] = snapshot[i] = (unsigned char)(i * 7 + 3);
                for (size_t i = 0; i < n; ++i) expected[to + 1 + i] = snapshot[from + 1 + i];
                if (memmove(actual + 1 + to, actual + 1 + from, n) != actual + 1 + to) return 2;
                for (size_t i = 0; i < 34; ++i) if (actual[i] != expected[i]) return 3;
                ++cases;
            }
    for (unsigned n = 0; n <= 32; ++n) {
        unsigned char a[34], b[34];
        for (unsigned i = 0; i < 34; ++i) { a[i] = 0x5a; b[i] = (unsigned char)i; }
        if (memset(a + 1, 0x1ff, n) != a + 1) return 4;
        for (unsigned i = 0; i < 34; ++i)
            if (a[i] != (i >= 1 && i <= n ? 0xff : 0x5a)) return 5;
        if (memcpy(a + 1, b + 1, n) != a + 1) return 6;
        for (unsigned i = 0; i < 34; ++i)
            if (a[i] != (i >= 1 && i <= n ? b[i] : 0x5a)) return 7;
    }
    puts("PASS runtime: overlap matrix, copy/fill canaries, comparison selftest");
    printf("memmove cases: %u\n", cases);
    return 0;
}
