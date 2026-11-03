/* SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 */
#ifndef KERNSEED_RUNTIME_H
#define KERNSEED_RUNTIME_H
#include <stddef.h>
void *memcpy(void *restrict dst, const void *restrict src, size_t n);
void *memmove(void *dst, const void *src, size_t n);
void *memset(void *dst, int value, size_t n);
int memcmp(const void *a, const void *b, size_t n);
int runtime_selftest(void);
#endif
