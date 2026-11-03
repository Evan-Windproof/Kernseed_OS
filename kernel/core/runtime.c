/* SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 */
#include <kernseed/runtime.h>
#include <stdint.h>

void *memcpy(void *restrict dst, const void *restrict src, size_t n) {
    unsigned char *d = dst;
    const unsigned char *s = src;
    for (size_t i = 0; i < n; ++i) d[i] = s[i];
    return dst;
}
void *memmove(void *dst, const void *src, size_t n) {
    unsigned char *d = dst;
    const unsigned char *s = src;
    if ((uintptr_t)d < (uintptr_t)s) {
        for (size_t i = 0; i < n; ++i) d[i] = s[i];
    } else if (d != s) {
        for (size_t i = n; i; --i) d[i - 1] = s[i - 1];
    }
    return dst;
}
void *memset(void *dst, int value, size_t n) {
    unsigned char *d = dst;
    for (size_t i = 0; i < n; ++i) d[i] = (unsigned char)value;
    return dst;
}
int memcmp(const void *a, const void *b, size_t n) {
    const unsigned char *x = a, *y = b;
    for (size_t i = 0; i < n; ++i)
        if (x[i] != y[i]) return (int)x[i] - (int)y[i];
    return 0;
}
/* Runs inside the kernel, without libc or host services. */
int runtime_selftest(void) {
    unsigned char a[16], b[16];
    if (memset(a, 0xa5, sizeof(a)) != a) return 1;
    if (memcpy(b, a, sizeof(a)) != b || memcmp(a, b, sizeof(a))) return 2;
    for (size_t i = 0; i < sizeof(a); ++i) a[i] = (unsigned char)i;
    if (memmove(a + 3, a, 10) != a + 3) return 3;
    for (size_t i = 0; i < 10; ++i) if (a[i + 3] != i) return 4;
    memmove(a, a + 3, 10);
    for (size_t i = 0; i < 10; ++i) if (a[i] != i) return 5;
    a[0] = 0; b[0] = 255;
    if (memcmp(a, b, 1) >= 0 || memcmp(b, a, 1) <= 0) return 6;
    if (memcmp(a, b, 0) || memmove(a, a, sizeof(a)) != a) return 7;
    return 0;
}
